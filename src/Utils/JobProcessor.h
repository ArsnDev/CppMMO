#pragma once

#include "pch.h"
#include "JobQueue.h"
#include "Network/IPacketManager.h"

namespace CppMMO
{
    namespace Utils
    {
        class JobProcessor
        {
        public:
            JobProcessor(std::shared_ptr<JobQueue> jobQueue, std::shared_ptr<Network::IPacketManager> packetManager);
            ~JobProcessor();

            void Start(int numThreads);
            void Stop();

        private:
            std::shared_ptr<JobQueue> m_jobQueue;
            std::shared_ptr<Network::IPacketManager> m_packetManager;

            std::vector<std::thread> m_workerThreads;
            std::atomic<bool> m_running = false;

            void WorkerLoop();
        };
    }
}