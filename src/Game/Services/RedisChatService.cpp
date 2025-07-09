#include "pch.h"
#include "RedisChatService.h"

namespace CppMMO
{
    namespace Game
    {
        namespace Services
        {
            RedisChatService& RedisChatService::GetInstance()
            {
                static RedisChatService instance;
                return instance;
            }

            RedisChatService::RedisChatService()
                : m_redis(nullptr), m_subscriber(nullptr), m_running(false)
            {
            }

            RedisChatService::~RedisChatService()
            {
                Disconnect();
            }

            bool RedisChatService::Connect(const std::string& uri)
            {
                try
                {
                    m_redis = std::make_unique<sw::redis::Redis>(uri);
                    m_subscriber = std::make_unique<sw::redis::Subscriber>(m_redis->subscriber());
                    LOG_INFO("RedisChatService: Connected to Redis at {}.", uri);
                    m_running = true;
                    return true;
                }
                catch (const sw::redis::Error& e)
                {
                    LOG_ERROR("RedisChatService: Failed to connect to Redis: {}.", e.what());
                    return false;
                }
            }

            void RedisChatService::Disconnect()
            {
                if (m_running)
                {
                    m_running = false;
                    if (m_subscribeThread.joinable())
                    {
                        m_subscribeThread.join();
                    }
                    if (m_subscriber)
                    {
                        m_subscriber->unsubscribe("chat_channel"); // Unsubscribe from all channels
                    }
                    LOG_INFO("RedisChatService: Disconnected from Redis.");
                }
            }

            bool RedisChatService::Publish(const std::string& channel, const std::string& message)
            {
                if (!m_redis)
                {
                    LOG_ERROR("RedisChatService: Not connected to Redis. Cannot publish.");
                    return false;
                }
                try
                {
                    m_redis->publish(channel, message);
                    LOG_DEBUG("RedisChatService: Published message to channel \'{}\': {}.", channel, message);
                    return true;
                }
                catch (const sw::redis::Error& e)
                {
                    LOG_ERROR("RedisChatService: Failed to publish message: {}.", e.what());
                    return false;
                }
            }

            void RedisChatService::Subscribe(const std::string& channel, std::function<void(const std::string&, const std::string&)> callback)
            {
                if (!m_subscriber)
                {
                    LOG_ERROR("RedisChatService: Not connected to Redis. Cannot subscribe.");
                    return;
                }

                m_subscriber->subscribe(channel);

                // Register the message callback
                m_subscriber->on_message([callback](const std::string& channel, const std::string& msg) {
                    if (callback) {
                        callback(channel, msg);
                    }
                });

                m_subscribeThread = std::thread([this]()
                {
                    LOG_INFO("RedisChatService: Starting subscribe thread.");
                    try
                    {
                        while (m_running)
                        {
                            m_subscriber->consume(); // Blocking call, waits for messages
                        }
                    }
                    catch (const sw::redis::Error& e)
                    {
                        LOG_ERROR("RedisChatService: Subscribe thread error: {}.", e.what());
                    }
                    LOG_INFO("RedisChatService: Subscribe thread stopped.");
                });
            }

            void RedisChatService::Unsubscribe(const std::string& channel)
            {
                if (m_subscriber)
                {
                    m_subscriber->unsubscribe(channel);
                    LOG_INFO("RedisChatService: Unsubscribed from channel \'{}\'.", channel);
                }
            }
        }
    }
}