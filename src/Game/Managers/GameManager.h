#pragma once
#include "pch.h"
#include "Game/GameLogicQueue.h"
#include "Game/Models/World.h"
#include "Game/Spatial/QuadTree.h"
#include "Network/ISessionManager.h"
#include "protocol_generated.h"

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

                int m_tickRate = 60;
                std::chrono::milliseconds m_tickDuration{1000 / 60};

                float m_aoiRange = 100.0f;
                float m_chatRange = 50.0f;
                float m_moveSpeed = 5.0f;
                float m_mapWidth = 200.0f;
                float m_mapHeight = 200.0f;

                void GameLoop();
                void ProcessPendingCommands();
                void UpdateWorld(float deltaTime);
                void SendWorldSnapshots();
                void ProcessGameCommand(GameCommand command);

                void HandlePlayerInput(const PlayerInputCommandData& data, std::shared_ptr<Network::ISession> session);
                void HandleEnterZone(const EnterZoneCommandData& data, std::shared_ptr<Network::ISession> session);
                void HandlePlayerDisconnect(const PlayerDisconnectCommandData& data, std::shared_ptr<Network::ISession> session);
           
                std::vector<uint64_t> GetPlayersInAOI(const Vec3& position);
                void SendSnapshotToPlayers(const std::vector<uint64_t>& playerIds, const std::vector<uint64_t>& visiblePlayers);
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