#include "JobProcessor.h"

namespace CppMMO
{
    namespace Utils
    {
        JobProcessor::JobProcessor(std::shared_ptr<JobQueue> jobQueue, 
                                   std::shared_ptr<Network::IPacketManager> packetManager,
                                   std::shared_ptr<Game::GameLogicQueue> gameLogicQueue)
            : m_jobQueue(jobQueue), 
              m_packetManager(packetManager),
              m_gameLogicQueue(gameLogicQueue)
        {
            if (!m_jobQueue)
            {
                LOG_CRITICAL("JobProcessor initialized with null JobQueue");
            }
            if (!m_packetManager)
            {
                LOG_CRITICAL("JobProcessor initialized with null PacketManager");
            }
            if (!m_gameLogicQueue)
            {
                LOG_CRITICAL("JobProcessor initialized with null GameLogicQueue");
            }
        }

        JobProcessor::~JobProcessor()
        {
            Stop();
        }

        void JobProcessor::Start(int numThreads)
        {
            if(m_running.load(std::memory_order_acquire))
            {
                LOG_WARN("JobProcessor is already running.");
                return;
            }
            m_running.store(true, std::memory_order_release);

            for(int i=0; i< numThreads; ++i)
            {
                m_workerThreads.emplace_back(&JobProcessor::WorkerLoop, this);
                LOG_INFO("JobProcessor worker thread {} started.", i+1);
            }
        }

        void JobProcessor::Stop()
        {
            if (!m_running.load(std::memory_order_acquire))
            {
                return;
            }
            for (size_t i = 0; i < m_workerThreads.size(); ++i)
            {
                m_jobQueue->PushJob({ .isShutdownSignal = true});
            }
            for (std::thread& thread : m_workerThreads)
            {
                if (thread.joinable())
                {
                    thread.join();
                }
            }
            m_workerThreads.clear();
            LOG_INFO("JobProcessor stopped and all worker threads joined.");
        }

        void JobProcessor::WorkerLoop()
        {
            while (true)
            {
                Job job = m_jobQueue->PopJob();

                if (job.isShutdownSignal)
                {
                    LOG_INFO("JobProcessor worker thread received shutdown signal and is exiting.");
                    break;
                }
                try
                {
                    if(job.packetBuffer.empty())
                    {
                        LOG_ERROR("Received empty packet buffer in worker thread.");
                        continue;
                    }
                    flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(job.packetBuffer.data()), job.packetBuffer.size());
                    if(!Protocol::VerifyUnifiedPacketBuffer(verifier))
                    {
                        LOG_ERROR("Received invalid FlatBuffers UnifiedPacket in worker thread.");
                        continue;
                    }
                    const Protocol::UnifiedPacket* unifiedPacket = Protocol::GetUnifiedPacket(job.packetBuffer.data());
                    ProcessJobPacket(job, unifiedPacket);
                }
                catch(const std::exception& e)
                {
                    LOG_ERROR("Exception in JobProcessor worker thread: {}", e.what());
                }
            }
        }

        void JobProcessor::ProcessJobPacket(const Job& job, const Protocol::UnifiedPacket* unifiedPacket)
        {
            Protocol::PacketId packetId = unifiedPacket->id();
            bool isNonGamePacket = false;
            switch (packetId)
            {
                case Protocol::PacketId_C_Login:
                case Protocol::PacketId_S_Login:
                case Protocol::PacketId_C_Chat:
                case Protocol::PacketId_S_Chat:
                    isNonGamePacket = true;
                    break;
                default:
                    break;
            }
            if (isNonGamePacket)
            {
                if(m_packetManager)
                {
                    m_packetManager->DispatchPacket(packetId, job.session, unifiedPacket);
                }
                else
                {
                    LOG_ERROR("PacketManager is null in JobProcessor worker thread.");
                }
            }
            else
            {
                if(m_gameLogicQueue)
                {
                    Game::GameCommand gameCommand;
                    gameCommand.commandId = 0;
                    gameCommand.senderSessionId = job.session->GetSessionId();

                    switch (packetId)
                    {
                        case Protocol::PacketId_C_Move:
                        {
                            const Protocol::C_Move* c_move_packet = static_cast<const Protocol::C_Move*>(unifiedPacket->data());
                            if (c_move_packet)
                            {
                                Game::MoveCommandData moveCommandData;
                                moveCommandData.entityId = job.session->GetPlayerId();
                                moveCommandData.targetPosition.x = c_move_packet->target_position()->x();
                                moveCommandData.targetPosition.y = c_move_packet->target_position()->y();
                                gameCommand.payload = moveCommandData;
                                m_gameLogicQueue->PushGameCommand(std::move(gameCommand));
                                LOG_DEBUG("In-game PacketId {} (C_Move) pushed to GameLogicQueue.", static_cast<int>(packetId));
                            }
                            else
                            {
                                LOG_ERROR("Failed to get C_Move packet data from UnifiedPacket in JobProcessor.");
                            }
                            break;
                        }
                        case Protocol::PacketId_C_ChangeZone:
                        {
                            const Protocol::C_ChangeZone* c_change_zone_packet = static_cast<const Protocol::C_ChangeZone*>(unifiedPacket->data());
                            if (c_change_zone_packet)
                            {
                                Game::ChangeZoneCommandData changeZoneCommandData;
                                changeZoneCommandData.playerId = job.session->GetPlayerId();
                                changeZoneCommandData.targetZoneId = c_change_zone_packet->target_zone_id();
                                gameCommand.payload = changeZoneCommandData;
                                m_gameLogicQueue->PushGameCommand(std::move(gameCommand));
                                LOG_DEBUG("In-game PacketId {} (C_ChangeZone) pushed to GameLogicQueue.", static_cast<int>(packetId));
                            }
                            break;
                        }
                        case Protocol::PacketId_S_PlayerHpUpdate:
                        {
                            const Protocol::S_PlayerHpUpdate* s_player_hp_update_packet = static_cast<const Protocol::S_PlayerHpUpdate*>(unifiedPacket->data());
                            if (s_player_hp_update_packet)
                            {
                                Game::PlayerHpUpdateCommandData hpUpdateCommandData;
                                hpUpdateCommandData.playerId = s_player_hp_update_packet->player_id();
                                hpUpdateCommandData.currentHp = s_player_hp_update_packet->current_hp();
                                gameCommand.payload = hpUpdateCommandData;
                                m_gameLogicQueue->PushGameCommand(std::move(gameCommand));
                                LOG_DEBUG("In-game PacketId {} (S_PlayerHpUpdate) pushed to GameLogicQueue.", static_cast<int>(packetId));
                            }
                            break;
                        }
                        default:
                        {
                            LOG_WARN("Unhandled in-game PacketId {} in JobProcessor. No GameCommand created.", static_cast<int>(packetId));
                            break;
                        }
                    }
                }
                else
                {
                    LOG_ERROR("GameLogicQueue is null in JobProcessor. Cannot push in-game packet.");
                }
            }
        }
    }
}