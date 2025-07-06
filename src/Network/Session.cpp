#include "pch.h"
#include "Session.h"

namespace asio = boost::asio;
namespace ip = boost::asio::ip;

std::atomic<uint64_t> CppMMO::Network::Session::s_nextSessionId = 0;

namespace CppMMO
{
    namespace Network
    {
        Session::Session(ip::tcp::socket socket, std::shared_ptr<IPacketManager> packetManager) 
            : m_socket{std::move(socket)}, 
              m_packetManager{packetManager},
              m_timer(m_socket.get_executor()),
              m_sessionId(s_nextSessionId.fetch_add(1))
        {
            LOG_INFO("Session {} created. Remote endpoint: {}", m_sessionId, m_socket.remote_endpoint().address().to_string());
        }

        ip::tcp::endpoint Session::GetRemoteEndpoint() const
        {
            return m_socket.remote_endpoint();
        }

        void Session::Start()
        {
            LOG_INFO("Session {} started.", m_sessionId);
            asio::co_spawn(m_socket.get_executor(), [self = shared_from_this()]() -> asio::awaitable<void>
            {
                return self->ReadLoop();
            }, asio::detached);
            asio::co_spawn(m_socket.get_executor(), [self = shared_from_this()]() -> asio::awaitable<void>
            {
                return self->WriteLoop();
            }, asio::detached);
        }

        void Session::Disconnect()
        {
            boost::system::error_code ec;
            m_socket.shutdown(ip::tcp::socket::shutdown_both, ec);
            m_socket.close(ec);
            m_timer.cancel();
            LOG_INFO("Session {} closed.", m_sessionId);

            if (m_onDisconnectedCallback)
            {
                m_onDisconnectedCallback(shared_from_this());
            }
        }

        void Session::SetOnDisconnectedCallback(const std::function<void(std::shared_ptr<ISession>)>& callback)
        {
            m_onDisconnectedCallback = callback;
            LOG_DEBUG("Session {}: Disconnected callback set.", m_sessionId);
        }

        bool Session::IsConnected() const
        {
            return m_socket.is_open();
        }

        void Session::Send(std::span<const std::byte> data)
        {
            uint32_t bodyLength = static_cast<uint32_t>(data.size());
            uint32_t totalPacketLength = sizeof(uint32_t) + bodyLength;

            std::vector<std::byte> packetToSend;
            packetToSend.reserve(totalPacketLength);
            uint32_t networkByteOrderLength = htonl(bodyLength);
            std::byte lengthBytes[sizeof(uint32_t)];
            std::memcpy(lengthBytes, &networkByteOrderLength, sizeof(uint32_t));

            packetToSend.insert(packetToSend.end(), std::span(lengthBytes).begin(), std::span(lengthBytes).end());
            
            packetToSend.insert(packetToSend.end(), data.begin(), data.end());

            m_writeQueue.emplace_back(std::move(packetToSend));

            m_timer.cancel_one();
            LOG_INFO("Session {}: Packet of total {} bytes (body {}) added to write queue. Queue size: {}", m_sessionId, totalPacketLength, bodyLength, m_writeQueue.size());
        }

        asio::awaitable<void> Session::ReadLoop()
        {
            try
            {
                while(true)
                {
                    while(m_readBuffer.size() >= sizeof(uint32_t))
                    {
                        uint32_t packetLength = 0;
                        std::memcpy(&packetLength, asio::buffer_cast<const void*>(m_readBuffer.data()), sizeof(uint32_t));
                        packetLength = ntohl(packetLength);

                        if (m_readBuffer.size() >= sizeof(uint32_t) + packetLength)
                        {
                            m_readBuffer.consume(sizeof(uint32_t));
                            const uint8_t* flatbuffer_data = reinterpret_cast<const uint8_t*>(
                                asio::buffer_cast<const std::byte*>(m_readBuffer.data()));
                            const Protocol::UnifiedPacket* unified_packet = 
                                flatbuffers::GetRoot<Protocol::UnifiedPacket>(flatbuffer_data);
                            m_packetManager->HandlePacket(shared_from_this(), unified_packet);
                            m_readBuffer.consume(packetLength);
                            LOG_DEBUG("Session {}: Processed packet with ID: {}", m_sessionId, static_cast<int>(unified_packet->id()));
                        }
                        else
                        {
                            break;
                        }
                    }
                    size_t bytes_transferred = co_await m_socket.async_read_some(
                        m_readBuffer.prepare(4096),
                        asio::use_awaitable
                    );
                    m_readBuffer.commit(bytes_transferred);
                    LOG_DEBUG("Session {}: Read {} bytes from socket.", m_sessionId, bytes_transferred);
                }
            }
            catch(const boost::system::system_error& e)
            {
                HandleError(e.code(), "ReadLoop");
                Disconnect();
                co_return;
            }
            catch(const std::exception& e)
            {
                LOG_ERROR("Session {}: Unexpected exception in ReadLoop: {}", m_sessionId, e.what());
                Disconnect();
                co_return;
            }
        }

        asio::awaitable<void> Session::WriteLoop()
        {
            try
            {
                while(m_socket.is_open())
                {
                    if(m_writeQueue.empty())
                    {
                        boost::system::error_code ec;
                        co_await m_timer.async_wait(asio::redirect_error(asio::use_awaitable, ec));

                        if (ec && ec != asio::error::operation_aborted)
                        {
                            throw boost::system::system_error(ec, "WriteLoop timer wait failed");
                        }
                        continue;
                    }
                    co_await asio::async_write(m_socket, asio::buffer(m_writeQueue.front()), asio::use_awaitable);

                    m_writeQueue.pop_front();
                    LOG_DEBUG("Session {}: Packet sent. Remaining in queue: {}", m_sessionId, m_writeQueue.size());
                }
            }
            catch (const boost::system::system_error& e)
            {
                HandleError(e.code(), "WriteLoop");
                Disconnect();
                co_return;
            }
            catch(const std::exception& e)
            {
                LOG_ERROR("Session {}: Unexpected exception in WriteLoop: {}", m_sessionId, e.what());
                Disconnect();
                co_return;
            }
        }

        void Session::HandleError(const boost::system::error_code& ec, const std::string& operation)
        {
            if (ec == asio::error::eof || ec == asio::error::operation_aborted)
            {
                LOG_INFO("Session {}: {} completed gracefully or aborted. Error: {}", m_sessionId, operation, ec.message());
            }
            else
            {
                LOG_ERROR("Session {}: Error in {} operation. Error code: {} ({})", m_sessionId, operation, ec.value(), ec.message());
            }
        }
    }
}
