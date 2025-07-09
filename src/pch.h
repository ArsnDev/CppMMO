#pragma once

// ========================[ 표준 라이브러리 ]========================
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <array>
#include <deque>
#include <memory>
#include <optional>
#include <stdexcept>
#include <cstdint>
#include <cstddef>
#include <atomic>
#include <chrono>
#include <span>
#include <mutex>
#include <functional>
#include <thread>

// ========================[ Boost 라이브러리 ]========================
#include <boost/system/error_code.hpp>
#include <boost/version.hpp>

#include <boost/asio.hpp>
#include <boost/asio/ts/buffer.hpp>
#include <boost/asio/ts/internet.hpp>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp> 
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>

// ========================[ concurrentqueue 라이브러리 ]========================
#include <concurrentqueue.h>

// ========================[ Spdlog 라이브러리 ]========================
#include "Utils/Logger.h"

// ========================[ Redis++ 라이브러리 ]========================
#include <sw/redis++/redis++.h>

// ========================[ FlatBuffers 라이브러리 ]========================
#include <flatbuffers/flatbuffers.h>
// 각 .cpp 파일에서 필요할 때 직접 #include "protocol_generated.h"