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
                auto lastTickTime = std::chrono::steady_clock::now();
                
                while (m_running.load(std::memory_order_acquire))
                {
                    auto currentTime = std::chrono::steady_clock::now();
                    auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTickTime);
                    
                    if (deltaTime >= TICK_DURATION)
                    {
                        try
                        {
                            ProcessPendingCommands();
                            UpdatePlayerPositions();
                            BroadcastPlayerStates();
                            
                            lastTickTime = currentTime;
                        }
                        catch(const std::exception& e)
                        {
                            LOG_ERROR("Exception in GameManager game loop: {}", e.what());
                        }
                    }
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
            void GameManager::ProcessPendingCommands()
            {
                while (true)
                {
                    auto optCommand = m_gameLogicQueue->TryPopGameCommand();
                    if (!optCommand.has_value())
                    {
                        break;
                    }
                    
                    if (!m_running.load(std::memory_order_acquire))
                    {
                        break;
                    }
                    
                    try
                    {
                        ProcessGameCommand(optCommand.value());
                    }
                    catch(const std::exception& e)
                    {
                        LOG_ERROR("Exception processing game command: {}", e.what());
                    }
                }
            }
            
            void GameManager::UpdatePlayerPositions()
            {
                std::lock_guard<std::mutex> lock(m_playerStatesMutex);
                auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
                
                for (auto& [playerId, playerState] : m_playerStates)
                {
                    if (Game::IsMoving(playerState.inputFlags))
                    {
                        auto deltaTime = currentTime - playerState.lastUpdateTime;
                        float deltaSeconds = deltaTime.count() / 1000.0f;
                        
                        auto direction = Game::InputFlagsToDirection(playerState.inputFlags);
                        float newX = playerState.posX + 
                            direction.x * playerState.moveSpeed * deltaSeconds;
                        float newY = playerState.posY + 
                            direction.y * playerState.moveSpeed * deltaSeconds;
                        
                        playerState.posX = newX;
                        playerState.posY = newY;
                        playerState.lastUpdateTime = currentTime;
                    }
                }
            }
            
            void GameManager::BroadcastPlayerStates()
            {
                std::lock_guard<std::mutex> lock(m_playerStatesMutex);
                
                for (auto& [playerId, playerState] : m_playerStates)
                {
                    // 브로드캐스트가 필요한 플레이어만 처리
                    if (!playerState.needsBroadcast)
                    {
                        continue;
                    }
                    
                    flatbuffers::FlatBufferBuilder builder;
                    
                    auto position = Protocol::CreateVec2(builder, playerState.posX, playerState.posY);
                    auto s_player_move_state_packet = Protocol::CreateS_PlayerMoveState(builder, 
                        playerState.playerId, 
                        position, 
                        playerState.inputFlags,
                        playerState.moveSpeed, 
                        playerState.lastUpdateTime.count(), 
                        0);
                        
                    auto unified_packet_offset = Protocol::CreateUnifiedPacket(builder, 
                        Protocol::PacketId_S_PlayerMoveState, 
                        Protocol::Packet_S_PlayerMoveState, 
                        s_player_move_state_packet.Union());
                    builder.Finish(unified_packet_offset);
                    
                    auto sessions = m_sessionManager->GetAllSessions();
                    for (auto& session : sessions)
                    {
                        if (session && session->IsConnected())
                        {
                            session->Send(std::span<const std::byte>(
                                reinterpret_cast<const std::byte*>(builder.GetBufferPointer()), 
                                builder.GetSize()));
                        }
                    }
                    
                    // 브로드캐스트 완료 후 플래그 리셋
                    playerState.needsBroadcast = false;
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
                auto direction = Game::InputFlagsToDirection(data.inputFlags);
                LOG_DEBUG("HandleMoveCommand: Player {} at ({}, {}) with flags {} direction ({}, {}) moving: {}. SessionId: {}", 
                         data.playerId, data.currentPosition.x, data.currentPosition.y, 
                         static_cast<int>(data.inputFlags), direction.x, direction.y, 
                         Game::IsMoving(data.inputFlags), session->GetSessionId());
                
                // Update player state
                {
                    std::lock_guard<std::mutex> lock(m_playerStatesMutex);
                    PlayerState& playerState = m_playerStates[data.playerId];
                    playerState.playerId = data.playerId;
                    playerState.posX = data.currentPosition.x;
                    playerState.posY = data.currentPosition.y;
                    playerState.inputFlags = data.inputFlags;
                    playerState.moveSpeed = Game::IsMoving(data.inputFlags) ? 5.0f : 0.0f;
                    playerState.lastUpdateTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch());
                    playerState.needsBroadcast = true;  // 상태 변경 시 브로드캐스트 마킹
                }
                
                // Send immediate response to the player who moved
                flatbuffers::FlatBufferBuilder builder;
                auto fb_current_pos = Protocol::CreateVec2(builder, data.currentPosition.x, data.currentPosition.y);
                
                auto s_player_move_state_packet = Protocol::CreateS_PlayerMoveState(builder, 
                    static_cast<int64_t>(data.playerId), 
                    fb_current_pos, 
                    data.inputFlags,
                    Game::IsMoving(data.inputFlags) ? 5.0f : 0.0f, 
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(), 
                    originalCommandId);
                    
                auto unified_packet_offset = Protocol::CreateUnifiedPacket(builder, Protocol::PacketId_S_PlayerMoveState, Protocol::Packet_S_PlayerMoveState, s_player_move_state_packet.Union());
                builder.Finish(unified_packet_offset);
                
                if (session && session->IsConnected())
                {
                    session->Send(std::span<const std::byte>(reinterpret_cast<const std::byte*>(builder.GetBufferPointer()), builder.GetSize()));
                    LOG_DEBUG("Sent S_PlayerMoveState to session {} for player {}.", session->GetSessionId(), data.playerId);
                }
                else
                {
                    LOG_WARN("HandleMoveCommand: Session {} is not connected or null. Cannot send S_PlayerMoveState.", session->GetSessionId());
                }
            }

            void GameManager::HandleChangeZoneCommand(const ChangeZoneCommandData& data, [[maybe_unused]] int64_t originalCommandId, std::shared_ptr<Network::ISession> session)
            {
                LOG_DEBUG("HandleChangeZoneCommand: Player {} changing to zone {}. SessionId: {}", data.playerId, data.targetZoneId, session->GetSessionId());
                flatbuffers::FlatBufferBuilder builder;

                // Create PlayerInfo for the player entering the zone.
                // In a real scenario, you'd fetch the player's actual name and stats.
                const auto pos = Protocol::CreateVec2(builder, 0.0f, 0.0f);
                auto player_info_offset = Protocol::CreatePlayerInfo(builder, data.playerId, builder.CreateString("PlayerName"), pos, 100, 100);

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

             void GameManager::HandlePlayerHpUpdateCommand(const PlayerHpUpdateCommandData& data, [[maybe_unused]] int64_t originalCommandId, std::shared_ptr<Network::ISession> session)
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