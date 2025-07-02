cmake_minimum_required(VERSION 3.15)

set(CMAKE_TOOLCHAIN_FILE "C:/Users/clums/Desktop/vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "Vcpkg toolchain file")

project(CppMMO VERSION 1.0
               DESCRIPTION "My Cpp Project"
               LANGUAGES CXX)

# =========================[ 빌드 옵션 및 출력 경로 ]=========================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0077 NEW)
cmake_policy(SET CMP0167 NEW)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# =========================[ 실행 파일 및 소스 파일 ]=========================
add_executable(${PROJECT_NAME})

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_precompile_headers(${PROJECT_NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}/src/pch.h"
    )
    target_sources(${PROJECT_NAME} PRIVATE
        src/pch.cpp
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # 이 프로젝트는 일단 MSVC만 지원함 필요하다면 아래 주석을 해지할것.
    # target_precompile_headers(${PROJECT_NAME} PRIVATE pch.h)
    # target_sources(${PROJECT_NAME} PRIVATE src/pch.cpp)
endif()

target_sources(${PROJECT_NAME} PRIVATE
    src/main.cpp
    src/Utils/Logger.cpp
    src/Network/TcpServer.cpp
    src/Network/Session.cpp
    src/Network/PacketManager.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/src/Common/protocol_generated.h
)

# =========================[ 컴파일러 경고 옵션 ]=========================
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/W4 /utf-8)
    add_compile_definitions(
        _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING
        _CRT_SECURE_NO_WARNINGS # C4996과 관련된 다른 일반적인 경고도 억제
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# =========================[ 외부 라이브러리: Boost ]=========================
# list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/external/boost_install")

find_package(Boost REQUIRED COMPONENTS system thread program_options filesystem)

if (Boost_FOUND)
    message(STATUS "Boost found. Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
else()
    message(FATAL_ERROR "Boost not found at ${CMAKE_PREFIX_PATH}. Please ensure Boost is installed and CMAKE_PREFIX_PATH is set correctly.")
endif()

# =========================[ 외부 라이브러리: spdlog ]=========================
include(FetchContent)

FetchContent_Declare(
    spdlog_src
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.13.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(spdlog_src)

# =========================[ 외부 라이브러리: redis-plus-plus ]=========================
find_package(redis++ CONFIG REQUIRED)

# =========================[ 외부 라이브러리: FlatBuffers ]==========================
FetchContent_Declare(
    flatbuffers_src
    GIT_REPOSITORY https://github.com/google/flatbuffers.git
    GIT_TAG v24.3.25
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(flatbuffers_src)

target_link_libraries(${PROJECT_NAME} PRIVATE flatbuffers)

FetchContent_GetProperties(flatbuffers_src)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/src/Common")

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/src/Common/protocol_generated.h
    COMMAND $<TARGET_FILE:flatc>
            --cpp
            --gen-object-api
            -o "${CMAKE_CURRENT_BINARY_DIR}/src/Common/"
            -I "${CMAKE_SOURCE_DIR}/src/Common/"
            "${CMAKE_SOURCE_DIR}/src/Common/protocol.fbs"
    DEPENDS ${CMAKE_SOURCE_DIR}/src/Common/protocol.fbs
            flatc
    COMMENT "Generating FlatBuffers C++ code from protocol.fbs"
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/src/Common
)

add_custom_target(generate_protocol_fbs
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/src/Common/protocol_generated.h
)

add_dependencies(generate_protocol_fbs flatc)
add_dependencies(${PROJECT_NAME} generate_protocol_fbs)

# =========================[ 헤더 파일 포함 경로 ]=========================
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Utils
    ${CMAKE_SOURCE_DIR}/src/Network
)

# =========================[ 라이브러리 링크 ]=========================
target_link_libraries(${PROJECT_NAME} PRIVATE
    spdlog
    redis++::redis++
    flatbuffers
    Boost::system
    Boost::thread
    Boost::program_options
    Boost::filesystem
)

# =========================[ Doxygen 문서화 ]=========================
find_package(Doxygen QUIET)

if(DOXYGEN_FOUND)
    message(STATUS "Doxygen found. Generating documentation.")

    add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/docs/doxygen/html/index.html
        COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )

    add_custom_target(doc ALL
        DEPENDS ${CMAKE_SOURCE_DIR}/docs/doxygen/html/index.html
        COMMENT "Builds the Doxygen documentation"
    )
    
else()
    message(WARNING "Doxygen not found. API documentation will not be generated. Please install Doxygen and Graphviz.")
endif()