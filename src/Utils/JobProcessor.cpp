#include "JobProcessor.h"

namespace CppMMO
{
    namespace Utils
    {
        JobProcessor::JobProcessor(std::shared_ptr<JobQueue> jobQueue, std::shared_ptr<Network::IPacketManager> packetManager)
            : m_jobQueue(jobQueue), m_packetManager(packetManager)
        {
            if (!m_jobQueue)
            {
                LOG_CRITICAL("JobProcessor initialized with null JobQueue");
            }
            if (!m_packetManager)
            {
                LOG_CRITICAL("JobProcessor initialized with null PacketManager");
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
                    flatbuffers::Verifier verifier(job.packetBuffer.data(), job.packetBuffer.size());
                    if(!Protocol::VerifyUnifiedPacketBuffer(verifier))
                    {
                        LOG_ERROR("Received invalid FlatBuffers UnifiedPacket in worker thread.");
                        continue;
                    }
                    const Protocol::UnifiedPacket* unifiedPacket = Protocol::GetUnifiedPacket(job.packetBuffer.data());
                    if (m_packetManager)
                    {
                        m_packetManager->DispatchPacket(job.packetId, job.session, unifiedPacket);
                    }
                    else
                    {
                        LOG_ERROR("PacketManager is null in JobProcessor worker thread.");
                    }
                }
                catch(const std::exception& e)
                {
                    LOG_ERROR("Exception in JobProcessor worker thread: {}", e.what());
                }
            }
        }
    }
}