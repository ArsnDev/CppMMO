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
            std::vector<std::byte> packetBuffer;
            bool isShutdownSignal = false;
            
            // Move constructor to avoid vector copying
            Job() = default;
            Job(std::shared_ptr<Network::ISession> sess, std::vector<std::byte>&& buffer)
                : session(std::move(sess)), packetBuffer(std::move(buffer)) {}
            Job(Job&& other) noexcept
                : session(std::move(other.session)), 
                  packetBuffer(std::move(other.packetBuffer)),
                  isShutdownSignal(other.isShutdownSignal) {}
            Job& operator=(Job&& other) noexcept
            {
                if (this != &other)
                {
                    session = std::move(other.session);
                    packetBuffer = std::move(other.packetBuffer);
                    isShutdownSignal = other.isShutdownSignal;
                }
                return *this;
            }
            
            // Delete copy operations to prevent accidental copying
            Job(const Job&) = delete;
            Job& operator=(const Job&) = delete;
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
            ::moodycamel::ConcurrentQueue<Job> m_jobQueue;
            mutable std::mutex m_mutex;
            std::condition_variable m_condition;
            std::atomic<bool> m_shuttingDown = false;
        };
    }
}