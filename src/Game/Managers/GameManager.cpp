#include "GameManager.h"
#include <fstream>
#include <nlohmann/json.hpp>

namespace CppMMO
{
    namespace Game
    {
        namespace Managers
        {
            GameManager::GameManager(std::shared_ptr<GameLogicQueue> gameLogicQueue,
                                   std::shared_ptr<Network::ISessionManager> sessionManager)
                : m_gameLogicQueue(gameLogicQueue),
                  m_sessionManager(sessionManager)
            {
                LoadGameConfig();
                
                m_world = std::make_unique<Models::World>();
                m_quadTree = std::make_unique<Spatial::QuadTree>(0.0f, 0.0f, m_mapWidth, m_mapHeight);
                
                LOG_INFO("GameManager initialized with {} TPS, AOI range: {}, Map size: {}x{}", 
                        m_tickRate, m_aoiRange, m_mapWidth, m_mapHeight);
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
                LOG_INFO("GameManager started with 60 TPS game loop.");
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
                
                LOG_INFO("GameManager stopped.");
            }

            void GameManager::LoadGameConfig()
            {
                try
                {
                    std::ifstream file("config/game_config.json");
                    if (!file.is_open())
                    {
                        LOG_WARN("Could not open game_config.json, using default values");
                        return;
                    }
                    
                    nlohmann::json config;
                    file >> config;
                    
                    m_aoiRange = config["gameplay"]["aoi_range"].get<float>();
                    m_chatRange = config["gameplay"]["chat_range"].get<float>();
                    m_moveSpeed = config["gameplay"]["move_speed"].get<float>();
                    m_tickRate = config["gameplay"]["tick_rate"].get<int>();
                    m_tickDuration = std::chrono::milliseconds(1000 / m_tickRate);
                    
                    m_mapWidth = config["map"]["width"].get<float>();
                    m_mapHeight = config["map"]["height"].get<float>();
                    
                    LOG_INFO("Game config loaded - AOI: {}, Chat: {}, Speed: {}, TickRate: {}, Map: {}x{}", 
                            m_aoiRange, m_chatRange, m_moveSpeed, m_tickRate, m_mapWidth, m_mapHeight);
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR("Failed to load game config: {}", e.what());
                }
            }

            Vec3 GameManager::GetSpawnPosition() const
            {
                static std::random_device rd;
                static std::mt19937 gen(rd());
                
                // 맵 중앙 근처에서 스폰 (20% 범위)
                float centerX = m_mapWidth * 0.5f;
                float centerY = m_mapHeight * 0.5f;
                float spawnRange = std::min(m_mapWidth, m_mapHeight) * 0.1f;
                
                std::uniform_real_distribution<float> disX(centerX - spawnRange, centerX + spawnRange);
                std::uniform_real_distribution<float> disY(centerY - spawnRange, centerY + spawnRange);
                
                return Vec3{disX(gen), disY(gen), 0.0f};
            }

            bool GameManager::IsValidPosition(const Vec3& position) const
            {
                return position.x >= 0.0f && position.x < m_mapWidth && 
                       position.y >= 0.0f && position.y < m_mapHeight;
            }

            void GameManager::GameLoop()
            {
                auto lastTickTime = std::chrono::steady_clock::now();
                
                while (m_running.load(std::memory_order_acquire))
                {
                    auto currentTime = std::chrono::steady_clock::now();
                    auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTickTime);
                    
                    if (deltaTime >= m_tickDuration)
                    {
                        try
                        {
                            float deltaSeconds = deltaTime.count() / 1000.0f;
                            
                            ProcessPendingCommands();
                            UpdateWorld(deltaSeconds);
                            SendWorldSnapshots();
                            
                            lastTickTime = currentTime;
                        }
                        catch (const std::exception& e)
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
                    catch (const std::exception& e)
                    {
                        LOG_ERROR("Exception processing game command: {}", e.what());
                    }
                }
            }

            void GameManager::UpdateWorld(float deltaTime)
            {
                m_world->Update(deltaTime);
                
                for (auto& [playerId, player] : m_world->GetAllPlayers())
                {
                    if (player.IsActive())
                    {
                        Vec3 newPos = player.GetPosition() + player.GetVelocity() * deltaTime;
                        if(IsValidPosition(newPos))
                        {
                            player.SetPosition(newPos);
                        }
                        m_quadTree->Update(playerId, player.GetPosition());
                    }
                }
            }

