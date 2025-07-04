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
            
        };
    }
}
