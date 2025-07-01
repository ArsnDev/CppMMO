#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <string>
#include <vector>
#include <memory>
#include <iostream>

namespace CppMMO{
    namespace Utils
    {
        class Logger
        {
            private:
                static std::shared_ptr<spdlog::logger> s_logger;

                Logger() = default;
                ~Logger() = default;

                Logger(const Logger&) = delete;
                Logger& operator=(const Logger&) = delete;

            public:
                static void Init()
                {
                    if(s_logger)
                    {
                        return;
                    }
                    try
                    {
                        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                        console_sink->set_level(spdlog::level::info);
                        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");
                    
                        auto daily_file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>("logs/server.log", 0, 0, false, 30);
                        daily_file_sink->set_level(spdlog::level::debug);
                        daily_file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] [%s:%#] %v");
                    
                        std::vector<spdlog::sink_ptr> sinks {console_sink, daily_file_sink};
                        s_logger = std::make_shared<spdlog::logger>("CppMMO_Logger", sinks.begin(), sinks.end());
                        s_logger->set_level(spdlog::level::debug);
                        spdlog::set_default_logger(s_logger);
                        spdlog::flush_on(spdlog::level::info);

                        spdlog::info("Logger initialized successfully.");
                    }
                    catch (const spdlog::spdlog_ex& ex)
                    {
                        std::cerr << "spdlog initialization failed: " << ex.what() << std::endl;
                    }
                }
                static std::shared_ptr<spdlog::logger>& Get()
                {
                    return s_logger;
                }
        };
    }
}

#define LOG_TRACE(...)    ::CppMMO::Utils::Logger::Get()->trace(__VA_ARGS__)
#define LOG_DEBUG(...)    ::CppMMO::Utils::Logger::Get()->debug(__VA_ARGS__)
#define LOG_INFO(...)     ::CppMMO::Utils::Logger::Get()->info(__VA_ARGS__)
#define LOG_WARN(...)     ::CppMMO::Utils::Logger::Get()->warn(__VA_ARGS__)
#define LOG_ERROR(...)    ::CppMMO::Utils::Logger::Get()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::CppMMO::Utils::Logger::Get()->critical(__VA_ARGS__)