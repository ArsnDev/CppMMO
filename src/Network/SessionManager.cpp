#include "SessionManager.h"

namespace CppMMO
{
    namespace Network
    {
        void SessionManager::AddSession(std::shared_ptr<ISession> session)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_activeSessions[session->GetSessionId()] = session;
            LOG_INFO("SessionManager: Session {} added. Total active sessions: {}", session->GetSessionId(), m_activeSessions.size());
        }

        void SessionManager::RemoveSession(uint64_t sessionId)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_activeSessions.erase(sessionId);
            LOG_INFO("SessionManager: Session {} removed. Total active sessions: {}", sessionId, m_activeSessions.size());
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
    }
}