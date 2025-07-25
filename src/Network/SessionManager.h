#pragma once
#include "pch.h"

#include "ISessionManager.h"
#include "ISession.h"

/**
 * Forward declaration of the GameLogicQueue class in the CppMMO::Game namespace.
 */
namespace CppMMO 
{ 
    namespace Game 
    { 
        class GameLogicQueue; 
    } 
}

/**
 * Handles cleanup and processing when a session is disconnected.
 * 
 * This method is invoked internally when a session is no longer active, allowing the manager to perform any necessary cleanup or notify other components.
 * @param session The session that has been disconnected.
 */
namespace CppMMO
{
    namespace Network
    {
        class SessionManager : public ISessionManager
        {
        public:
            SessionManager() = default;
            explicit SessionManager(std::shared_ptr<Game::GameLogicQueue> gameLogicQueue);
            virtual ~SessionManager() noexcept override = default;

            virtual void AddSession(std::shared_ptr<ISession> session) override;
            virtual void RemoveSession(uint64_t sessionId) override;
            virtual std::shared_ptr<ISession> GetSession(uint64_t sessionId) const override;
            virtual std::vector<std::shared_ptr<ISession>> GetAllSessions() const override;
            virtual size_t GetActiveSessionCount() const override;
            
            void OnSessionDisconnected(std::shared_ptr<ISession> session);
        private:
            mutable std::mutex m_mutex;
            std::unordered_map<uint64_t, std::shared_ptr<ISession>> m_activeSessions;

            std::shared_ptr<Game::GameLogicQueue> m_gameLogicQueue; 
        };
    }
}