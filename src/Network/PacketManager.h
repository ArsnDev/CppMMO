#pragma once
#include "pch.h"
#include "IPacketManager.h"

namespace CppMMO
{
    namespace Network
    {
        class PacketManager : public IPacketManager
        {
        public:
            PacketManager() = default;
            virtual ~PacketManager() = default;
            
            virtual void RegisterHandler(PacketId id, PacketHandler handler) override;
            virtual void UnregisterHandler(PacketId id) noexcept override;
            virtual void HandlePacket(std::shared_ptr<ISession> session, const Protocol::UnifiedPacket* packet) override;

        private:
            std::unordered_map<PacketId, PacketHandler> m_handlers;
        };
    }
}