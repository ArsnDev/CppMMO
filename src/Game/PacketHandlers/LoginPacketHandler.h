#pragma once
#include "pch.h"
#include "Network/ISession.h"
#include "Game/Services/AuthService.h"
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
                LoginPacketHandler(boost::asio::io_context& ioc, std::shared_ptr<CppMMO::Game::Services::AuthService> authService);
                void operator()(std::shared_ptr<Network::ISession> session, const Protocol::UnifiedPacket* unifiedPacket) const;
            private:
                void SendLoginFailure(std::shared_ptr<Network::ISession> session, int errorCode, const std::string& errorMessage, int64_t commandId) const;
                boost::asio::io_context& m_ioc;
                std::shared_ptr<CppMMO::Game::Services::AuthService> m_authService;
            };
        }
    }
}