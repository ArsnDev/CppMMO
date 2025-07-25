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
            explicit Session(ip::tcp::socket socket, const std::shared_ptr<IPacketManager> packetManager);
            virtual ~Session() = default;

            virtual void Start() override;
            virtual void Disconnect() override;
            virtual ip::tcp::endpoint GetRemoteEndpoint() const override;
            virtual bool IsConnected() const override;
            virtual void Send(std::span<const std::byte> data) override;
            virtual void SendBatch(const std::vector<std::span<const std::byte>>& packets) override;

            virtual void SetOnDisconnectedCallback(const std::function<void(std::shared_ptr<ISession>)>& callback) override;
            virtual uint64_t GetSessionId() const override { return m_sessionId; }
            virtual uint64_t GetPlayerId() const override;
            virtual void SetPlayerId(uint64_t playerId) override;
        private:
            ip::tcp::socket m_socket;
            std::shared_ptr<IPacketManager> m_packetManager;

            std::array<std::byte, 4> m_readHeader;
            std::vector<std::byte> m_readBody;

            moodycamel::ConcurrentQueue<std::vector<std::byte>> m_writeQueue;

            asio::steady_timer m_timer;
            uint64_t m_sessionId;
            uint64_t m_playerId = 0;

            // TODO : Set Timeout For Read & Write
            // std::chrono::steady_clock::time_point m_readDeadline;
            // std::chrono::steady_clock::time_point m_writeDeadline;
            
            std::function<void(std::shared_ptr<ISession>)> m_onDisconnectedCallback{};
            
            asio::awaitable<void> ReadLoop();
            asio::awaitable<void> WriteLoop();

            void HandleError(const boost::system::error_code& ec, std::string_view operation);

            static std::atomic<uint64_t> s_nextSessionId;
        };
    }
}