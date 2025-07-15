#pragma once

#include "pch.h"
#include "JobQueue.h"
#include "Network/IPacketManager.h"
#include "Game/GameLogicQueue.h"

namespace CppMMO
{
    namespace Utils
    {
        class JobProcessor
        {
        public:
            JobProcessor(std::shared_ptr<JobQueue> jobQueue, 
                         std::shared_ptr<Network::IPacketManager> packetManager,
                         std::shared_ptr<Game::GameLogicQueue> gameLogicQueue);
            ~JobProcessor();

            void Start(int numThreads);
            void Stop();

        private:
            std::shared_ptr<JobQueue> m_jobQueue;
            std::shared_ptr<Network::IPacketManager> m_packetManager;
            std::shared_ptr<Game::GameLogicQueue> m_gameLogicQueue;

            std::vector<std::thread> m_workerThreads{};
            std::atomic<bool> m_running = false;

            void WorkerLoop();
            void ProcessJobPacket(const Job& job, const Protocol::UnifiedPacket* unifiedPacket);
        };
    }
}