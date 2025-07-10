#include "GameManager.h"

namespace CppMMO
{
    namespace Game
    {
        namespace Managers
        {
            GameManager::GameManager(std::shared_ptr<GameLogicQueue> gameLogicQueue,
                                     std::shared_ptr<Network::ISessionManager> sessionManager)
                                    :m_gameLogicQueue(gameLogicQueue),
                                     m_sessionManager(sessionManager)
            {
            }
            GameManager::~GameManager()
            {
                Stop();
            }
            void GameManager::Start()
            {
                if (m_running.load(std::memory_order_acquire))
                {
                    LOG_WARN("GameManager is already running.");
                    return;
                }
                m_running.store(true, std::memory_order_release);
                m_gameLoopThread = std::thread(&GameManager::GameLoop, this);
                LOG_INFO("GameManager game loop thread started.");
            }
            void GameManager::Stop()
            {
                if (!m_running.load(std::memory_order_acquire))
                {
                    return;
                }
                m_running.store(false, std::memory_order_release);
                m_gameLogicQueue->Shutdown();
                if (m_gameLoopThread.joinable())
                {
                    m_gameLoopThread.join();
                }
                LOG_INFO("GameManager stopped and game loop thread joined.");
            }
            void GameManager::GameLoop()
            {
                while (m_running.load(std::memory_order_acquire))
                {
                    GameCommand command = m_gameLogicQueue->PopGameCommand();
                    if (!m_running.load(std::memory_order_acquire))
                    {
                        LOG_INFO("GameManager game loop received shutdown signal and is exiting.");
                        break;
                    }
                    try
                    {
                        ProcessGameCommand(command);
                    }
                    catch(const std::exception& e)
                    {
                        LOG_ERROR("Exception in GameManager game loop: {}", e.what());
                    }
                }
            }
            void GameManager::ProcessGameCommand(GameCommand command)
            {
                if (!m_sessionManager)
                {
                    LOG_ERROR("ProcessGameCommand: ISessionManager is null.");
                    return;
                }
                std::shared_ptr<Network::ISession> session = m_sessionManager->GetSession(command.senderSessionId);
                if (!session)
                {
                    LOG_WARN("ProcessGameCommand: Session {} not found for command.", command.senderSessionId);
                    return;
                }
                std::visit([this, &command, &session](auto&& arg)
                {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, MoveCommandData>)
                    {
                        HandleMoveCommand(arg, command.commandId, session);
                    }
                    else if constexpr (std::is_same_v<T, ChangeZoneCommandData>)
                    {
                        HandleChangeZoneCommand(arg, command.commandId, session);
                    }
                    else if constexpr (std::is_same_v<T, PlayerHpUpdateCommandData>)
                    {
                        HandlePlayerHpUpdateCommand(arg, command.commandId, session);
                    }
                    else
                    {
                        LOG_WARN("ProcessGameCommand: Unhandled GameCommandPayload type.");
                    }
                }, command.payload);
            }
            void GameManager::HandleMoveCommand(const MoveCommandData& data, int64_t originalCommandId, std::shared_ptr<Network::ISession> session)
            {
                LOG_DEBUG("HandleMoveCommand: Entity {} moving to ({}, {}). SessionId: {}", data.entityId, data.targetPosition.x, data.targetPosition.y, session->GetSessionId());
                flatbuffers::FlatBufferBuilder builder;
                Protocol::Vec2 fb_target_pos(data.targetPosition.x, data.targetPosition.y);
                auto s_player_move_packet = Protocol::CreateS_PlayerMove(builder, static_cast<int64_t>(data.entityId), &fb_target_pos, originalCommandId);
                auto unified_packet_offset = Protocol::CreateUnifiedPacket(builder, Protocol::PacketId_S_PlayerMove, Protocol::Packet_S_PlayerMove, s_player_move_packet.Union());
                builder.Finish(unified_packet_offset);
                if (session && session->IsConnected())
                {
                    session->Send(std::span<const std::byte>(reinterpret_cast<const std::byte*>(builder.GetBufferPointer()), builder.GetSize()));
                    LOG_DEBUG("Sent S_PlayerMove to session {} for entity {}.", session->GetSessionId(), data.entityId);
                }
                else
                {
                    LOG_WARN("HandleMoveCommand: Session {} is not connected or null. Cannot send S_PlayerMove.", session->GetSessionId());
                }
                // TODO : broadcast to other players too
            }

