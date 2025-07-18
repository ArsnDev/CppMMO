#include "SessionManager.h"
#include "Game/GameCommand.h"

namespace CppMMO
{
    namespace Network
    {
        SessionManager::SessionManager(std::shared_ptr<Game::GameLogicQueue> gameLogicQueue)
            : m_gameLogicQueue(gameLogicQueue)
        {
        }
        void SessionManager::AddSession(std::shared_ptr<ISession> session)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_activeSessions[session->GetSessionId()] = session;
            LOG_INFO("SessionManager: Session {} added. Total active sessions: {}", session->GetSessionId(), m_activeSessions.size());
        }

        void SessionManager::RemoveSession(uint64_t sessionId)
        {
            std::shared_ptr<ISession> session;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto it = m_activeSessions.find(sessionId);
                if (it != m_activeSessions.end())
                {
                    session = it->second;
                    m_activeSessions.erase(it);
                }
            }
            
            if (session)
            {
                OnSessionDisconnected(session);
                LOG_INFO("SessionManager: Session {} removed. Total active sessions: {}", sessionId, m_activeSessions.size());
            }
        }

        std::shared_ptr<ISession> SessionManager::GetSession(uint64_t sessionId) const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_activeSessions.find(sessionId);
            if (it != m_activeSessions.end())
            {
                return it->second;
            }
            return nullptr;
        }

        /**
         * @brief Retrieves all active network sessions.
         *
         * @return A vector containing shared pointers to all currently active sessions.
         */
        std::vector<std::shared_ptr<ISession>> SessionManager::GetAllSessions() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::vector<std::shared_ptr<ISession>> sessions;
            sessions.reserve(m_activeSessions.size());
            
            for (const auto& [sessionId, session] : m_activeSessions)
            {
                sessions.push_back(session);
            }
            
            return sessions;
        }

        /**
         * @brief Handles a session disconnection by queuing a player disconnect command.
         *
         * If the disconnected session has a valid player ID and a game logic command queue is available, constructs and enqueues a player disconnect command for processing by the game logic.
         */
        void SessionManager::OnSessionDisconnected(std::shared_ptr<ISession> session)
        {
            uint64_t playerId = session->GetPlayerId();

            if(playerId != 0 && m_gameLogicQueue)
            {
                Game::PlayerDisconnectCommandData disconnectData;
                disconnectData.playerId = playerId;

                Game::GameCommand command;
                command.payload = disconnectData;
                command.senderSessionId = session->GetSessionId();
                command.timestamp = Game::GetCurrentTimestamp();

                m_gameLogicQueue->PushCommand(command);

                LOG_INFO("SessionManager: Queued disconnect command for player {}", playerId);
            }
        }
    }
}