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
              m_sessionId(s_nextSessionId.fetch_add(1)),
              m_writing(false)
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
            m_socket.close();
            m_timer.cancel();
            LOG_INFO("Session {} closed.", m_sessionId);
        }

        bool Session::IsConnected() const
        {
            return m_socket.is_open();
        }

        void Session::Send(std::span<const std::byte> data)
        {
            m_writeQueue.emplace_back(data);
        }

        asio::awaitable<void> Session::ReadLoop()
        {
            co_return;
        }

        asio::awaitable<void> Session::WriteLoop()
        {
            co_return;
        }

        void Session::HandleError(const boost::system::error_code& ec, const std::string& operation)
        {

        }
    }
}
