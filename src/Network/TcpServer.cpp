#include "pch.h"
#include "TcpServer.h"

namespace asio = boost::asio;
namespace ip = boost::asio::ip;

namespace CppMMO
{
    namespace Network
    {
        TcpServer::TcpServer(asio::io_context& io_context,
                            unsigned short port,
                            std::shared_ptr<IPacketManager> packetManager)
                            :m_ioContext(io_context),
                            m_acceptor(io_context, ip::tcp::endpoint(ip::tcp::v4(), port)),
                            m_packetManager(packetManager),
                            m_signals(io_context, SIGINT, SIGTERM)
        {
            LOG_INFO("TcpServer Created. Listening on port {}", port);

            m_signals.async_wait([this](const boost::system::error_code& ec, int signal_number)
            {
                if (!ec) 
                {
                    LOG_INFO("Received signal {}. Stopping server...", signal_number);
                    m_ioContext.stop();
                }
            });
        }

        TcpServer::~TcpServer()
        {
            Stop();
            LOG_INFO("TcpServer Closed.");
        }

        bool TcpServer::Start(const ServiceConfig& config)
        {
            try
            {
                if (config.worker_threads <= 0)
                {
                    LOG_ERROR("TcpServer Start failed: worker_threads must be greater than 0.");
                    return false;
                }

                m_acceptor.listen(asio::socket_base::max_connections);
                LOG_INFO("TcpServer started listening with backlog {}.", asio::socket_base::max_listen_connections);

                asio::co_spawn(m_ioContext, AcceptLoop(), asio::detached);

                for(int i=0; i<config.worker_threads; ++i)
                {
                    m_workerThreads.emplace_back([this]()
                    {
                        m_ioContext.run();
                    });
                    LOG_INFO("WorkerThread {} started.", i+1);
                }
                return true;
            }
            catch(const boost::system::system_error& e)
            {
                LOG_ERROR("TcpServer Start failed: {}", e.what());
                return false;
            }
            catch(const std::exception& e)
            {
                LOG_CRITICAL("TcpServer Start failed with unexpected exception: {}", e.what());
                return false;
            }
            
        }

        void TcpServer::Stop()
        {
            if (!m_ioContext.stopped())
            {
                m_ioContext.stop();
                LOG_INFO("TcpServer stopping io_context.");
            }
            for (std::thread& thread : m_workerThreads)
            {
                if (thread.joinable())
                {
                    thread.join();
                }
            }
            m_workerThreads.clear();
            LOG_INFO("TcpServer stopped and all worker threads joined.");
        }

        void TcpServer::SetOnSessionConnected(const std::function<void(std::shared_ptr<ISession>)>& callback)
        {
            m_onSessionConnected = callback;
            LOG_DEBUG("OnSessionConnected callback set.");
        }

        void TcpServer::SetOnSessionDisconnected(const std::function<void(std::shared_ptr<ISession>)>& callback)
        {
            m_onSessionDisconnected = callback;
            LOG_DEBUG("OnSessionDisconnected callback set.");
        }

        asio::awaitable<void> TcpServer::AcceptLoop()
        {
            try
            {
                while(true)
                {
                    ip::tcp::socket socket(m_ioContext);
                    co_await m_acceptor.async_accept(socket, asio::use_awaitable);
                    LOG_INFO("New connection accepted from {}", socket.remote_endpoint().address().to_string());
                    auto session = std::make_shared<Session>(std::move(socket), m_packetManager);
                    session->SetOnDisconnectedCallback([self = shared_from_this()](std::shared_ptr<ISession> session)
                    {
                        self->OnSessionDisconnectedInternal(session);
                    });
                    session->Start();
                    if (m_onSessionConnected)
                    {
                        m_onSessionConnected(session);
                    }
                }
            }
            catch (const boost::system::system_error& e)
            {
                if (e.code() == asio::error::operation_aborted || e.code() == asio::error::bad_descriptor)
                {
                    LOG_INFO("AcceptLoop aborted gracefully: {}", e.what());
                }
                else
                {
                    LOG_ERROR("AcceptLoop error: {}", e.what());
                }
            }
            catch(const std::exception& e)
            {
                LOG_CRITICAL("AcceptLoop unexpected exception: {}", e.what());
            }
            co_return;
        }

        void TcpServer::OnSessionDisconnectedInternal(const std::shared_ptr<ISession>& session)
        {
            LOG_INFO("Session disconnected.");
            if(m_onSessionDisconnected)
            {
                m_onSessionDisconnected(session);
            }
        }
    }
}