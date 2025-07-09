#pragma once
#include "pch.h"
#include "Network/ISession.h"
#include "protocol_generated.h"

namespace CppMMO
{
    namespace Game
    {
        namespace PacketHandlers
        {
            class LoginPacketHandler
            {
            public:
                void operator()(std::shared_ptr<Network::ISession> session, const Protocol::UnifiedPacket* unifiedPacket) const;
            };
        }
    }
}