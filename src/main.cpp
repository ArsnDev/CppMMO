// src/main.cpp
#include <iostream>
#include <string>
#include <vector> // std::vector 사용
#include <optional> // std::optional 사용

#include <boost/system/error_code.hpp>
#include <boost/version.hpp>

#include "Utils/Logger.h" // Logger 헤더 포함

// redis-plus-plus 헤더 포함
#include <sw/redis++/redis++.h>

// FlatBuffers generated 헤더 포함
// 이 파일은 CMake가 out/build/x64-debug/src/Common/ 에 생성합니다.
#include "protocol_generated.h" // 경로는 CMake의 target_include_directories에 의해 찾아집니다.

// FlatBuffers 네임스페이스
using namespace CppMMO::Protocol;

int main() {

    spdlog::info("Hello, CppMMO Server! (from spdlog)");
    spdlog::debug("This is a debug message from spdlog in main.cpp.");
    spdlog::warn("This is a warning message. Pay attention!");
    spdlog::error("This is an error message. Something went wrong!");

    std::cout << "-----------------------" << std::endl;

    // Boost 버전 확인 (기존 코드)
    spdlog::info("Using Boost version: {}.{}.{}",
                  BOOST_VERSION / 100000,
                  BOOST_VERSION / 100 % 1000,
                  BOOST_VERSION % 100);

    // Boost.System 기본 기능 확인 (기존 코드)
    boost::system::error_code ec;
    spdlog::info("Boost.System error_code default value: {}: {}", ec.value(), ec.message());

    boost::system::error_code custom_ec(static_cast<int>(boost::system::errc::address_not_available),
                                        boost::system::system_category());
    spdlog::warn("Custom error_code value: {}: {}", custom_ec.value(), custom_ec.message());

    std::cout << "-----------------------" << std::endl;

    // redis-plus-plus 테스트 코드 (기존 코드)
    try {
        // Redis 인스턴스 생성 (기본 로컬호스트:6379 연결 시도)
        // Redis 서버가 실행 중이어야 합니다!
        auto redis = sw::redis::Redis("tcp://127.0.0.1:6379");
        spdlog::info("Successfully connected to Redis server at 127.0.0.1:6379.");

        // SET/GET 명령어 테스트
        redis.set("mykey", "Hello from Redis++!");
        auto val = redis.get("mykey");
        if (val) {
            spdlog::info("Retrieved 'mykey': {}", *val);
        } else {
            spdlog::error("Failed to retrieve 'mykey'.");
        }

        // DEL 명령어 테스트
        long long deleted_count = redis.del("mykey");
        spdlog::info("Deleted 'mykey', count: {}", deleted_count);

        // PING 명령어 테스트
        redis.ping();
        spdlog::info("Redis PING successful.");

    } catch (const sw::redis::Error &err) {
        spdlog::critical("Redis connection or command failed: {}", err.what());
    }

    std::cout << "-----------------------" << std::endl;


    // =====================================================================
    // FlatBuffers 직렬화/역직렬화 테스트 코드
    // =====================================================================
    try {
        // 1. 빌더 생성 (직렬화를 위한 FlatBuffers 객체)
        flatbuffers::FlatBufferBuilder builder(1024); // 초기 버퍼 크기 1KB

        // 2. 'C_EnterRoom' 메시지 직렬화 예시
        auto username = builder.CreateString("TestUser");
        auto enter_room_request = CreateC_EnterRoom(builder, 12345L, username);

        // 3. 'UnifiedPacket' 생성 (루트 테이블)
        // PacketId와 message_data(union)를 사용하여 패킷을 만듭니다.
        auto unified_packet = CreateUnifiedPacket(builder,
            PacketId_C_ENTER_ROOM, // Packet ID
            AnyMessage_C_EnterRoom, // Union 타입 (자동 생성됨)
            enter_room_request.Union() // 실제 데이터 (union으로 변환)
        );

        // 4. 빌더가 FlatBuffer를 완료하도록 지시
        builder.Finish(unified_packet);

        // 5. 직렬화된 데이터 얻기
        uint8_t *buffer_pointer = builder.GetBufferPointer();
        size_t buffer_size = builder.GetSize();

        spdlog::info("FlatBuffers Serialization: C_EnterRoom packet created.");
        spdlog::info("  Buffer Size: {} bytes", buffer_size);

        // 6. 역직렬화 (받은 바이트 배열로부터 데이터 읽기)
        // VerifyFlatBuffer를 사용하여 데이터 유효성을 검사하는 것이 좋습니다.
        flatbuffers::Verifier verifier(buffer_pointer, buffer_size);
        if (VerifyUnifiedPacketBuffer(verifier)) { // protocol_generated.h에 생성된 verify 함수
            spdlog::info("FlatBuffers Verification: Buffer is valid.");

            // 루트 객체 가져오기
            const UnifiedPacket* received_packet = GetUnifiedPacket(buffer_pointer);

            // 패킷 ID 확인
            spdlog::info("  Received Packet ID: {}", static_cast<int>(received_packet->id()));

            // union 타입 확인 및 데이터 역직렬화
            if (received_packet->message_data_type() == AnyMessage_C_EnterRoom) {
                const C_EnterRoom* received_request = received_packet->message_data_as_C_EnterRoom();
                if (received_request) {
                    spdlog::info("  Received C_EnterRoom - Room ID: {}, User Name: {}",
                                 received_request->room_id(),
                                 received_request->username()->c_str());
                }
            } else {
                spdlog::warn("  Received packet is not C_EnterRoom type as expected.");
            }
        } else {
            spdlog::error("FlatBuffers Verification: Buffer is INVALID!");
        }

        // 7. S_PlayerMoved 메시지 직렬화 예시 (Vec2 포함)
        spdlog::info("FlatBuffers Serialization: S_PlayerMoved packet created.");
        flatbuffers::FlatBufferBuilder builder2(1024);
        auto vec2_pos = Vec2(10.5f, 20.3f); // struct는 직접 생성 (new 없이)
        auto player_moved = CreateS_PlayerMoved(builder2,
                                               54321L, // player_id
                                               &vec2_pos, // struct는 포인터로 전달
                                               90.0f); // new_rotation

        auto unified_packet2 = CreateUnifiedPacket(builder2,
            PacketId_S_PLAYER_MOVED,
            AnyMessage_S_PlayerMoved,
            player_moved.Union()
        );
        builder2.Finish(unified_packet2);

        uint8_t *buffer_pointer2 = builder2.GetBufferPointer();
        size_t buffer_size2 = builder2.GetSize();

        spdlog::info("  Buffer Size: {} bytes", buffer_size2);

        flatbuffers::Verifier verifier2(buffer_pointer2, buffer_size2);
        if (VerifyUnifiedPacketBuffer(verifier2)) {
            spdlog::info("FlatBuffers Verification: PlayerMoved buffer is valid.");
            const UnifiedPacket* received_packet2 = GetUnifiedPacket(buffer_pointer2);

            spdlog::info("  Received Packet ID: {}", static_cast<int>(received_packet2->id()));
            if (received_packet2->message_data_type() == AnyMessage_S_PlayerMoved) {
                const S_PlayerMoved* received_move = received_packet2->message_data_as_S_PlayerMoved();
                if (received_move) {
                    spdlog::info("  Received S_PlayerMoved - Player ID: {}, Position: ({}, {}), Rotation: {}",
                                 received_move->player_id(),
                                 received_move->new_position()->x(),
                                 received_move->new_position()->y(),
                                 received_move->new_rotation());
                }
            } else {
                spdlog::warn("  Received packet is not S_PlayerMoved type as expected.");
            }
        } else {
            spdlog::error("FlatBuffers Verification: PlayerMoved buffer is INVALID!");
        }


    } catch (const std::exception& e) {
        spdlog::critical("FlatBuffers test failed: {}", e.what());
    }


    std::cout << "-----------------------" << std::endl;
    spdlog::info("All integrations test complete.");

    return 0;
}