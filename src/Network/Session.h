#pragma once
#include "pch.h"
#include "ISession.h"
#include "IPacketManager.h"
#include <span>
#include <cstddef>

namespace asio = boost::asio;
namespace ip = boost::asio::ip;

namespace CppMMO
{
    namespace Network
    {
        class Session : public ISession, public std::enable_shared_from_this<Session>
        {
        public:
            explicit Session(ip::tcp::socket socket, std::shared_ptr<IPacketManager> packetManager);
            virtual ~Session() = default;

            virtual void Start() override;
            virtual void Disconnect() override;
            virtual ip::tcp::endpoint GetRemoteEndpoint() const override;
            virtual bool IsConnected() const override;
            virtual void Send(std::span<const std::byte> data) override;

        private:
            ip::tcp::socket m_socket;
            std::shared_ptr<IPacketManager> m_packetManager;

            asio::streambuf m_readBuffer;
            std::array<std::byte, 4> m_packetHeader;

            std::deque<std::vector<std::byte>> m_writeQueue;
            bool m_writing;

            uint64_t m_sessionId;
            asio::steady_timer m_timer;

            std::chrono::steady_clock::time_point m_readDeadLine;
            std::chrono::steady_clock::time_point m_writeDeadLine;
            
            asio::awaitable<void> ReadLoop();
            asio::awaitable<void> WriteLoop();

            void HandleError(const boost::system::error_code& ec, const std::string& operation);

            static std::atomic<uint64_t> s_nextSessionId;
        };
    }
}