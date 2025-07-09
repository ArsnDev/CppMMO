#pragma once
#include "pch.h"
#include "Network/ISession.h"
#include "protocol_generated.h"

namespace CppMMO
{
    namespace Utils
    {
        struct Job
        {
            std::shared_ptr<Network::ISession> session;
            std::vector<uint8_t> packetBuffer;
            Protocol::PacketId packetId;
            bool isShutdownSignal = false;
        };

        class JobQueue
        {
        public:
            JobQueue();
            void PushJob(Job job);
            Job PopJob();
            void Shutdown();
            bool IsShuttingDown() const {return m_shuttingDown.load(std::memory_order_acquire);}
        private:
            moodycamel::ConcurrentQueue<Job> m_jobQueue;
            mutable std::mutex m_mutex;
            std::condition_variable m_condition;
            std::atomic<bool> m_shuttingDown = false;
        };
    }
}