#include "pch.h"
#include "Session.h"

namespace asio = boost::asio;
namespace ip = boost::asio::ip;

namespace CppMMO
{
    namespace Network
    {
        Session::Session(asio::io_context& io_context)
            : m_socket(io_context)
        {
        }

        void Session::Start()
        {
            // TODO: 구현
        }

        void Session::Send(std::span<const std::byte> data)
        {
            // TODO: 구현
        }

        ip::tcp::socket& Session::GetSocket()
        {
            return m_socket;
        }
    }
}
