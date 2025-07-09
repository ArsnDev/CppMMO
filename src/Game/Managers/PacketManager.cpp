#include "pch.h"
#include "PacketManager.h"
#include "JobProcessor.h"
#include "JobQueue.h"

namespace asio = boost::asio;
namespace ip = boost::asio::ip;

namespace CppMMO
{
    namespace Network
    {
        PacketManager::PacketManager(std::shared_ptr<Utils::JobQueue> jobQueue)
            : m_jobQueue(std::move(jobQueue))
        {
        }
        
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

        void PacketManager::HandlePacket(const std::shared_ptr<ISession>& session, std::span<const uint8_t> rawPacketData)
        {
            if (!session || rawPacketData.empty())
            {
                LOG_ERROR("Attempted to handle null session or empty raw packet data.");
                return;
            }
            flatbuffers::Verifier verifier(rawPacketData.data(), rawPacketData.size());
            if(!Protocol::VerifyUnifiedPacketBuffer(verifier))
            {
                LOG_ERROR("Received invalid FlatBuffers UnifiedPacket buffer.");
                return;
            }
            const Protocol::UnifiedPacket* packet = Protocol::GetUnifiedPacket(rawPacketData.data());

            Protocol::PacketId id = packet->id();

            auto it = m_handlers.find(id);
            if (it == m_handlers.end())
            {
                LOG_WARN("No handler registered for PacketId: {}", static_cast<int>(id));
                return;
            }
            if (m_jobQueue)
            {
                Utils::Job job;
                job.session = session;
                job.packetId = id;
                job.packetBuffer.assign(rawPacketData.begin(), rawPacketData.end());
                m_jobQueue->PushJob(std::move(job));
                LOG_DEBUG("PacketId {} pushed to JobQueue.", static_cast<int>(id));
            }
            else
            {
                LOG_ERROR("JobQueue is null in PacketManager. Cannot push packet job.");
            }
        }

        void PacketManager::DispatchPacket(Protocol::PacketId id, const std::shared_ptr<ISession>& session, const Protocol::UnifiedPacket* packet)
        {
            auto it = m_handlers.find(id);
            if (it != m_handlers.end())
            {
                it->second(session, packet);
            }
            else
            {
                LOG_WARN("No handler registered for PacketId: {} for direct dispatch.", static_cast<int>(id));
            }
        }
    }
}
