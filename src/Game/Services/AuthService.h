#pragma once

#include "pch.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

namespace CppMMO
{
    namespace Game
    {
        namespace Services
        {
            struct VerifyTicketResponse
            {
                bool success = false;
                uint64_t playerId = 0;
                std::string username;
                float posX = 0.0f;
                float posY = 0.0f;
                int hp = 0;
                int maxHp = 0;
                std::string errorMessage;
                int errorCode = 0;
            };

            using VerifyCallback = std::function<void(const VerifyTicketResponse&)>;

            class AuthService : public std::enable_shared_from_this<AuthService>
            {
            public:
                AuthService(boost::asio::io_context& ioc, const std::string& auth_host, const std::string& auth_port);
                void VerifySessionTicketAsync(const std::string& sessionTicket, VerifyCallback callback);
            private:
                boost::asio::io_context& m_ioc;
                std::string m_auth_host;
                std::string m_auth_port;

                class HttpRequestSession : public std::enable_shared_from_this<HttpRequestSession>
                {
                public:
                    HttpRequestSession(boost::asio::io_context& ioc, const std::string& host, const std::string& port, VerifyCallback callback);
                    void Run(const std::string& sessionTicket);
                private:
                    void OnResolve(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results);
                    void OnConnect(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type::endpoint_type ep);
                    void OnWrite(boost::beast::error_code ec, std::size_t bytes_transferred);
                    void OnRead(boost::beast::error_code ec, std::size_t bytes_transferred);

                    boost::asio::ip::tcp::resolver m_resolver;
                    boost::beast::tcp_stream m_stream;
                    boost::beast::flat_buffer m_buffer;
                    boost::beast::http::request<boost::beast::http::string_body> m_req;
                    boost::beast::http::response<boost::beast::http::string_body> m_res;
                    VerifyCallback m_callback;
                    std::string m_host;
                    std::string m_port;
                };
            };
        }
    }
}