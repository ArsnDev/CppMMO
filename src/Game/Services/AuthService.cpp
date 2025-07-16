#include "pch.h"
#include "AuthService.h"
#include <nlohmann/json.hpp>

namespace CppMMO
{
    namespace Game
    {
        namespace Services
        {
            AuthService::AuthService(boost::asio::io_context& ioc, const std::string& auth_host, const std::string& auth_port)
                : m_ioc(ioc), m_auth_host(auth_host), m_auth_port(auth_port)
            {
                LOG_INFO("AuthService initialized for {}:{}", m_auth_host, m_auth_port);
            }

            void AuthService::VerifySessionTicketAsync(const std::string& sessionTicket, VerifyCallback callback)
            {
                std::make_shared<HttpRequestSession>(m_ioc, m_auth_host, m_auth_port, std::move(callback))->Run(sessionTicket);
            }

            AuthService::HttpRequestSession::HttpRequestSession(boost::asio::io_context& ioc, const std::string& host, const std::string& port, VerifyCallback callback)
                : m_resolver(ioc)
                , m_stream(ioc)
                , m_callback(std::move(callback))
                , m_host(host)
                , m_port(port)
            {
            }

            void AuthService::HttpRequestSession::Run(const std::string& sessionTicket)
            {
                m_req.version(11);
                m_req.method(boost::beast::http::verb::post);
                m_req.target("/api/auth/verify");
                m_req.set(boost::beast::http::field::host, m_host);
                m_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
                m_req.set(boost::beast::http::field::content_type, "application/json");

                nlohmann::json request_body;
                request_body["SessionTicket"] = sessionTicket;
                m_req.body() = request_body.dump();
                m_req.prepare_payload();

                LOG_INFO("AuthService::HttpRequestSession: Resolving host {}:{}", m_host, m_port);
                m_resolver.async_resolve(
                    m_host,
                    m_port,
                    boost::beast::bind_front_handler(
                        &HttpRequestSession::OnResolve,
                        shared_from_this()));
            }

            void AuthService::HttpRequestSession::OnResolve(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results)
            {
                if (ec)
                {
                    LOG_ERROR("AuthService::HttpRequestSession: Resolve error: {}", ec.message());
                    VerifyTicketResponse response;
                    response.success = false;
                    response.errorCode = -1;
                    response.errorMessage = "AuthServer DNS resolution failed: " + ec.message();
                    m_callback(response);
                    return;
                }

                LOG_INFO("AuthService::HttpRequestSession: Connecting to endpoint...");
                m_stream.async_connect(
                    results,
                    boost::beast::bind_front_handler(
                        &HttpRequestSession::OnConnect,
                        shared_from_this()));
            }

            void AuthService::HttpRequestSession::OnConnect(boost::beast::error_code ec, [[maybe_unused]] boost::asio::ip::tcp::resolver::results_type::endpoint_type ep)
            {
                if (ec)
                {
                    LOG_ERROR("AuthService::HttpRequestSession: Connect error: {}", ec.message());
                    VerifyTicketResponse response;
                    response.success = false;
                    response.errorCode = -2;
                    response.errorMessage = "AuthServer connection failed: " + ec.message();
                    m_callback(response);
                    return;
                }

                LOG_INFO("AuthService::HttpRequestSession: Connected. Sending HTTP request...");
                boost::beast::http::async_write(
                    m_stream,
                    m_req,
                    boost::beast::bind_front_handler(
                        &HttpRequestSession::OnWrite,
                        shared_from_this()));
            }

            void AuthService::HttpRequestSession::OnWrite(boost::beast::error_code ec, std::size_t bytes_transferred)
            {
                boost::ignore_unused(bytes_transferred);

                if (ec)
                {
                    LOG_ERROR("AuthService::HttpRequestSession: Write error: {}", ec.message());
                    VerifyTicketResponse response;
                    response.success = false;
                    response.errorCode = -4;
                    response.errorMessage = "AuthServer request write failed: " + ec.message();
                    m_callback(response);
                    return;
                }

                LOG_INFO("AuthService::HttpRequestSession: Request sent. Reading HTTP response...");
                boost::beast::http::async_read(
                    m_stream,
                    m_buffer,
                    m_res,
                    boost::beast::bind_front_handler(
                        &HttpRequestSession::OnRead,
                        shared_from_this()));
            }

