#include "pch.h"
#include "LoginPacketHandler.h"
#include <boost/asio/post.hpp>

namespace CppMMO
{
    namespace Game
    {
        namespace PacketHandlers
        {
            LoginPacketHandler::LoginPacketHandler(boost::asio::io_context& ioc, std::shared_ptr<CppMMO::Game::Services::AuthService> authService)
                : m_ioc(ioc), m_authService(authService)
            {
                if (!m_authService)
                {
                    LOG_ERROR("LoginPacketHandler: AuthService is null during initialization!");
                }
            }

            void LoginPacketHandler::operator()(std::shared_ptr<Network::ISession> session, const Protocol::UnifiedPacket* unifiedPacket) const
            {
                if (!session)
                {
                    LOG_ERROR("Error: Session is null in LoginPacketHandler.");
                    return;
                }

                if (unifiedPacket->id() != Protocol::PacketId_C_Login)
                {
                    LOG_ERROR("Error: Session {}: Received non-C_Login packet in LoginPacketHandler. Actual ID: {}", session->GetRemoteEndpoint().address().to_string(), static_cast<int>(unifiedPacket->id()));
                    return;
                }

                const Protocol::C_Login* c_login_packet = unifiedPacket->data_as_C_Login();
                if (!c_login_packet)
                {
                    LOG_ERROR("Error: Session {}: Failed to get C_Login data from unified packet.", session->GetRemoteEndpoint().address().to_string());
                    SendLoginFailure(session, -1, "Invalid C_Login packet data.", 0);
                    return;
                }

                const std::string sessionTicket = c_login_packet->session_ticket()->str();
                const int64_t commandId = c_login_packet->command_id();

                LOG_INFO("[LoginPacketHandler] Processing login request for session ticket: '{}' from session: {}", sessionTicket, session->GetRemoteEndpoint().address().to_string());

                if (!m_authService)
                {
                    LOG_ERROR("Error: AuthService is not initialized in LoginPacketHandler.");
                    SendLoginFailure(session, -99, "Server internal error: Auth service unavailable.", commandId);
                    return;
                }

                std::weak_ptr<Network::ISession> weakSession = session;

                m_authService->VerifySessionTicketAsync(sessionTicket,
                    [this, weakSession, commandId, sessionTicket](const CppMMO::Game::Services::VerifyTicketResponse& authResponse)
                    {
                        boost::asio::post(m_ioc, [this, weakSession, commandId, sessionTicket, authResponse]()
                        {
                            std::shared_ptr<Network::ISession> session = weakSession.lock();
                            if (!session || !session->IsConnected())
                            {
                                LOG_WARN("Session for ticket '{}' disconnected before AuthService response processed.", sessionTicket);
                                return;
                            }

                            if (authResponse.success)
                            {
                                LOG_INFO("[LoginPacketHandler] User '{}' authenticated successfully. PlayerId: {}", authResponse.username, authResponse.playerId);
                                session->SetPlayerId(authResponse.playerId);
                                flatbuffers::FlatBufferBuilder builder;
                                auto player_name_offset = builder.CreateString(authResponse.username);
                                
                                auto player_info_offset = Protocol::CreatePlayerInfo(builder,
                                                                                     authResponse.playerId,
                                                                                     player_name_offset,
                                                                                     0,
                                                                                     100,
                                                                                     100);
                                auto s_login_success_offset = Protocol::CreateS_LoginSuccess(builder, player_info_offset, commandId);
                                auto unified_packet_offset = Protocol::CreateUnifiedPacket(builder, 
                                                                                           Protocol::PacketId_S_LoginSuccess, 
                                                                                           Protocol::Packet_S_LoginSuccess, 
                                                                                           s_login_success_offset.Union());
                                builder.Finish(unified_packet_offset);
                                session->Send(std::span<const std::byte>(reinterpret_cast<const std::byte*>(builder.GetBufferPointer()), builder.GetSize()));
                                LOG_INFO("--- LoginPacketHandler: Sent S_LoginSuccess ---");
                            }
                            else
                            {
                                LOG_ERROR("Error: Session {}: Authentication failed for ticket: '{}'. Reason: {}", session->GetRemoteEndpoint().address().to_string(), sessionTicket, authResponse.errorMessage);
                                SendLoginFailure(session, authResponse.errorCode, authResponse.errorMessage, commandId);
                            }
                        });
                    });
                LOG_INFO("--- LoginPacketHandler: Initiated AuthService verification for session ticket '{}' ---", sessionTicket);
            }

            void LoginPacketHandler::SendLoginFailure(std::shared_ptr<Network::ISession> session, int errorCode, const std::string& errorMessage, int64_t commandId) const
            {
                flatbuffers::FlatBufferBuilder builder;
                auto error_message_offset = builder.CreateString(errorMessage);
                auto s_login_failure_offset = Protocol::CreateS_LoginFailure(builder, errorCode, error_message_offset, commandId);
                
                auto unified_packet_offset = Protocol::CreateUnifiedPacket(builder, 
                                                                           Protocol::PacketId_S_LoginFailure, 
                                                                           Protocol::Packet_S_LoginFailure, 
                                                                           s_login_failure_offset.Union());
                builder.Finish(unified_packet_offset);

                session->Send(std::span<const std::byte>(reinterpret_cast<const std::byte*>(builder.GetBufferPointer()), builder.GetSize()));
            }
        }
    }
}