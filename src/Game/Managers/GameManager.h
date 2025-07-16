#pragma once
#include "pch.h"
#include "Game/GameLogicQueue.h"
#include "Network/ISessionManager.h"
#include "protocol_generated.h"

namespace CppMMO
{
    namespace Game
    {
        namespace Managers
        {
            struct PlayerState {
                int64_t playerId;
                float posX, posY;  // Vec2 대신 원시 값 저장
                uint8_t inputFlags;
                float moveSpeed;
                std::chrono::milliseconds lastUpdateTime;
                bool needsBroadcast = false;  // 브로드캐스트 필요 여부
            };

            class GameManager
            {
            public:
                GameManager(std::shared_ptr<GameLogicQueue> gameLogicQueue,
                            std::shared_ptr<Network::ISessionManager> sessionManager);
                ~GameManager();
                void Start();
                void Stop();
            private:
                std::shared_ptr<GameLogicQueue> m_gameLogicQueue;
                std::shared_ptr<Network::ISessionManager> m_sessionManager;

                std::thread m_gameLoopThread{};
                std::atomic<bool> m_running = false;
                
                static constexpr int TICK_RATE = 20; // 20 TPS
                static constexpr std::chrono::milliseconds TICK_DURATION{1000 / TICK_RATE};
                
                std::unordered_map<int64_t, PlayerState> m_playerStates;
                std::mutex m_playerStatesMutex;

                void GameLoop();
                void ProcessPendingCommands();
                void UpdatePlayerPositions();
                void BroadcastPlayerStates();
                void ProcessGameCommand(GameCommand command);

                void HandleMoveCommand(const MoveCommandData& data, int64_t originalCommandId, std::shared_ptr<Network::ISession> session);
            void HandleChangeZoneCommand(const ChangeZoneCommandData& data, int64_t originalCommandId, std::shared_ptr<Network::ISession> session);
            void HandlePlayerHpUpdateCommand(const PlayerHpUpdateCommandData& data, int64_t originalCommandId, std::shared_ptr<Network::ISession> session);
            };
        }
    }
}