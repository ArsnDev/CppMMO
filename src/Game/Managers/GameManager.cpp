#include "GameManager.h"
#include <fstream>
#include <random>
#include <nlohmann/json.hpp>

namespace CppMMO
{
    namespace Game
    {
        namespace Managers
        {
            /**
             * @brief Constructs a GameManager, initializing core game state and configuration.
             *
             * Loads game configuration from file, sets up the world model and spatial index, and prepares the game for operation.
             */
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

            /**
             * @brief Destructor for GameManager, ensuring the game loop is stopped and resources are cleaned up.
             */
            GameManager::~GameManager()
            {
                Stop();
            }

            /**
             * @brief Starts the game loop if it is not already running.
             *
             * Sets the running flag and launches the main game loop thread at a fixed tick rate.
             * If the game loop is already active, the function returns without effect.
             */
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

            /**
             * @brief Stops the game loop and performs cleanup.
             *
             * Halts the main game loop, shuts down the game logic queue, and joins the game loop thread if it is running.
             */
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

            /**
             * @brief Loads game configuration parameters from a JSON file.
             *
             * Attempts to read gameplay parameters such as area of interest range, chat range, and move speed from "config/game_config.json". If the file cannot be opened or parsed, default values are retained and a warning or error is logged.
             */
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

            /**
             * @brief Generates a random spawn position within a predefined area.
             *
             * @return Vec3 A position with x and y coordinates randomly selected between 90.0 and 110.0, and z set to 0.
             */
            Vec3 GameManager::GetSpawnPosition() const
            {
                // Fixed spawn position (map center)
                float centerX = m_mapWidth * 0.5f;   // 100.0f (200*0.5)
                float centerY = m_mapHeight * 0.5f;  // 100.0f (200*0.5)
                
                // All players spawn at the same location
                return Vec3(centerX, centerY, 0.0f);
                
                // Original random spawn code (commented out)
                /*
                static std::random_device rd;
                static std::mt19937 gen(rd());
                float spawnRange = std::min(m_mapWidth, m_mapHeight) * 0.1f;
                std::uniform_real_distribution<float> disX(centerX - spawnRange, centerX + spawnRange);
                std::uniform_real_distribution<float> disY(centerY - spawnRange, centerY + spawnRange);
                return Vec3(disX(gen), disY(gen), 0.0f);
                */
            }

            /**
             * @brief Checks if a position is within the valid map boundaries.
             *
             * Determines whether the given position lies inside the 200x200 map area.
             *
             * @param position The position to validate.
             * @return true if the position is within bounds; false otherwise.
             */
            bool GameManager::IsValidPosition(const Vec3& position) const
            {
                return position.x >= 0.0f && position.x < m_mapWidth && 
                       position.y >= 0.0f && position.y < m_mapHeight;
            }

            /**
             * @brief Runs the main game loop, processing commands, updating the world, and sending snapshots at a fixed tick rate.
             *
             * The loop continues while the game is running, maintaining a consistent tick interval. On each tick, it processes pending game commands, updates the world state based on elapsed time, and sends updated world snapshots to players. Exceptions during the loop are caught and logged.
             */
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

            /**
             * @brief Processes all pending game commands from the game logic queue.
             *
             * Continuously retrieves and processes commands from the queue until it is empty or the game manager is no longer running. Each command is dispatched to the appropriate handler. Exceptions during command processing are caught and logged.
             */
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

            /**
             * @brief Updates the world state and player positions for the current tick.
             *
             * Advances the simulation by the specified delta time, updating all active players' positions based on their velocities. Ensures new positions are within map boundaries and updates the spatial index accordingly.
             *
             * @param deltaTime Time elapsed since the last update, in seconds.
             */
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

            /**
             * @brief Sends world state snapshots to all active players.
             *
             * For each active player, determines nearby players within their area of interest (AOI) and sends a snapshot containing the states of those visible players.
             */
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

            /**
             * @brief Returns the IDs of players within the area of interest (AOI) around a given position.
             *
             * Queries the spatial QuadTree to find all players located within the configured AOI range of the specified position.
             *
             * @param position The center position to search around.
             * @return std::vector<uint64_t> List of player IDs within AOI range.
             */
            std::vector<uint64_t> GameManager::GetPlayersInAOI(const Vec3& position)
            {
                return m_quadTree->Query(position, m_aoiRange);
            }