            void AuthService::HttpRequestSession::OnRead(boost::beast::error_code ec, std::size_t bytes_transferred)
            {
                boost::ignore_unused(bytes_transferred);

                if (ec == boost::beast::http::error::end_of_stream)
                {
                    LOG_INFO("AuthService::HttpRequestSession: AuthServer closed connection gracefully.");
                }
                else if (ec)
                {
                    LOG_ERROR("AuthService::HttpRequestSession: Read error: {}", ec.message());
                    VerifyTicketResponse response;
                    response.success = false;
                    response.errorCode = -5;
                    response.errorMessage = "AuthServer response read failed: " + ec.message();
                    m_callback(response);
                    return;
                }

                VerifyTicketResponse response;
                response.success = false;

                LOG_INFO("AuthService::HttpRequestSession: Received response from AuthServer. Status: {}", m_res.result_int());
                LOG_INFO("Response Body: {}", m_res.body());

                try
                {
                    if (m_res.result() == boost::beast::http::status::ok)
                    {
                        nlohmann::json json_response = nlohmann::json::parse(m_res.body());

                        if (json_response.contains("success") && json_response["success"].is_boolean())
                        {
                            response.success = json_response["success"].get<bool>();
                            if (response.success)
                            {
                                if (json_response.contains("playerInfo") && json_response["playerInfo"].is_object())
                                {
                                    const auto& playerInfo = json_response["playerInfo"];
                                    if (playerInfo.contains("playerId") && playerInfo["playerId"].is_number_unsigned())
                                    {
                                        response.playerId = playerInfo["playerId"].get<uint64_t>();
                                    }
                                    if (playerInfo.contains("name") && playerInfo["name"].is_string())
                                    {
                                        response.username = playerInfo["name"].get<std::string>();
                                    }
                                    if (playerInfo.contains("posX") && playerInfo["posX"].is_number())
                                    {
                                        response.posX = playerInfo["posX"].get<float>();
                                    }
                                    if (playerInfo.contains("posY") && playerInfo["posY"].is_number())
                                    {
                                        response.posY = playerInfo["posY"].get<float>();
                                    }
                                    if (playerInfo.contains("hp") && playerInfo["hp"].is_number_integer())
                                    {
                                        response.hp = playerInfo["hp"].get<int>();
                                    }
                                    if (playerInfo.contains("maxHp") && playerInfo["maxHp"].is_number_integer())
                                    {
                                        response.maxHp = playerInfo["maxHp"].get<int>();
                                    }
                                }
                                LOG_INFO("AuthService::HttpRequestSession: AuthServer verification successful. PlayerId: {}, Username: {}", response.playerId, response.username);
                            }
                            else
                            {
                                if (json_response.contains("errorMessage") && json_response["errorMessage"].is_string())
                                {
                                    response.errorMessage = json_response["errorMessage"].get<std::string>();
                                }
                                if (json_response.contains("errorCode") && json_response["errorCode"].is_number_integer())
                                {
                                    response.errorCode = json_response["errorCode"].get<int>();
                                }
                                LOG_WARN("AuthService::HttpRequestSession: AuthServer verification failed. ErrorCode: {}, Message: {}", response.errorCode, response.errorMessage);
                            }
                        }
                        else
                        {
                            response.success = false;
                            response.errorCode = -6;
                            response.errorMessage = "Invalid JSON response from AuthServer: Missing 'success' field.";
                            LOG_ERROR("AuthService::HttpRequestSession: Invalid JSON response from AuthServer: Missing 'success' field.");
                        }
                    }
                    else
                    {
                        response.success = false;
                        response.errorCode = m_res.result_int();
                        response.errorMessage = "AuthServer returned HTTP error: " + std::to_string(m_res.result_int()) + " " + std::string(m_res.reason());

                        try {
                            nlohmann::json error_json = nlohmann::json::parse(m_res.body());
                            if (error_json.contains("message") && error_json["message"].is_string()) {
                                response.errorMessage += " - " + error_json["message"].get<std::string>();
                            }
                        } catch (const nlohmann::json::parse_error& e) {
                            LOG_WARN("AuthService::HttpRequestSession: Failed to parse error details from AuthServer response body: {}", e.what());
                        }
                        LOG_ERROR("AuthService::HttpRequestSession: AuthServer returned non-200 HTTP status: {}", response.errorMessage);
                    }
                }
                catch (const nlohmann::json::parse_error& e)
                {
                    response.success = false;
                    response.errorCode = -7;
                    response.errorMessage = "Failed to parse AuthServer JSON response: " + std::string(e.what());
                    LOG_ERROR("AuthService::HttpRequestSession: JSON parse error: {}", e.what());
                }
                catch (const std::exception& e)
                {
                    response.success = false;
                    response.errorCode = -8;
                    response.errorMessage = "Unknown error during AuthServer response processing: " + std::string(e.what());
                    LOG_ERROR("AuthService::HttpRequestSession: Unknown error: {}", e.what());
                }

                m_callback(response);

                m_stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
                if (ec && ec != boost::system::errc::not_connected)
                {
                    LOG_ERROR("AuthService::HttpRequestSession: Shutdown error: {}", ec.message());
                }
            }
        }
    }
}