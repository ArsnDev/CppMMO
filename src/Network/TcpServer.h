#pragma once
#include "pch.h"
#include "IService.h"
#include "IPacketManager.h" 
#include "ISessionManager.h"
#include "Session.h"

namespace asio = boost::asio;
namespace ip = boost::asio::ip;

namespace CppMMO
{
    namespace Network
    {
        class Session;

        class TcpServer : public IService, public std::enable_shared_from_this<TcpServer>
        {
        public:
            explicit TcpServer(asio::io_context& io_context, unsigned short port, 
                               std::shared_ptr<IPacketManager> packetManager,
                               std::shared_ptr<ISessionManager> sessionManager);
            virtual ~TcpServer();

            virtual bool Start(const ServiceConfig& config) override;
            virtual void Stop() override;

            virtual void SetOnSessionConnected(const std::function<void(std::shared_ptr<ISession>)>& callback) override;
            virtual void SetOnSessionDisconnected(const std::function<void(std::shared_ptr<ISession>)>& callback) override;

        private:
            asio::io_context& m_ioContext;
            asio::ip::tcp::acceptor m_acceptor;
            std::shared_ptr<IPacketManager> m_packetManager;
            std::shared_ptr<ISessionManager> m_sessionManager;

            std::function<void(std::shared_ptr<ISession>)> m_onSessionConnected{};
            std::function<void(std::shared_ptr<ISession>)> m_onSessionDisconnected{};

            std::vector<std::thread> m_workerThreads;
            asio::signal_set m_signals;
            asio::awaitable<void> AcceptLoop();
            void OnSessionDisconnectedInternal(std::shared_ptr<ISession> session);
        };
    }
}
