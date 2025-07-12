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
            ISession() = default;
            virtual ~ISession() = default;

            ISession(const ISession&) = delete;
            ISession& operator=(const ISession&) = delete;
            
            /**
             * @brief 세션을 시작하고 비동기 I/O 작업을 초기화합니다.
             * 
             * @details 이 메서드는 생성자와 별도로 호출되어 `shared_from_this()`를 안전하게 사용할 수 있도록 합니다.
             *          생성자에서는 `shared_from_this()`를 호출할 수 없으므로,
             *          안전한 세션 생명주기 관리 및 비동기 작업 시작을 위해 이 2단계 초기화 패턴이 사용됩니다.
             */
            virtual void Start() = 0;

            virtual void Disconnect() = 0;

            virtual boost::asio::ip::tcp::endpoint GetRemoteEndpoint() const = 0;

            virtual bool IsConnected() const = 0;

            virtual void Send(std::span<const std::byte> data) = 0;

            virtual void SetOnDisconnectedCallback(const std::function<void(std::shared_ptr<ISession>)>& callback) = 0;
            virtual uint64_t GetSessionId() const = 0;
            virtual uint64_t GetPlayerId() const = 0;
            virtual void SetPlayerId(uint64_t playerId) = 0;
        };
    }
}