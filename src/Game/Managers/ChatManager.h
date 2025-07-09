#pragma once
#include "pch.h"
#include "Network/TcpServer.h"
#include "Game/Services/RedisChatService.h"

namespace CppMMO
{
    namespace Game
    {
        namespace Managers
        {
            class ChatManager
            {
            public:
                static ChatManager& GetInstance();

                ChatManager(const ChatManager&) = delete;
                ChatManager& operator=(const ChatManager&) = delete;

                void Initialize(std::shared_ptr<Network::TcpServer> tcpServer);
                void Shutdown();

            private:
                ChatManager();
                ~ChatManager();

                std::shared_ptr<Network::TcpServer> m_tcpServer;
                std::map<std::string, std::shared_ptr<Network::ISession>> m_connectedSessions;
                std::mutex m_sessionsMutex;

                void HandleRedisChatMessage(const std::string& channel, const std::string& message);
                void OnSessionConnected(std::shared_ptr<Network::ISession> session);
                void OnSessionDisconnected(std::shared_ptr<Network::ISession> session);
            };
        }
    }
}
