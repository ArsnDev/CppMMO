#pragma once

namespace CppMMO
{
    namespace Network
    {
        class ISession;

        struct ServiceConfig
        {
            std::string host;
            unsigned short port;
            int worker_threads;
        };

        class IService
        {
        public:
            IService() = default;
            virtual ~IService() = default;

            IService(const IService&) = delete;
            IService& operator=(const IService&) = delete;

            virtual bool Start(const ServiceConfig& config) = 0;
            virtual void Stop() = 0;

            virtual void SetOnSessionConnected(const std::function<void(std::shared_ptr<ISession>)>& callback) = 0;
            virtual void SetOnSessionDisconnected(const std::function<void(std::shared_ptr<ISession>)>& callback) = 0;
        };
    }
}
