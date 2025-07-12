#include "pch.h"
#include "LoginPacketHandler.h"

namespace CppMMO
{
    namespace Game
    {
        namespace PacketHandlers
        {
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
                const std::string username = "test_user";
                LOG_INFO("[LoginPacketHandler] Processing login for user: '{}' from session: {}", username, session->GetRemoteEndpoint().address().to_string());

                // TODO: Replace with real authentication logic (e.g., database lookup)
                bool isAuthenticated = (username == "test_user");
                uint64_t retrievedPlayerId = 0; // 실제 인증 시스템에서 가져올 플레이어 ID

                flatbuffers::FlatBufferBuilder builder;
                flatbuffers::Offset<void> s_login_offset;

                if (isAuthenticated)
                {
                    // 임시로 하드코딩된 ID 사용. 실제로는 인증 시스템에서 가져와야 함.
                    retrievedPlayerId = 12345L;
                    LOG_INFO("[LoginPacketHandler] User '{}' authenticated successfully. PlayerId: {}", username, retrievedPlayerId);

                    // 세션에 플레이어 ID 설정
                    session->SetPlayerId(retrievedPlayerId);

                    // TODO: Replace with actual player's last known position or spawn point
                    auto position = Protocol::Vec2(10.0f, 20.0f); // 현재는 하드코딩된 위치

                    auto player_info_offset = Protocol::CreatePlayerInfo(builder,
                                                                        retrievedPlayerId, // 실제 플레이어 ID 사용
                                                                        builder.CreateString(username),
                                                                        &position,
                                                                        100,
                                                                        100);
                    auto s_login_success = Protocol::CreateS_Login(builder, true, player_info_offset);
                    s_login_offset = s_login_success.Union();
                }
                else
                {
                    LOG_ERROR("Error: Session {}: Authentication failed for user: '{}'", session->GetRemoteEndpoint().address().to_string(), username);
                    auto s_login_failure = Protocol::CreateS_Login(builder, false);
                    s_login_offset = s_login_failure.Union();
                }

                auto unified_packet_offset = Protocol::CreateUnifiedPacket(builder, Protocol::PacketId_S_Login, Protocol::Packet_S_Login, s_login_offset);
                builder.Finish(unified_packet_offset);

                session->Send(std::span<const std::byte>(reinterpret_cast<const std::byte*>(builder.GetBufferPointer()), builder.GetSize()));
                LOG_INFO("--- LoginPacketHandler: Finished processing for user '{}' ---", username);
            }
        }
    }
}
