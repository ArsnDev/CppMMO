#pragma once
#include "pch.h"
#include <span>
#include <cstddef>
#include <functional>
#include "protocol_generated.h"

namespace CppMMO
{
    namespace Network
    {
        class ISession;
        
        // FlatBuffers에서 생성된 PacketId enum을 패킷 식별자로 사용합니다.
        using PacketId = Protocol::PacketId;
        /**
         *  @brief 패킷 핸들러 함수의 타입을 정의합니다.
         *  @param std::shared_ptr<ISession> 패킷을 수신한 세션의 포인터입니다.
         *  @param const Protocol::AnyMessage& FlatBuffers의 union 타입으로, 실제 패킷 데이터입니다.
         */
        using PacketHandler = std::function<void(std::shared_ptr<ISession>, const Protocol::UnifiedPacket*)>;

        class IPacketManager
        {
        public:
            virtual ~IPacketManager() = default;

            IPacketManager(const IPacketManager&) = delete;
            IPacketManager& operator=(const IPacketManager&) = delete;
            
            /**
             *  @brief 특정 패킷 ID에 대한 핸들러 함수를 등록합니다.
             *  @param id 처리할 패킷의 ID입니다.
             *  @param handler 해당 ID의 패킷이 수신되었을 때 호출될 함수입니다.
             */
            virtual void RegisterHandler(PacketId id, PacketHandler handler) = 0;
            /**
             *  @brief 등록된 핸들러를 제거합니다.
             *  @param id 제거할 핸들러가 등록된 패킷의 ID입니다.
             */
            virtual void UnregisterHandler(PacketId id) = 0;
            /**
             *  @brief 수신된 패킷을 분석하여 등록된 핸들러에 전달합니다.
             *  @param session 패킷을 수신한 세션입니다.
             *  @param packet FlatBuffers를 통해 파싱된 최상위 패킷 객체입니다.
             */
            virtual void HandlePacket(std::shared_ptr<ISession> session, const Protocol::UnifiedPacket* packet) = 0;
        };
    }
}