            void GameManager::SendWorldSnapshots()
            {
                for (const auto& [playerId, player] : m_world->GetAllPlayers())
                {
                    if (player.IsActive())
                    {
                        auto visiblePlayers = GetPlayersInAOI(player.GetPosition());
                        SendSnapshotToPlayers({playerId}, visiblePlayers);
                    }
                }
            }

            std::vector<uint64_t> GameManager::GetPlayersInAOI(const Vec3& position)
            {
                return m_quadTree->Query(position, m_aoiRange);
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
                    LOG_WARN("ProcessGameCommand: Session {} not found.", command.senderSessionId);
                    return;
                }
                
                std::visit([this, &session](auto&& arg)
                {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, PlayerInputCommandData>)
                    {
                        HandlePlayerInput(arg, session);
                    }
                    else if constexpr (std::is_same_v<T, EnterZoneCommandData>)
                    {
                        HandleEnterZone(arg, session);
                    }
                    else if constexpr (std::is_same_v<T, PlayerDisconnectCommandData>)
                    {
                        HandlePlayerDisconnect(arg, session);
                    }
                    else
                    {
                        LOG_WARN("ProcessGameCommand: Unhandled command type.");
                    }
                }, command.payload);
            }

            void GameManager::HandlePlayerInput(const PlayerInputCommandData& data, std::shared_ptr<Network::ISession> session)
            {
                auto playerOpt = m_world->GetPlayer(data.playerId);
                if (!playerOpt.has_value())
                {
                    LOG_WARN("HandlePlayerInput: Player {} not found in world.", data.playerId);
                    return;
                }
                Models::Player& player = playerOpt.value().get();
                // TODO : Check Sequence Num
                if (data.sequenceNumber <= player.GetLastInputSequence())
                {
                    LOG_DEBUG("Ignoring old/duplicate input: seq {} <= last {}", data.sequenceNumber, player.GetLastInputSequence());
                    return;
                }
                player.SetLastInputSequence(data.sequenceNumber);
                player.SetCurrentInputFlags(data.inputFlags);
                const Vec3& direction = InputFlagsToDirection(data.inputFlags);
                Vec3 velocity = direction*m_moveSpeed;
                player.SetVelocity(velocity);
                LOG_DEBUG("Player {} input: flags={}, vel=({:.2f},{:.2f})",
                    data.playerId, data.inputFlags, velocity.x, velocity.y);
            }

            void GameManager::HandleEnterZone(const EnterZoneCommandData& data, std::shared_ptr<Network::ISession> session)
            {
                auto existingPlayer = m_world->GetPlayer(data.playerId);
                if (existingPlayer.has_value())
                {
                    LOG_WARN("HandleEnterZone: Player {} already in world", data.playerId);
                    return;
                }
                
                Vec3 spawnPosition = GetSpawnPosition();
                Models::Player newPlayer(data.playerId, spawnPosition);
                m_world->AddPlayer(std::move(newPlayer));

                m_quadTree->Insert(data.playerId, spawnPosition);
                SendEnterZoneResponse(data.playerId, session);
                BroadcastPlayerJoined(data.playerId);
                LOG_INFO("HandleEnterZone: Player {} entered zone at ({}, {})", data.playerId, spawnPosition.x, spawnPosition.y);
            }

            void GameManager::HandlePlayerDisconnect(const PlayerDisconnectCommandData& data, std::shared_ptr<Network::ISession> session)
            {
                auto playerOpt = m_world->GetPlayer(data.playerId);
                if (!playerOpt.has_value())
                {
                    LOG_WARN("HandlePlayerDisconnect: Player {} not found in world.", data.playerId);
                    return;
                }
                Models::Player& player = playerOpt.value().get();

                player.SetActive(false);
                m_quadTree->Remove(data.playerId);
                BroadcastPlayerLeft(data.playerId);
                
                LOG_INFO("HandlePlayerDisconnect: Player {} disconnected.", data.playerId);
            }

