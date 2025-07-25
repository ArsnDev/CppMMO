#include "pch.h"
#include "ChatPacketHandler.h"
#include "Game/Services/RedisChatService.h"

namespace CppMMO
{
    namespace Game
    {
        namespace PacketHandlers
        {
            void ChatPacketHandler::operator()(std::shared_ptr<Network::ISession> session, const Protocol::UnifiedPacket* unifiedPacket) const
            {
                if (!session)
                {
                    LOG_ERROR("Error: Session is null in ChatPacketHandler.");
                    return;
                }

                if (unifiedPacket->id() != Protocol::PacketId_C_Chat)
                {
                    LOG_ERROR("Error: Session {}: Received non-C_Chat packet in ChatPacketHandler. Actual ID: {}", session->GetRemoteEndpoint().address().to_string(), static_cast<int>(unifiedPacket->id()));
                    return;
                }

                const Protocol::C_Chat* c_chat_packet = unifiedPacket->data_as_C_Chat();
                if (!c_chat_packet || !c_chat_packet->message())
                {
                    LOG_ERROR("Error: Session {}: Received C_Chat packet with null data or message.", session->GetRemoteEndpoint().address().to_string());
                    return;
                }

                const std::string chat_message = c_chat_packet->message()->str();
                const int64_t player_id = session->GetPlayerId();
                LOG_INFO("[ChatPacketHandler] Processing chat message: '{}' from player: {} (session: {})", chat_message, player_id, session->GetRemoteEndpoint().address().to_string());

                std::string message_with_player = std::to_string(player_id) + "|" + chat_message;
                
                Services::RedisChatService::GetInstance().Publish("chat_channel", message_with_player);

                LOG_INFO("--- ChatPacketHandler: Finished processing chat message '{}' ---", chat_message);
            }
        }
    }
}
