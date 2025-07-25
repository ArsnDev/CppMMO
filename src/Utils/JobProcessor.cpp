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
                Job shutdownJob;
                shutdownJob.isShutdownSignal = true;
                m_jobQueue->PushJob(std::move(shutdownJob));
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
                    
                    // 패킷 데이터 처리 - 필요시 헤더 제거
                    const uint8_t* packetData = reinterpret_cast<const uint8_t*>(job.packetBuffer.data());
                    size_t packetSize = job.packetBuffer.size();
                    
                    LOG_DEBUG("JobProcessor: Processing packet size={}, Session: {}", 
                             packetSize, job.session->GetSessionId());
                    
                    // 일관된 패킷 처리: Session에서 이미 4바이트 헤더를 제거했으므로 순수 FlatBuffers 데이터
                    LOG_DEBUG("Processing FlatBuffers packet of size: {}", packetSize);
                    
                    flatbuffers::Verifier verifier(packetData, packetSize);
                    if(!Protocol::VerifyUnifiedPacketBuffer(verifier))
                    {
                        LOG_ERROR("Received invalid FlatBuffers UnifiedPacket in worker thread. Buffer size: {}, Session: {}", 
                                 packetSize, job.session->GetSessionId());
                        // 첫 16바이트 출력 (디버깅용)
                        std::string hexDump;
                        for(size_t i = 0; i < std::min(packetSize, size_t(16)); ++i) {
                            hexDump += fmt::format("{:02x} ", static_cast<unsigned char>(packetData[i]));
                        }
                        LOG_ERROR("Packet hex dump (first 16 bytes): {}", hexDump);
                        continue;
                    }
                    const Protocol::UnifiedPacket* unifiedPacket = Protocol::GetUnifiedPacket(packetData);
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
            LOG_DEBUG("JobProcessor: Received PacketId {} from Session {}", static_cast<int>(packetId), job.session->GetSessionId());
            bool isNonGamePacket = false;
            switch (packetId)
            {
                case Protocol::PacketId_C_Login:
                case Protocol::PacketId_S_LoginSuccess:
                case Protocol::PacketId_S_LoginFailure:
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
                        case Protocol::PacketId_C_PlayerInput:
                        {
                            const Protocol::C_PlayerInput* c_player_input_packet = static_cast<const Protocol::C_PlayerInput*>(unifiedPacket->data());
                            if (c_player_input_packet)
                            {
                                Game::PlayerInputCommandData playerInputCommandData;
                                playerInputCommandData.playerId = job.session->GetPlayerId();
                                playerInputCommandData.inputFlags = c_player_input_packet->input_flags();
                                playerInputCommandData.sequenceNumber = c_player_input_packet->sequence_number();
                                gameCommand.payload = playerInputCommandData;
                                m_gameLogicQueue->PushGameCommand(std::move(gameCommand));
                                LOG_DEBUG("In-game PacketId {} (C_PlayerInput) pushed to GameLogicQueue. InputFlags: {}, Seq: {}", static_cast<int>(packetId), c_player_input_packet->input_flags(), c_player_input_packet->sequence_number());
                            }
                            else
                            {
                                LOG_ERROR("Failed to get C_PlayerInput packet data from UnifiedPacket in JobProcessor.");
                            }
                            break;
                        }
                        case Protocol::PacketId_C_EnterZone:
                        {
                            const Protocol::C_EnterZone* c_enter_zone_packet = static_cast<const Protocol::C_EnterZone*>(unifiedPacket->data());
                            if (c_enter_zone_packet)
                            {
                                Game::EnterZoneCommandData enterZoneCommandData;
                                enterZoneCommandData.playerId = job.session->GetPlayerId();
                                enterZoneCommandData.zoneId = c_enter_zone_packet->zone_id();
                                enterZoneCommandData.sessionId = job.session->GetSessionId();
                                gameCommand.payload = enterZoneCommandData;
                                m_gameLogicQueue->PushGameCommand(std::move(gameCommand));
                                LOG_DEBUG("In-game PacketId {} (C_EnterZone) pushed to GameLogicQueue.", static_cast<int>(packetId));
                            }
                            else
                            {
                                LOG_ERROR("Failed to get C_EnterZone packet data from UnifiedPacket in JobProcessor.");
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