            void GameManager::SendSnapshotToPlayers(const std::vector<uint64_t>& playerIds, const std::vector<uint64_t>& visiblePlayers)
            {
                flatbuffers::FlatBufferBuilder builder;

                std::vector<flatbuffers::Offset<Protocol::PlayerState>> playerStates;
                for (uint64_t playerId : visiblePlayers)
                {
                    auto playerOpt = m_world->GetPlayer(playerId);
                    if (playerOpt.has_value())
                    {
                        const auto& player = playerOpt.value().get();
                        auto pos = Protocol::CreateVec3(builder, player.GetPosition().x, player.GetPosition().y, player.GetPosition().z);
                        
                        auto vel = Protocol::CreateVec3(builder, player.GetVelocity().x, player.GetVelocity().y, player.GetVelocity().z);
                        
                        auto playerState = Protocol::CreatePlayerState(builder, playerId, pos, vel, player.IsActive());
                        playerStates.push_back(playerState);
                    }
                }

                auto playerStatesVector = builder.CreateVector(playerStates);
                auto eventsVector = builder.CreateVector<flatbuffers::Offset<Protocol::GameEvent>>({});

                m_tickNumber++;
                uint64_t serverTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();

                auto snapshot = Protocol::CreateS_WorldSnapshot(builder,
                    m_tickNumber,
                    serverTime,
                    playerStatesVector,
                    eventsVector);
                
                auto unifiedPacket = Protocol::CreateUnifiedPacket(builder, 
                    Protocol::PacketId_S_WorldSnapshot, 
                    Protocol::Packet_S_WorldSnapshot, 
                    snapshot.Union());

                builder.Finish(unifiedPacket);
                
                for (uint64_t playerId : playerIds)
                {
                    auto session = m_sessionManager->GetSession(playerId);
                    if (session && session->IsConnected())
                    {
                        session->Send(std::span<const std::byte>(
                            reinterpret_cast<const std::byte*>(builder.GetBufferPointer()), 
                            builder.GetSize()));
                    }
                }
            }

            void GameManager::SendEnterZoneResponse(uint64_t playerId, std::shared_ptr<Network::ISession> session)
            {
                flatbuffers::FlatBufferBuilder builder;

                auto playerOpt = m_world->GetPlayer(playerId);
                if (!playerOpt.has_value())
                {
                    LOG_WARN("SendEnterZoneResponse: Player {} not found in world.", playerId);
                    return;
                }
                const auto& player = playerOpt.value().get();

                auto pos = Protocol::CreateVec3(builder, player.GetPosition().x, player.GetPosition().y, player.GetPosition().z);
                auto playerName = builder.CreateString("Player_" + std::to_string(playerId));
                auto playerInfo = Protocol::CreatePlayerInfo(builder, playerId, playerName, pos, 100, 100);
                
                auto nearPlayers = GetPlayersInAOI(player.GetPosition());
                std::vector<flatbuffers::Offset<Protocol::PlayerInfo>> nearPlayerInfos;

                for (uint64_t nearPlayerId : nearPlayers)
                {
                    if (nearPlayerId != playerId)
                    {
                        auto nearPlayerOpt = m_world->GetPlayer(nearPlayerId);
                        if (nearPlayerOpt.has_value())
                        {
                            const auto& nearPlayer = nearPlayerOpt.value().get();
                            auto nearPos = Protocol::CreateVec3(builder, nearPlayer.GetPosition().x, nearPlayer.GetPosition().y, nearPlayer.GetPosition().z);
                            auto nearPlayerName = builder.CreateString("Player_" + std::to_string(nearPlayerId));
                            auto nearPlayerInfo = Protocol::CreatePlayerInfo(builder, nearPlayerId, nearPlayerName, nearPos, 100, 100);
                            nearPlayerInfos.push_back(nearPlayerInfo);
                        }
                    }
                }

                auto nearPlayersVector = builder.CreateVector(nearPlayerInfos);
                auto zoneEntered = Protocol::CreateS_ZoneEntered(builder, 1, playerInfo, nearPlayersVector); //zoneId = 1

                auto unifiedPacket = Protocol::CreateUnifiedPacket(builder, Protocol::PacketId_S_ZoneEntered, Protocol::Packet_S_ZoneEntered, zoneEntered.Union());
                builder.Finish(unifiedPacket);

                if (session && session->IsConnected())
                {
                    session->Send(std::span<const std::byte>(reinterpret_cast<const std::byte*>(builder.GetBufferPointer()), builder.GetSize()));
                    LOG_INFO("SendEnterZoneResponse: Sent to player {}", playerId);
                }
            }

