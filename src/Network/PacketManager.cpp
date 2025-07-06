#include "pch.h"
#include "PacketManager.h"

namespace asio = boost::asio;
namespace ip = boost::asio::ip;

namespace CppMMO
{
    namespace Network
    {
        void PacketManager::RegisterHandler(PacketId id, const PacketHandler& handler)
        {
            if(!handler)
            {
                LOG_ERROR("Attempted to register a null PacketHandler for PacketId: {}", static_cast<int>(id));
                throw std::invalid_argument("Attempted to register a null PacketHandler for PacketId: " + std::to_string(static_cast<int>(id)));
            }
            m_handlers.emplace(id, handler);
            LOG_INFO("Handler registered for PacketId: {}", static_cast<int>(id));
        }

        void PacketManager::UnregisterHandler(PacketId id) noexcept
        {
            m_handlers.erase(id);
            LOG_INFO("Handler unregistered for PacketId: {}", static_cast<int>(id));
        }

        void PacketManager::HandlePacket(const std::shared_ptr<ISession>& session, const Protocol::UnifiedPacket* packet)
        {
            PacketId packetId = packet->id();
            auto it = m_handlers.find(packetId);
            if(it != m_handlers.end())
            {
                it->second(session, packet);
                LOG_DEBUG("Handled packet with ID: {}", static_cast<int>(packetId));
            }
            else
            {
                LOG_WARN("No handler registered for PacketId: {}", static_cast<int>(packetId));
            }
        }
    }
}