            /**
             * @brief Processes a game command by dispatching it to the appropriate handler based on its payload type.
             *
             * Validates the sender's session and routes the command to the corresponding handler for player input, zone entry, or player disconnect events.
             */
            void GameManager::ProcessGameCommand(GameCommand command)
            {
                if (!m_sessionManager)
                {
                    LOG_ERROR("ProcessGameCommand: ISessionManager is null.");
                    return;
                }
                
                // PlayerDisconnectCommandData bypasses session validation (connection already terminated)
                if (std::holds_alternative<PlayerDisconnectCommandData>(command.payload))
                {
                    HandlePlayerDisconnect(std::get<PlayerDisconnectCommandData>(command.payload), nullptr);
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
                    else
                    {
                        LOG_WARN("ProcessGameCommand: Unhandled command type.");
                    }
                }, command.payload);
            }

            /**
             * @brief Processes a player's input command, updating their movement and input state.
             *
             * If the player exists and the input sequence number is newer than the last processed, updates the player's input flags and velocity based on the provided input.
             */
            void GameManager::HandlePlayerInput(const PlayerInputCommandData& data, std::shared_ptr<Network::ISession> session)
            {
                auto playerOpt = m_world->GetPlayer(data.playerId);
                if (!playerOpt.has_value())
                {
                    LOG_WARN("HandlePlayerInput: Player {} not found in world.", data.playerId);
                    return;
                }
                auto& player = playerOpt.value().get();
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
                LOG_INFO("Player {} input: flags={}, vel=({:.2f},{:.2f})",
                    data.playerId, data.inputFlags, velocity.x, velocity.y);
            }

            /**
             * @brief Handles a player's request to enter the game zone.
             *
             * Adds a new player to the world at a random spawn position if they are not already present, inserts them into the spatial index, sends an enter zone response to the player's session, and broadcasts the join event to other players.
             */
            void GameManager::HandleEnterZone(const EnterZoneCommandData& data, std::shared_ptr<Network::ISession> session)
            {
                auto existingPlayer = m_world->GetPlayer(data.playerId);
                if (existingPlayer.has_value())
                {
                    LOG_WARN("HandleEnterZone: Player {} already in world", data.playerId);
                    return;
                }
                
                Vec3 spawnPosition = GetSpawnPosition();
                Models::Player newPlayer(data.playerId, "Player_" + std::to_string(data.playerId), spawnPosition);
                newPlayer.SetSessionId(data.sessionId);  // Set session ID
                m_world->AddPlayer(std::move(newPlayer));

                m_quadTree->Insert(data.playerId, spawnPosition);
                SendEnterZoneResponse(data.playerId, session);
                BroadcastPlayerJoined(data.playerId);
                LOG_INFO("HandleEnterZone: Player {} entered zone at ({}, {})", data.playerId, spawnPosition.x, spawnPosition.y);
            }

            /**
             * @brief Handles player disconnection by marking the player inactive and removing them from the world.
             *
             * Marks the specified player as inactive, removes them from the spatial index, and broadcasts a player left event to other players.
             */
            void GameManager::HandlePlayerDisconnect(const PlayerDisconnectCommandData& data, std::shared_ptr<Network::ISession> session)
            {
                auto playerOpt = m_world->GetPlayer(data.playerId);
                if (!playerOpt.has_value())
                {
                    LOG_WARN("HandlePlayerDisconnect: Player {} not found in world.", data.playerId);
                    return;
                }
                auto& player = playerOpt.value().get();

                player.SetActive(false);
                m_quadTree->Remove(data.playerId);
                BroadcastPlayerLeft(data.playerId);
                
                LOG_INFO("HandlePlayerDisconnect: Player {} disconnected.", data.playerId);
            }

