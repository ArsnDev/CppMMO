#pragma once
#include "pch.h"
#include <span>
#include <cstddef>

namespace CppMMO
{
    namespace Network
    {
        class ISession;

        class IPacketManager
        {
        public:
            virtual ~IPacketManager() = default;

            virtual void HandlePacket(std::shared_ptr<ISession> session, std::span<const std::byte> data) = 0;
        };
    }
}
