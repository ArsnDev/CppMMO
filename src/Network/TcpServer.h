#pragma once
#include "pch.h"
#include "IService.h"

namespace asio = boost::asio;
namespace ip = boost::asio::ip;

namespace CppMMO
{
    namespace Network
    {
        class Session;

        class TcpServer : public IService, public std::enable_shared_from_this<TcpServer>
        {
        public:
            TcpServer(asio::io_context& io_context, unsigned short port);
            virtual ~TcpServer() = default;

            virtual void Start() override;
            virtual void Stop() override;

        private:
            asio::io_context&    m_io_context;
            ip::tcp::acceptor    m_acceptor;
        };
    }
}
