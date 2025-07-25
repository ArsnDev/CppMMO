#pragma once
#include "pch.h"
#include "Game/GameLogicQueue.h"
#include "Game/Models/World.h"
#include "Game/Models/Player.h"
#include "Game/Spatial/QuadTree.h"
#include "Network/ISessionManager.h"
#include "protocol_generated.h"

/**
 * Manages core game logic, player sessions, and world state for a multiplayer online game.
 *
 * The GameManager coordinates the main game loop, processes player commands, updates the game world, manages player visibility and communication, and interacts with network sessions. It uses spatial partitioning for efficient queries and maintains gameplay parameters such as area of interest, chat range, and movement speed.
 */
namespace CppMMO
{
    namespace Game
    {
        namespace Managers
        {
            class GameManager
            {
            public:
                GameManager(std::shared_ptr<GameLogicQueue> gameLogicQueue,
                           std::shared_ptr<Network::ISessionManager> sessionManager);
                ~GameManager();

                void Start();
                void Stop();

            private:
                // Core components
                std::shared_ptr<GameLogicQueue> m_gameLogicQueue;
                std::shared_ptr<Network::ISessionManager> m_sessionManager;
                std::unique_ptr<Models::World> m_world;
                std::unique_ptr<Spatial::QuadTree> m_quadTree;

                // Game loop
                std::thread m_gameLoopThread;
                std::atomic<bool> m_running = false;
                uint64_t m_tickNumber = 0;

                int m_tickRate = 30;  // 60 → 30 TPS for better performance
                std::chrono::milliseconds m_tickDuration{1000 / 30};

                float m_aoiRange = 100.0f;
                float m_chatRange = 50.0f;
                float m_moveSpeed = 5.0f;
                float m_mapWidth = 200.0f;
                float m_mapHeight = 200.0f;
                
                // Performance settings
                int m_commandBatchSize = 500;  // Optimized: 100 → 500
                int m_maxProcessingTimeMs = 10; // Time limit for command processing
                int m_aoiUpdateInterval = 3;    // Update AOI every 3 ticks instead of every tick
                float m_aoiPositionThreshold = 10.0f; // Force AOI update if player moved > 10 units

                // Tick-based batching system
                std::unordered_map<uint64_t, std::vector<std::vector<std::byte>>> m_playerBatches;
                
                // AOI caching system for performance optimization
                struct AOICache {
                    std::vector<uint64_t> visiblePlayers;
                    uint64_t lastUpdateTick = 0;
                    Vec3 lastPosition{0, 0, 0};
                };
                std::unordered_map<uint64_t, AOICache> m_aoiCache;
                
                // Performance monitoring
                struct PerformanceStats {
                    uint64_t totalCommandsProcessed = 0;
                    uint64_t totalAOIQueriesSkipped = 0;
                    uint64_t totalAOIQueriesExecuted = 0;
                    std::chrono::microseconds totalCommandProcessingTime{0};
                    std::chrono::microseconds totalWorldUpdateTime{0};
                    std::chrono::microseconds totalSnapshotTime{0};
                };
                PerformanceStats m_performanceStats;
                uint64_t m_lastStatsReportTick = 0;
                static constexpr uint64_t STATS_REPORT_INTERVAL = 300; // Report every 5 seconds at 60 TPS

                void GameLoop();
                void ProcessPendingCommands();
                void UpdateWorld(float deltaTime);
                void SendWorldSnapshots();
                void ProcessGameCommand(GameCommand command);

                // Tick-based batching methods
                void AddToPlayerBatch(uint64_t playerId, std::span<const std::byte> packetData);
                void AddSnapshotToPlayerBatch(uint64_t playerId, const std::vector<uint64_t>& visiblePlayers, uint64_t serverTime);
                void FlushAllBatches();

                void HandlePlayerInput(const PlayerInputCommandData& data, std::shared_ptr<Network::ISession> session);
                void HandleEnterZone(const EnterZoneCommandData& data, std::shared_ptr<Network::ISession> session);
                void HandlePlayerDisconnect(const PlayerDisconnectCommandData& data, std::shared_ptr<Network::ISession> session);
           
                std::vector<uint64_t> GetPlayersInAOI(const Vec3& position);
                std::vector<uint64_t> GetCachedPlayersInAOI(uint64_t playerId, const Vec3& position);
                bool ShouldUpdateAOI(uint64_t playerId, const Vec3& currentPosition) const;
                void UpdateAOICache(uint64_t playerId, const Vec3& position, const std::vector<uint64_t>& visiblePlayers);
                void ReportPerformanceStats();
                void SendEnterZoneResponse(uint64_t playerId, std::shared_ptr<Network::ISession> session);
                void BroadcastPlayerJoined(uint64_t playerId);
                void BroadcastPlayerLeft(uint64_t playerId);

                void LoadGameConfig();
                Vec3 GetSpawnPosition() const;
                bool IsValidPosition(const Vec3& position) const;

                Vec3 InputFlagsToDirection(uint8_t inputFlags) const;
            };
        }
    }
}