            /**
             * @brief Sends a world snapshot packet to specified players, including the states of visible players.
             *
             * Constructs a FlatBuffers-serialized snapshot containing the current tick, server time, and the state of each visible player, then sends it to each player in the provided list if their session is connected.
             *
             * @param playerIds List of player IDs to receive the snapshot.
             * @param visiblePlayers List of player IDs whose states are included in the snapshot.
             */
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
                    auto playerOpt = m_world->GetPlayer(playerId);
                    if (playerOpt.has_value())
                    {
                        const auto& player = playerOpt.value().get();
                        auto session = m_sessionManager->GetSession(player.GetSessionId());
                        if (session && session->IsConnected())
                        {
                            session->Send(std::span<const std::byte>(
                                reinterpret_cast<const std::byte*>(builder.GetBufferPointer()), 
                                builder.GetSize()));
                            LOG_DEBUG("Sent S_WorldSnapshot to Player {} (Session {})", playerId, player.GetSessionId());
                        }
                    }
                }
            }

            /**
             * @brief Sends a zone entry response to a player upon entering the game world.
             *
             * Constructs and sends a FlatBuffers packet containing the entering player's information and a list of nearby players to the specified session. The response includes player IDs, names, positions, and default HP values.
             *
             * @param playerId The ID of the player entering the zone.
             * @param session The network session associated with the player.
             */
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

            /**
             * @brief Converts input flags to a normalized 2D movement direction vector.
             *
             * Maps a 4-bit input flag (representing combinations of WASD keys) to a corresponding direction vector, handling diagonal and conflicting inputs.
             *
             * @param inputFlags Bitmask representing pressed movement keys (WASD).
             * @return Vec3 Normalized direction vector based on input flags; zero vector if input is conflicting or no movement.
             */
            Vec3 GameManager::InputFlagsToDirection(uint8_t inputFlags) const
            {
                static constexpr Vec3 DIRECTION_TABLE[16] = {
                    {0.0f, 0.0f, 0.0f},                    // 0000: None
                    {0.0f, 1.0f, 0.0f},                    // 0001: W (Up)
                    {0.0f, -1.0f, 0.0f},                   // 0010: S (Down)
                    {0.0f, 0.0f, 0.0f},                    // 0011: W+S (Cancel out)
                    {-1.0f, 0.0f, 0.0f},                   // 0100: A (Left)
                    {-0.7071067f, 0.7071067f, 0.0f},       // 0101: W+A (Up-Left)
                    {-0.7071067f, -0.7071067f, 0.0f},      // 0110: S+A (Down-Left)
                    {-1.0f, 0.0f, 0.0f},                   // 0111: W+S+A (A only)
                    {1.0f, 0.0f, 0.0f},                    // 1000: D (Right)
                    {0.7071067f, 0.7071067f, 0.0f},        // 1001: W+D (Up-Right)
                    {0.7071067f, -0.7071067f, 0.0f},       // 1010: S+D (Down-Right)
                    {1.0f, 0.0f, 0.0f},                    // 1011: W+S+D (D only)
                    {0.0f, 0.0f, 0.0f},                    // 1100: A+D (Cancel out)
                    {0.0f, 1.0f, 0.0f},                    // 1101: W+A+D (W only)
                    {0.0f, -1.0f, 0.0f},                   // 1110: S+A+D (S only)
                    {0.0f, 0.0f, 0.0f}                     // 1111: All directions (Cancel out)
                };
                
                return DIRECTION_TABLE[inputFlags & 0x0F];
            }

            /**
             * @brief Broadcasts a player joined event to all other active players.
             *
             * Sends a notification containing the joining player's information to all connected and active player sessions except the joining player.
             *
             * @param playerId The ID of the player who has joined.
             */
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
                        auto session = m_sessionManager->GetSession(otherPlayer.GetSessionId());
                        if (session && session->IsConnected()) {
                            session->Send(std::span<const std::byte>(
                                reinterpret_cast<const std::byte*>(builder.GetBufferPointer()), 
                                builder.GetSize()));
                        }
                    }
                }

                LOG_INFO("BroadcastPlayerJoined: Player {} joined, notified others", playerId);
            }

            /**
             * @brief Broadcasts a player left event to all other active players.
             *
             * Sends a notification packet to all connected sessions, except the player who left, informing them that the specified player has left the game.
             *
             * @param playerId The ID of the player who has left.
             */
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
                        auto session = m_sessionManager->GetSession(otherPlayer.GetSessionId());
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