            void GameManager::HandleChangeZoneCommand(const ChangeZoneCommandData& data, int64_t originalCommandId, std::shared_ptr<Network::ISession> session)
            {
                LOG_DEBUG("HandleChangeZoneCommand: Player {} changing to zone {}. SessionId: {}", data.playerId, data.targetZoneId, session->GetSessionId());
                flatbuffers::FlatBufferBuilder builder;

                // Create PlayerInfo for the player entering the zone.
                // In a real scenario, you'd fetch the player's actual name and stats.
                const auto pos = Protocol::Vec2(0, 0);
                auto player_info_offset = Protocol::CreatePlayerInfo(builder, data.playerId, builder.CreateString("PlayerName"), &pos, 100, 100);

                // Create an empty vector for other players, as we are not populating it yet.
                auto other_players_vector = builder.CreateVector<flatbuffers::Offset<Protocol::PlayerInfo>>({});

                // Create the S_EnterZone packet.
                // Note: originalCommandId is not part of S_EnterZone, so it's not used here.
                auto s_enter_zone_packet = Protocol::CreateS_EnterZone(builder, data.targetZoneId, player_info_offset, other_players_vector, data.playerId);
                
                auto unified_packet_offset = Protocol::CreateUnifiedPacket(builder, Protocol::PacketId_S_EnterZone, Protocol::Packet_S_EnterZone, s_enter_zone_packet.Union());
                builder.Finish(unified_packet_offset);

                if (session && session->IsConnected())
                {
                    session->Send(std::span<const std::byte>(reinterpret_cast<const std::byte*>(builder.GetBufferPointer()), builder.GetSize()));
                    LOG_DEBUG("Sent S_EnterZone to session {} for player {}.", session->GetSessionId(), data.playerId);
                }
                else
                {
                    LOG_WARN("HandleChangeZoneCommand: Session {} is not connected or null. Cannot send S_EnterZone.", session->GetSessionId());
                }
            }

             void GameManager::HandlePlayerHpUpdateCommand(const PlayerHpUpdateCommandData& data, int64_t originalCommandId, std::shared_ptr<Network::ISession> session)
             {
                LOG_DEBUG("HandlePlayerHpUpdateCommand: Player {} HP updated to {}. SessionId: {}", data.playerId, data.currentHp, session->GetSessionId());
                flatbuffers::FlatBufferBuilder builder;
                auto s_player_hp_update_packet = Protocol::CreateS_PlayerHpUpdate(builder, static_cast<int64_t>(data.playerId), data.currentHp);
                auto unified_packet_offset = Protocol::CreateUnifiedPacket(builder, Protocol::PacketId_S_PlayerHpUpdate, Protocol::Packet_S_PlayerHpUpdate, s_player_hp_update_packet.Union());
                builder.Finish(unified_packet_offset);
                if (session && session->IsConnected())
                {
                    session->Send(std::span<const std::byte>(reinterpret_cast<const std::byte*>(builder.GetBufferPointer()), builder.GetSize()));
                    LOG_DEBUG("Sent S_PlayerHpUpdate to session {} for player {}.", session->GetSessionId(), data.playerId);
                }
                else
                {
                    LOG_WARN("HandlePlayerHpUpdateCommand: Session {} is not connected or null. Cannot send S_PlayerHpUpdate.", session->GetSessionId());
                }
             }
        }
    }
}