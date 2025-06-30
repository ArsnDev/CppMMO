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

target_sources(${PROJECT_NAME} PRIVATE
    src/main.cpp
    src/Utils/Logger.cpp
)

# =========================[ 컴파일러 경고 옵션 ]=========================
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/W4)
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

# =========================[ 라이브러리 링크 ]=========================
target_link_libraries(${PROJECT_NAME} PRIVATE
    spdlog
    redis++::redis++
    Boost::system
    Boost::thread
    Boost::program_options
    Boost::filesystem
)