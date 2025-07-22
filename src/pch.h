#pragma once

// ========================[ Standard Libraries ]========================
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
#include <future>
#include <coroutine>

// ========================[ Boost Libraries ]========================
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

// ========================[ concurrentqueue Library ]========================
#include <concurrentqueue.h>

// ========================[ Spdlog Library ]========================
#include "Utils/Logger.h"

// ========================[ Redis++ Library ]========================
// Redis++ headers are included individually in files as needed
// #include <sw/redis++/redis++.h>

// ========================[ FlatBuffers Library ]========================
#include <flatbuffers/flatbuffers.h>
// Include "protocol_generated.h" directly in individual .cpp files as needed