cmake_minimum_required(VERSION 3.15)

project(CppMMO_Deployment VERSION 1.0
               DESCRIPTION "CppMMO Deployment Server"
               LANGUAGES CXX)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 빌드 출력 디렉토리 설정
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 실행 파일 생성
add_executable(${PROJECT_NAME})

# 컴파일러별 옵션
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /bigobj)
    target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Wno-maybe-uninitialized)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 소스 파일 추가
target_sources(${PROJECT_NAME} PRIVATE
    src/main.cpp
    src/pch.cpp
    src/Network/PacketManager.cpp
    src/Network/Session.cpp
    src/Network/SessionManager.cpp
    src/Network/TcpServer.cpp
    src/Game/Managers/ChatManager.cpp
    src/Game/Managers/GameManager.cpp
    src/Game/Models/Player.cpp
    src/Game/Models/World.cpp
    src/Game/Spatial/QuadTree.cpp
    src/Game/PacketHandlers/LoginPacketHandler.cpp
    src/Game/PacketHandlers/ChatPacketHandler.cpp
    src/Game/Services/AuthService.cpp
    src/Game/Services/RedisChatService.cpp
    src/Game/GameLogicQueue.cpp
    src/Utils/JobProcessor.cpp
    src/Utils/JobQueue.cpp
    src/Utils/Logger.cpp
)

# 헤더 디렉토리 설정
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Common
)

# 외부 라이브러리 찾기
find_package(Boost REQUIRED COMPONENTS system program_options)
find_package(Threads REQUIRED)
find_package(PkgConfig QUIET)

# spdlog 
find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
    find_path(SPDLOG_INCLUDE_DIR spdlog/spdlog.h)
    find_library(SPDLOG_LIB spdlog)
endif()

# FlatBuffers
find_package(Flatbuffers QUIET)
if(NOT Flatbuffers_FOUND)
    find_path(FLATBUFFERS_INCLUDE_DIR flatbuffers/flatbuffers.h)
    find_library(FLATBUFFERS_LIB flatbuffers)
endif()

# Redis++
find_library(REDISPP_LIB redis++)
find_library(HIREDIS_LIB hiredis)

# nlohmann-json
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp)
endif()

# concurrentqueue 헤더
find_path(CONCURRENTQUEUE_INCLUDE_DIR concurrentqueue.h)

# Include 디렉토리 추가
if(SPDLOG_INCLUDE_DIR)
    target_include_directories(${PROJECT_NAME} PRIVATE ${SPDLOG_INCLUDE_DIR})
endif()

if(FLATBUFFERS_INCLUDE_DIR)
    target_include_directories(${PROJECT_NAME} PRIVATE ${FLATBUFFERS_INCLUDE_DIR})
endif()

if(NLOHMANN_JSON_INCLUDE_DIR)
    target_include_directories(${PROJECT_NAME} PRIVATE ${NLOHMANN_JSON_INCLUDE_DIR})
endif()

if(CONCURRENTQUEUE_INCLUDE_DIR)
    target_include_directories(${PROJECT_NAME} PRIVATE ${CONCURRENTQUEUE_INCLUDE_DIR})
endif()

# 라이브러리 링크 (의존성 순서 중요)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::system
    Boost::program_options
    Threads::Threads
)

# Redis++ 라이브러리 (hiredis보다 먼저)
if(REDISPP_LIB)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${REDISPP_LIB})
endif()

if(HIREDIS_LIB)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${HIREDIS_LIB})
endif()

# spdlog 라이브러리
if(spdlog_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)
elseif(SPDLOG_LIB)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${SPDLOG_LIB})
endif()

# FlatBuffers 라이브러리
if(Flatbuffers_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE flatbuffers::flatbuffers)
elseif(FLATBUFFERS_LIB)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${FLATBUFFERS_LIB})
endif()

# nlohmann-json 라이브러리
if(nlohmann_json_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
endif()

# 플랫폼별 추가 링크 라이브러리
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 wsock32)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(${PROJECT_NAME} PRIVATE stdc++fs)
endif()

# 빌드 정보 출력
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Boost Version: ${Boost_VERSION}")

if(spdlog_FOUND OR SPDLOG_LIB)
    message(STATUS "spdlog: Found")
endif()

if(Flatbuffers_FOUND OR FLATBUFFERS_LIB)
    message(STATUS "FlatBuffers: Found")
endif()

if(REDISPP_LIB)
    message(STATUS "Redis++: Found")
endif()

if(CONCURRENTQUEUE_INCLUDE_DIR)
    message(STATUS "concurrentqueue: Found")
endif()

if(nlohmann_json_FOUND OR NLOHMANN_JSON_INCLUDE_DIR)
    message(STATUS "nlohmann-json: Found")
endif()