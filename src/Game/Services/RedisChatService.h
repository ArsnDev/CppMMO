#pragma once
#include "pch.h"
#include <sw/redis++/redis++.h>

namespace CppMMO
{
    namespace Game
    {
        namespace Services
        {
            class RedisChatService
            {
            public:
                static RedisChatService& GetInstance();

                RedisChatService(const RedisChatService&) = delete;
                RedisChatService& operator=(const RedisChatService&) = delete;

                bool Connect(const std::string& uri);
                void Disconnect();
                bool Publish(const std::string& channel, const std::string& message);
                void Subscribe(const std::string& channel, std::function<void(const std::string&, const std::string&)> callback);
                void Unsubscribe(const std::string& channel);

            private:
                RedisChatService();
                ~RedisChatService();

                std::unique_ptr<sw::redis::Redis> m_redis;
                std::unique_ptr<sw::redis::Subscriber> m_subscriber;
                std::thread m_subscribeThread;
                std::atomic<bool> m_running;
            };
        }
    }
}
