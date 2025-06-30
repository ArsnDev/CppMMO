// src/main.cpp
#include <iostream>
#include <string>
#include <boost/system/error_code.hpp>
#include <boost/version.hpp>

#include "Utils/Logger.h"

// redis-plus-plus 헤더 포함
#include <sw/redis++/redis++.h>

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

    // redis-plus-plus 테스트 코드
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
    spdlog::info("All integrations test complete.");

    return 0;
}