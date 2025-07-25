#pragma once
#include "pch.h"
#include "Network/ISession.h"

namespace CppMMO
{
    namespace Network
    {
        class ISession;

        class ISessionManager
        {
        public:
            ISessionManager() = default;
            virtual ~ISessionManager() noexcept = default;
            virtual void AddSession(std::shared_ptr<ISession> session) = 0;
            virtual void RemoveSession(uint64_t sessionId) = 0;
            virtual std::shared_ptr<ISession> GetSession(uint64_t sessionId) const = 0;
            virtual std::vector<std::shared_ptr<ISession>> GetAllSessions() const = 0;
            virtual size_t GetActiveSessionCount() const = 0;
        };
    }
}