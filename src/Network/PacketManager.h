#pragma once
#include "pch.h"
#include "IPacketManager.h"
#include "Utils/JobQueue.h"

namespace CppMMO
{
    namespace Utils 
    {
        class JobQueue;
    }

    namespace Network
    {
        class PacketManager : public IPacketManager
        {
        public:
            explicit PacketManager(std::shared_ptr<Utils::JobQueue> jobQueue);
            virtual ~PacketManager() = default;
            
            virtual void RegisterHandler(PacketId id, const PacketHandler& handler) override;
            virtual void UnregisterHandler(PacketId id) noexcept override;
            virtual void HandlePacket(const std::shared_ptr<ISession>& session, const std::vector<std::byte>& packet) override;
            virtual void DispatchPacket(Protocol::PacketId id, const std::shared_ptr<ISession>& session, const Protocol::UnifiedPacket* packet) override;
        private:
            std::unordered_map<PacketId, PacketHandler> m_handlers{};
            std::shared_ptr<Utils::JobQueue> m_jobQueue;
        };
    }
}