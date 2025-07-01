#pragma once

// ========================[ 표준 라이브러리 ]========================
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <stdexcept>
#include <cstdint>

// ========================[ Boost 라이브러리 ]========================
#include <boost/system/error_code.hpp>
#include <boost/version.hpp>

#include <boost/asio.hpp>
#include <boost/asio/ts/buffer.hpp>
#include <boost/asio/ts/internet.hpp>

// ========================[ Spdlog 라이브러리 ]========================
#include "Utils/Logger.h"

// ========================[ Redis++ 라이브러리 ]========================
#include <sw/redis++/redis++.h>

// ========================[ FlatBuffers 라이브러리 ]========================
#include <flatbuffers/flatbuffers.h>
// 각 .cpp 파일에서 필요할 때 직접 #include "protocol_generated.h"