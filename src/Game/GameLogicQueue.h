#pragma once
#include "pch.h"
#include "GameCommand.h"

namespace CppMMO
{
    namespace Game
    {
        class GameLogicQueue
        {
        public:
            GameLogicQueue();
            void PushGameCommand(GameCommand gameCommand);
            GameCommand PopGameCommand();
            std::optional<GameCommand> TryPopGameCommand();
            void Shutdown();
            bool IsShuttingDown() const {return m_shuttingDown.load(std::memory_order_acquire);}
        private:
            moodycamel::ConcurrentQueue<GameCommand> m_gameCommandQueue{};
            mutable std::mutex m_mutex{};
            std::condition_variable m_condition{};
            std::atomic<bool> m_shuttingDown = false;
        };
    }
}