#include "pch.h"
#include "ChatManager.h"
#include "Utils/Logger.h"
#include "protocol_generated.h"

namespace CppMMO
{
    namespace Game
    {
        namespace Managers
        {
            ChatManager& ChatManager::GetInstance()
            {
                static ChatManager instance;
                return instance;
            }

            ChatManager::ChatManager()
            {
            }

            ChatManager::~ChatManager()
            {
                Shutdown();
            }

            void ChatManager::Initialize(std::shared_ptr<Network::TcpServer> tcpServer)
            {
                m_tcpServer = tcpServer;
                m_tcpServer->SetOnSessionConnected([this](std::shared_ptr<Network::ISession> session) {
                    OnSessionConnected(session);
                });
                m_tcpServer->SetOnSessionDisconnected([this](std::shared_ptr<Network::ISession> session) {
                    OnSessionDisconnected(session);
                });

                if (!Services::RedisChatService::GetInstance().Connect("tcp://127.0.0.1:6379"))
                {
                    LOG_ERROR("ChatManager: Failed to connect to Redis.");
                    return;
                }

                Services::RedisChatService::GetInstance().Subscribe("chat_channel",
                    [this](const std::string& channel, const std::string& message)
                {
                    HandleRedisChatMessage(channel, message);
                });

                LOG_INFO("ChatManager initialized.");
            }

            void ChatManager::Shutdown()
            {
                Services::RedisChatService::GetInstance().Disconnect();
                LOG_INFO("ChatManager shutdown.");
            }

            void ChatManager::HandleRedisChatMessage(const std::string& channel, const std::string& message)
            {
                LOG_DEBUG("ChatManager: Received Redis message on channel \'{}\': {}.", channel, message);

                // Create S_Chat packet
                flatbuffers::FlatBufferBuilder builder;
                auto chat_message_offset = builder.CreateString(message);
                // Assuming player_id 0 for server broadcast, or you can parse it from the message if included
                auto s_chat_packet = Protocol::CreateS_Chat(builder, 0, chat_message_offset);
                auto unified_packet_offset = Protocol::CreateUnifiedPacket(builder, Protocol::PacketId_S_Chat, Protocol::Packet_S_Chat, s_chat_packet.Union());
                builder.Finish(unified_packet_offset);

                std::span<const std::byte> send_buffer(reinterpret_cast<const std::byte*>(builder.GetBufferPointer()), builder.GetSize());

                // Broadcast to all connected sessions
                std::lock_guard<std::mutex> lock(m_sessionsMutex);
                for (const auto& pair : m_connectedSessions)
                {
                    pair.second->Send(send_buffer);
                }
                LOG_INFO("ChatManager: Broadcasted chat message to {} sessions.", m_connectedSessions.size());
            }

            void ChatManager::OnSessionConnected(std::shared_ptr<Network::ISession> session)
            {
                std::lock_guard<std::mutex> lock(m_sessionsMutex);
                m_connectedSessions[session->GetRemoteEndpoint().address().to_string()] = session;
                LOG_INFO("ChatManager: Session connected: {}. Total sessions: {}", session->GetRemoteEndpoint().address().to_string(), m_connectedSessions.size());
            }

            void ChatManager::OnSessionDisconnected(std::shared_ptr<Network::ISession> session)
            {
                std::lock_guard<std::mutex> lock(m_sessionsMutex);
                m_connectedSessions.erase(session->GetRemoteEndpoint().address().to_string());
                LOG_INFO("ChatManager: Session disconnected: {}. Total sessions: {}", session->GetRemoteEndpoint().address().to_string(), m_connectedSessions.size());
            }
        }
    }
}
