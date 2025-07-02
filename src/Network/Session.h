#pragma once
#include "pch.h"
#include "ISession.h"
#include <span>
#include <cstddef>

namespace asio = boost::asio;
namespace ip = boost::asio::ip;

namespace CppMMO
{
    namespace Network
    {
        class Session : public ISession, public std::enable_shared_from_this<Session>
        {
        public:
            Session(asio::io_context& io_context);
            virtual ~Session() = default;

            virtual void Start() override;
            virtual void Send(std::span<const std::byte> data) override;

            ip::tcp::socket& GetSocket();

        private:
            ip::tcp::socket m_socket;
        };
    }
}