            Vec3 GameManager::InputFlagsToDirection(uint8_t inputFlags) const
            {
                static constexpr Vec3 DIRECTION_TABLE[16] = {
                    {0.0f, 0.0f, 0.0f},                    // 0000: 없음
                    {0.0f, -1.0f, 0.0f},                   // 0001: W
                    {0.0f, 1.0f, 0.0f},                    // 0010: S  
                    {0.0f, 0.0f, 0.0f},                    // 0011: W+S (상쇄)
                    {-1.0f, 0.0f, 0.0f},                   // 0100: A
                    {-0.7071067f, -0.7071067f, 0.0f},      // 0101: W+A
                    {-0.7071067f, 0.7071067f, 0.0f},       // 0110: S+A
                    {-1.0f, 0.0f, 0.0f},                   // 0111: W+S+A (A만)
                    {1.0f, 0.0f, 0.0f},                    // 1000: D
                    {0.7071067f, -0.7071067f, 0.0f},       // 1001: W+D
                    {0.7071067f, 0.7071067f, 0.0f},        // 1010: S+D
                    {1.0f, 0.0f, 0.0f},                    // 1011: W+S+D (D만)
                    {0.0f, 0.0f, 0.0f},                    // 1100: A+D (상쇄)
                    {0.0f, -1.0f, 0.0f},                   // 1101: W+A+D (W만)
                    {0.0f, 1.0f, 0.0f},                    // 1110: S+A+D (S만)
                    {0.0f, 0.0f, 0.0f}                     // 1111: 모든 방향 (상쇄)
                };
                
                return DIRECTION_TABLE[inputFlags & 0x0F];
            }

            void GameManager::BroadcastPlayerJoined(uint64_t playerId)
            {
                auto playerOpt = m_world->GetPlayer(playerId);
                if (!playerOpt.has_value()) {
                    LOG_ERROR("BroadcastPlayerJoined: Player {} not found", playerId);
                    return;
                }

                const auto& player = playerOpt.value().get();
                flatbuffers::FlatBufferBuilder builder;

                auto pos = Protocol::CreateVec3(builder, player.GetPosition().x, player.GetPosition().y, player.GetPosition().z);
                auto playerName = builder.CreateString("Player_" + std::to_string(playerId));
                auto playerInfo = Protocol::CreatePlayerInfo(builder, playerId, playerName, pos, player.GetHp(), player.GetMaxHp());

                auto playerJoined = Protocol::CreateS_PlayerJoined(builder, playerInfo);
                auto unifiedPacket = Protocol::CreateUnifiedPacket(builder, 
                    Protocol::PacketId_S_PlayerJoined, 
                    Protocol::Packet_S_PlayerJoined, 
                    playerJoined.Union());

                builder.Finish(unifiedPacket);

                for (const auto& [otherPlayerId, otherPlayer] : m_world->GetAllPlayers()) {
                    if (otherPlayerId != playerId && otherPlayer.IsActive()) {
                        auto session = m_sessionManager->GetSession(otherPlayerId);
                        if (session && session->IsConnected()) {
                            session->Send(std::span<const std::byte>(
                                reinterpret_cast<const std::byte*>(builder.GetBufferPointer()), 
                                builder.GetSize()));
                        }
                    }
                }

                LOG_INFO("BroadcastPlayerJoined: Player {} joined, notified others", playerId);
            }

            void GameManager::BroadcastPlayerLeft(uint64_t playerId)
            {
                flatbuffers::FlatBufferBuilder builder;

                auto playerLeft = Protocol::CreateS_PlayerLeft(builder, playerId);
                auto unifiedPacket = Protocol::CreateUnifiedPacket(builder, 
                    Protocol::PacketId_S_PlayerLeft, 
                    Protocol::Packet_S_PlayerLeft, 
                    playerLeft.Union());

                builder.Finish(unifiedPacket);

                for (const auto& [otherPlayerId, otherPlayer] : m_world->GetAllPlayers()) {
                    if (otherPlayerId != playerId && otherPlayer.IsActive()) {
                        auto session = m_sessionManager->GetSession(otherPlayerId);
                        if (session && session->IsConnected()) {
                            session->Send(std::span<const std::byte>(
                                reinterpret_cast<const std::byte*>(builder.GetBufferPointer()), 
                                builder.GetSize()));
                        }
                    }
                }

                LOG_INFO("BroadcastPlayerLeft: Player {} left, notified others", playerId);
            }
        }
    }
}