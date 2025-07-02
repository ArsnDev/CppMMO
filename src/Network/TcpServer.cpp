#include "pch.h"
#include "TcpServer.h"

namespace asio = boost::asio;
namespace ip = boost::asio::ip;

namespace CppMMO
{
    namespace Network
    {
        TcpServer::TcpServer(asio::io_context& io_context, unsigned short port)
            : m_io_context(io_context)
            , m_acceptor(io_context, ip::tcp::endpoint(ip::tcp::v4(), port))
        {
            LOG_INFO("TcpServer created on port{}", port);
        }

        void TcpServer::Start()
        {
            // TODO: 구현
        }

        void TcpServer::Stop()
        {
            // TODO: 구현
        }
    }
}
