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

                std::thread m_gameLoopThread;
                std::atomic<bool> m_running = false;

                void GameLoop();
                void ProcessGameCommand(GameCommand command);

                void HandleMoveCommand(const MoveCommandData& data, int64_t originalCommandId, std::shared_ptr<Network::ISession> session);
            void HandleChangeZoneCommand(const ChangeZoneCommandData& data, int64_t originalCommandId, std::shared_ptr<Network::ISession> session);
            void HandlePlayerHpUpdateCommand(const PlayerHpUpdateCommandData& data, int64_t originalCommandId, std::shared_ptr<Network::ISession> session);
            };
        }
    }
}