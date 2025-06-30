cmake_minimum_required(VERSION 3.15)

project(CppMMO VERSION 1.0
               DESCRIPTION "My Cpp Project"
               LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

add_executable(${PROJECT_NAME} src/main.cpp)

include_directories(
    ${CMAKE_SOURCE_DIR}/external

)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/W4)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(Boost_ROOT "${CMAKE_SOURCE_DIR}/external/boost_install")
find_package(Boost 1.70 COMPONENTS system thread program_options filesystem REQUIRED)

if (Boost_FOUND)
    message(STATUS "Boost found. Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")

    target_link_libraries(${PROJECT_NAME} PRIVATE
        Boost::system
        Boost::thread
        Boost::program_options
        Boost::filesystem
    )
else()
    message(FATAL_ERROR "Boost not found at ${Boost_ROOT}. Please ensure Boost is installed and BOOST_ROOT is set correctly.")
endif()