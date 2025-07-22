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
            
            // Send in little endian format (consistent with client)
            packetToSend.insert(packetToSend.end(),
                                reinterpret_cast<const std::byte*>(&bodyLength),
                                reinterpret_cast<const std::byte*>(&bodyLength) + sizeof(uint32_t));
            
            packetToSend.insert(packetToSend.end(), data.begin(), data.end());

            m_writeQueue.enqueue(std::move(packetToSend));

            m_timer.cancel_one();
            LOG_INFO("Session {}: Packet of total {} bytes (body {}) added to write queue.", m_sessionId, totalPacketLength, bodyLength);
        }

        uint64_t Session::GetPlayerId() const
        {
            return m_playerId;
        }

        void Session::SetPlayerId(uint64_t playerId)
        {
            m_playerId = playerId;
            LOG_DEBUG("Session {}: PlayerId set to {}.", m_sessionId, m_playerId);
        }

        asio::awaitable<void> Session::ReadLoop()
        {
            try
            {
                while (true)
                {
                    auto [error, bytes_transferred] = co_await asio::async_read(m_socket, asio::buffer(m_readHeader), asio::as_tuple(asio::use_awaitable));
                    if (error)
                    {
                        HandleError(error, "ReadLoop header");
                        Disconnect();
                        co_return;
                    }

                    uint32_t bodyLength;
                    std::memcpy(&bodyLength, m_readHeader.data(), sizeof(uint32_t));
                    
                    // Debug header value
                    LOG_DEBUG("Session {}: Raw header bytes: {:02x} {:02x} {:02x} {:02x}", 
                             m_sessionId, 
                             static_cast<unsigned char>(m_readHeader[0]),
                             static_cast<unsigned char>(m_readHeader[1]),
                             static_cast<unsigned char>(m_readHeader[2]),
                             static_cast<unsigned char>(m_readHeader[3]));
                    
                    // FlatBuffers SizedByteArray() is transmitted in little endian
                    // Used as-is on server (host byte order = little endian)
                    LOG_DEBUG("Session {}: Header body length (little endian): {}", m_sessionId, bodyLength);
                    
                    // Reasonable range check
                    if (bodyLength == 0 || bodyLength > 100000)
                    {
                        LOG_ERROR("Session {}: Invalid header value: {}", m_sessionId, bodyLength);
                        Disconnect();
                        co_return;
                    }

                    m_readBody.resize(bodyLength);
                    auto [body_error, body_bytes_transferred] = co_await asio::async_read(m_socket, asio::buffer(m_readBody), asio::as_tuple(asio::use_awaitable));

                    if (body_error)
                    {
                        HandleError(body_error, "ReadLoop body");
                        Disconnect();
                        co_return;
                    }

                    if (m_packetManager)
                    {
                        LOG_DEBUG("Session {}: Received packet - Header: {} bytes, Body: {} bytes", 
                                 m_sessionId, m_readHeader.size(), m_readBody.size());
                        
                        // Debug hex dump (first 16 bytes)
                        std::string hexDump;
                        for(size_t i = 0; i < std::min(m_readBody.size(), size_t(16)); ++i) {
                            hexDump += fmt::format("{:02x} ", static_cast<unsigned char>(m_readBody[i]));
                        }
                        LOG_DEBUG("Session {}: Body hex dump (first 16 bytes): {}", m_sessionId, hexDump);
                        
                        m_packetManager->HandlePacket(shared_from_this(), m_readBody);
                    }
                    else
                    {
                        LOG_ERROR("PacketManager is null in Session ReadLoop.");
                    }
                }
            }
            catch (const boost::system::system_error& e)
            {
                HandleError(e.code(), "ReadLoop");
                Disconnect();
                co_return;
            }
            catch (const std::exception& e)
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
                    std::vector<std::byte> packetToSend;
                    if(m_writeQueue.try_dequeue(packetToSend))
                    {
                        co_await asio::async_write(m_socket, asio::buffer(packetToSend), asio::use_awaitable);
                        LOG_DEBUG("Session {}: Packet sent.", m_sessionId);
                    }
                    else
                    {
                        boost::system::error_code ec;
                        co_await m_timer.async_wait(asio::redirect_error(asio::use_awaitable, ec));

                        if (ec && ec != asio::error::operation_aborted)
                        {
                            throw boost::system::system_error(ec, "WriteLoop timer wait failed");
                        }
                    }
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

        void Session::HandleError(const boost::system::error_code& ec, std::string_view operation)
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
