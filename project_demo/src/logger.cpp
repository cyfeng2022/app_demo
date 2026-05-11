#include "logger.h"
#include <iostream>
#include <algorithm>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <iomanip>
#include <ctime>

Logger::~Logger() {
    spdlog::drop_all();
}

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

static std::string getTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << (localTime->tm_year + 1900)
        << std::setw(2) << std::setfill('0') << (localTime->tm_mon + 1)
        << std::setw(2) << std::setfill('0') << localTime->tm_mday
        << "_"
        << std::setw(2) << std::setfill('0') << localTime->tm_hour
        << std::setw(2) << std::setfill('0') << localTime->tm_min
        << std::setw(2) << std::setfill('0') << localTime->tm_sec;
    return oss.str();
}

static spdlog::level::level_enum stringToLevel(const std::string& levelStr) {
    if (levelStr == "trace") return spdlog::level::trace;
    if (levelStr == "debug") return spdlog::level::debug;
    if (levelStr == "info") return spdlog::level::info;
    if (levelStr == "warn") return spdlog::level::warn;
    if (levelStr == "error") return spdlog::level::err;
    if (levelStr == "critical") return spdlog::level::critical;
    return spdlog::level::debug;
}

void Logger::init(const std::string& appName, const std::string& logPath) {
    appName_ = appName;

    try {
        // 初始化线程池（全局共享）
        static std::once_flag tpInitFlag;
        static std::shared_ptr<spdlog::details::thread_pool> threadPool;
        std::call_once(tpInitFlag, []() {
            threadPool = std::make_shared<spdlog::details::thread_pool>(8192, 1);
            spdlog::details::registry::instance().set_tp(threadPool);
        });

        std::vector<spdlog::sink_ptr> sinks;

        if (consoleEnabled_) {
            auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            consoleSink->set_level(spdlog::level::trace);
            consoleSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%L%$] [%n] %v");
            sinks.push_back(consoleSink);
        }

        if (fileEnabled_) {
            auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                logPath + "/" + appName + ".log",
                1024 * 1024 * 10,
                3
            );
            fileSink->set_level(spdlog::level::trace);
            fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%L] [%t] %v");
            sinks.push_back(fileSink);
        }

        // 使用异步日志器（非阻塞模式）
        static auto asyncLogger = std::make_shared<spdlog::async_logger>(
            "async",
            sinks.begin(),
            sinks.end(),
            std::weak_ptr<spdlog::details::thread_pool>(threadPool),
            spdlog::async_overflow_policy::overrun_oldest
        );
        spdlog::details::registry::instance().initialize_logger(asyncLogger);

        traceLogger_ = asyncLogger;
        debugLogger_ = asyncLogger;
        infoLogger_ = asyncLogger;
        warnLogger_ = asyncLogger;
        errorLogger_ = asyncLogger;
        criticalLogger_ = asyncLogger;

        spdlog::set_default_logger(asyncLogger);

    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
    }
}

void Logger::initWithConfig(const std::string& level, 
                            bool consoleOutput, 
                            bool fileOutput, 
                            const std::string& logPath, 
                            const std::string& logName,
                            int maxFileSizeMb,
                            int maxFiles) {
    appName_ = logName;
    consoleEnabled_ = consoleOutput;
    fileEnabled_ = fileOutput;

    try {
        // 初始化线程池（全局共享）
        static std::once_flag tpInitFlag;
        static std::shared_ptr<spdlog::details::thread_pool> threadPool;
        std::call_once(tpInitFlag, []() {
            threadPool = std::make_shared<spdlog::details::thread_pool>(8192, 1);
            spdlog::details::registry::instance().set_tp(threadPool);
        });

        std::vector<spdlog::sink_ptr> sinks;

        if (consoleOutput) {
            auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            consoleSink->set_level(spdlog::level::trace);
            consoleSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%L%$] [%n] %v");
            sinks.push_back(consoleSink);
        }

        if (fileOutput) {
            std::string fileName = logPath + "/" + logName + "_" + getTimestamp() + ".log";
            auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                fileName,
                1024 * 1024 * maxFileSizeMb,
                maxFiles
            );
            fileSink->set_level(spdlog::level::trace);
            fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%L] [%t] %v");
            sinks.push_back(fileSink);
        }

        // 使用异步日志器（非阻塞模式）
        static auto asyncLogger = std::make_shared<spdlog::async_logger>(
            "async",
            sinks.begin(),
            sinks.end(),
            std::weak_ptr<spdlog::details::thread_pool>(threadPool),
            spdlog::async_overflow_policy::overrun_oldest
        );
        spdlog::details::registry::instance().initialize_logger(asyncLogger);

        traceLogger_ = asyncLogger;
        debugLogger_ = asyncLogger;
        infoLogger_ = asyncLogger;
        warnLogger_ = asyncLogger;
        errorLogger_ = asyncLogger;
        criticalLogger_ = asyncLogger;

        spdlog::set_default_logger(asyncLogger);
        
        // 设置日志级别
        auto spdLevel = stringToLevel(level);
        asyncLogger->set_level(spdLevel);

    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
    }
}

void Logger::setLevel(Level level) {
    auto spdLevel = static_cast<spdlog::level::level_enum>(level);
    if (traceLogger_) traceLogger_->set_level(spdLevel);
    if (debugLogger_) debugLogger_->set_level(spdLevel);
    if (infoLogger_) infoLogger_->set_level(spdLevel);
    if (warnLogger_) warnLogger_->set_level(spdLevel);
    if (errorLogger_) errorLogger_->set_level(spdLevel);
    if (criticalLogger_) criticalLogger_->set_level(spdLevel);
}

void Logger::setConsoleEnabled(bool enabled) {
    consoleEnabled_ = enabled;
}

void Logger::setFileEnabled(bool enabled) {
    fileEnabled_ = enabled;
}

void Logger::flush() {
    if (traceLogger_) traceLogger_->flush();
    if (debugLogger_) debugLogger_->flush();
    if (infoLogger_) infoLogger_->flush();
    if (warnLogger_) warnLogger_->flush();
    if (errorLogger_) errorLogger_->flush();
    if (criticalLogger_) criticalLogger_->flush();
    spdlog::apply_all([](std::shared_ptr<spdlog::logger> logger){ logger->flush(); });
}