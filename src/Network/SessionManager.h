#pragma once
#include "pch.h"

#include "ISessionManager.h"
#include "ISession.h"

namespace CppMMO
{
    namespace Network
    {
        class SessionManager : public ISessionManager
        {
        public:
            SessionManager() = default;
            virtual ~SessionManager() noexcept override = default;

            virtual void AddSession(std::shared_ptr<ISession> session) override;
            virtual void RemoveSession(uint64_t sessionId) override;
            virtual std::shared_ptr<ISession> GetSession(uint64_t sessionId) const override;
        private:
            mutable std::mutex m_mutex;
            std::unordered_map<uint64_t, std::shared_ptr<ISession>> m_activeSessions;
        };
    }
}