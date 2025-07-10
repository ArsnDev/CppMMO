#include "GameLogicQueue.h"

namespace CppMMO
{
    namespace Game
    {
        GameLogicQueue::GameLogicQueue()
        {
        }

        void GameLogicQueue::PushGameCommand(GameCommand gameCommand)
        {
            if (m_shuttingDown.load(std::memory_order_acquire))
            {
                LOG_WARN("Attempted to push game command to a shutting down queue.");
                return;
            }
            m_gameCommandQueue.enqueue(std::move(gameCommand));
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.notify_one();
        }

        GameCommand GameLogicQueue::PopGameCommand()
        {
            GameCommand command;
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.wait(lock, [this, &command]
            {
                return m_gameCommandQueue.try_dequeue(command) || m_shuttingDown.load(std::memory_order_acquire);
            });
            if(m_shuttingDown.load(std::memory_order_acquire) && !m_gameCommandQueue.try_dequeue(command))
            {
                return GameCommand{};
            }
            return command;
        }

        std::optional<GameCommand> GameLogicQueue::TryPopGameCommand()
        {
            GameCommand command;
            if (m_gameCommandQueue.try_dequeue(command))
            {
                return command;
            }
            return std::nullopt;
        }

        void GameLogicQueue::Shutdown()
        {
            m_shuttingDown.store(true, std::memory_order_release);
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.notify_all();
        }
    }
}