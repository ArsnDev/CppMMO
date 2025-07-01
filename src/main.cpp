// src/main.cpp
#include "pch.h" // <--- 모든 소스 파일의 가장 첫 줄에 포함되어야 합니다!

// pch.h에 포함되지 않은 (또는 PCH에 넣기 어려운) 헤더들
// FlatBuffers generated 헤더는 빌드 시 생성되므로 여기에 직접 포함합니다.
#include "protocol_generated.h"

// FlatBuffers 네임스페이스
using namespace CppMMO::Protocol;

int main() {
    // Logger 초기화: 프로그램 시작 시 단 한 번만 호출
    CppMMO::Utils::Logger::Init();

    LOG_INFO("Hello, CppMMO Server! (from spdlog via Logger singleton)");
    LOG_DEBUG("This is a debug message from spdlog.");
    LOG_WARN("This is a warning message. Pay attention!");
    LOG_ERROR("This is an error message. Something went wrong!");

    std::cout << "-----------------------" << std::endl;

    // Boost 버전 확인
    LOG_INFO("Using Boost version: {}.{}.{}",
                  BOOST_VERSION / 100000,
                  BOOST_VERSION / 100 % 1000,
                  BOOST_VERSION % 100);

    // Boost.System 기본 기능 확인
    boost::system::error_code ec;
    LOG_INFO("Boost.System error_code default value: {}: {}", ec.value(), ec.message());

    boost::system::error_code custom_ec(static_cast<int>(boost::system::errc::address_not_available),
                                        boost::system::system_category());
    LOG_WARN("Custom error_code value: {}: {}", custom_ec.value(), custom_ec.message());

    std::cout << "-----------------------" << std::endl;

    // redis-plus-plus 테스트 코드
    try {
        // Redis 서버가 실행 중이어야 합니다!
        auto redis = sw::redis::Redis("tcp://127.0.0.1:6379");
        LOG_INFO("Successfully connected to Redis server at 127.0.0.1:6379.");

        redis.set("mykey", "Hello from Redis++!");
        auto val = redis.get("mykey");
        if (val) {
            LOG_INFO("Retrieved 'mykey': {}", *val);
        } else {
            LOG_ERROR("Failed to retrieve 'mykey'.");
        }

        long long deleted_count = redis.del("mykey");
        LOG_INFO("Deleted 'mykey', count: {}", deleted_count);

        redis.ping();
        LOG_INFO("Redis PING successful.");

    } catch (const sw::redis::Error &err) {
        LOG_CRITICAL("Redis connection or command failed: {}", err.what());
    }

    std::cout << "-----------------------" << std::endl;


    // =====================================================================
    // FlatBuffers 직렬화/역직렬화 테스트 코드
    // =====================================================================
    try {
        flatbuffers::FlatBufferBuilder builder(1024);

        auto username_str = builder.CreateString("PCH_TestUser");
        auto enter_room_request = CreateC_EnterRoom(builder, 54321L, username_str);

        auto unified_packet = CreateUnifiedPacket(builder,
            PacketId_C_ENTER_ROOM,
            AnyMessage_C_EnterRoom,
            enter_room_request.Union()
        );
        builder.Finish(unified_packet);

        uint8_t *buffer_pointer = builder.GetBufferPointer();
        size_t buffer_size = builder.GetSize();

        LOG_INFO("FlatBuffers Serialization: C_EnterRoom packet created.");
        LOG_INFO("  Buffer Size: {} bytes", buffer_size);

        flatbuffers::Verifier verifier(buffer_pointer, buffer_size);
        if (VerifyUnifiedPacketBuffer(verifier)) {
            LOG_INFO("FlatBuffers Verification: Buffer is valid.");
            const UnifiedPacket* received_packet = GetUnifiedPacket(buffer_pointer);

            LOG_INFO("  Received Packet ID: {}", static_cast<int>(received_packet->id()));

            if (received_packet->message_data_type() == AnyMessage_C_EnterRoom) {
                const C_EnterRoom* received_request = received_packet->message_data_as_C_EnterRoom();
                if (received_request) {
                    LOG_INFO("  Received C_EnterRoom - Room ID: {}, User Name: {}",
                                 received_request->room_id(),
                                 received_request->username()->c_str());
                }
            } else {
                LOG_WARN("  Received packet is not C_EnterRoom type as expected.");
            }
        } else {
            LOG_ERROR("FlatBuffers Verification: Buffer is INVALID!");
        }

        // S_PlayerMoved 메시지 직렬화 예시
        LOG_INFO("FlatBuffers Serialization: S_PlayerMoved packet created.");
        flatbuffers::FlatBufferBuilder builder2(1024);
        auto vec2_pos = Vec2(100.0f, 200.0f);
        auto player_moved = CreateS_PlayerMoved(builder2,
                                               98765L,
                                               &vec2_pos,
                                               180.0f);

        auto unified_packet2 = CreateUnifiedPacket(builder2,
            PacketId_S_PLAYER_MOVED,
            AnyMessage_S_PlayerMoved,
            player_moved.Union()
        );
        builder2.Finish(unified_packet2);

        uint8_t *buffer_pointer2 = builder2.GetBufferPointer();
        size_t buffer_size2 = builder2.GetSize();

        LOG_INFO("  Buffer Size: {} bytes", buffer_size2);

        flatbuffers::Verifier verifier2(buffer_pointer2, buffer_size2);
        if (VerifyUnifiedPacketBuffer(verifier2)) {
            LOG_INFO("FlatBuffers Verification: PlayerMoved buffer is valid.");
            const UnifiedPacket* received_packet2 = GetUnifiedPacket(buffer_pointer2);

            LOG_INFO("  Received Packet ID: {}", static_cast<int>(received_packet2->id()));
            if (received_packet2->message_data_type() == AnyMessage_S_PlayerMoved) {
                const S_PlayerMoved* received_move = received_packet2->message_data_as_S_PlayerMoved();
                if (received_move) {
                    LOG_INFO("  Received S_PlayerMoved - Player ID: {}, Position: ({}, {}), Rotation: {}",
                                 received_move->player_id(),
                                 received_move->new_position()->x(),
                                 received_move->new_position()->y(),
                                 received_move->new_rotation());
                }
            } else {
                LOG_WARN("  Received packet is not S_PlayerMoved type as expected.");
            }
        } else {
            LOG_ERROR("FlatBuffers Verification: PlayerMoved buffer is INVALID!");
        }


    } catch (const std::exception& e) {
        LOG_CRITICAL("FlatBuffers test failed: {}", e.what());
    }


    std::cout << "-----------------------" << std::endl;
    LOG_INFO("All integrations test complete."); // 로거 매크로 사용

    return 0;
}