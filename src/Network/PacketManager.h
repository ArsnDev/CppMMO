#pragma once
#include "pch.h"
#include "IPacketManager.h"
#include <span>
#include <cstddef>

namespace CppMMO
{
    namespace Network
    {
        class ISession;

        class PacketManager : public IPacketManager
        {
        public:
            PacketManager() = default;
            virtual ~PacketManager() = default;

            virtual void HandlePacket(std::shared_ptr<ISession> session, std::span<const std::byte> data) override;
        };
    }
}