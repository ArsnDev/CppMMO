#include "JobQueue.h"

namespace CppMMO
{
    namespace Utils
    {
        JobQueue::JobQueue() : m_jobQueue(256)
        {
        }

        void JobQueue::PushJob(Job job)
        {
            if (m_shuttingDown.load(std::memory_order_acquire))
            {
                LOG_WARN("Attempted to push job to a shutting down queue.");
                return;
            }
            m_jobQueue.enqueue(std::move(job));

            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.notify_one();
        }

        Job JobQueue::PopJob()
        {
            Job job;
            std::unique_lock<std::mutex> lock(m_mutex);

            m_condition.wait(lock, [this, &job]
            {
                return m_jobQueue.try_dequeue(job) || m_shuttingDown.load(std::memory_order_acquire);
            });

            if(m_shuttingDown.load(std::memory_order_acquire) && !m_jobQueue.try_dequeue(job))
            {
                return Job{};
            }

            return job;
        }

        void JobQueue::Shutdown()
        {
            m_shuttingDown.store(true, std::memory_order_release);

            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.notify_all();
        }
    }
}