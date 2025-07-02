#pragma once
#include "pch.h"
#include <span>
#include <cstddef>

namespace CppMMO
{
    namespace Network
    {
        class ISession
        {
        public:
            virtual ~ISession() = default;
            /**
             * @brief Starts the session and initializes asynchronous I/O operations.
             * 
             * @details This method is called separately from the constructor to safely use `shared_from_this()`.
             *          Since `shared_from_this()` cannot be called in the constructor,
             *          this two-phase initialization pattern is used for safe session lifecycle management and asynchronous operation initiation.
             */
            virtual void Start() = 0;

            /**
             * @brief Executes the session's Send operation.
             */
            virtual void Send(std::span<const std::byte> data) = 0;
        };
    }
}
