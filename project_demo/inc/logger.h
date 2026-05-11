#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

class Logger {
public:
    enum class Level {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warn = 3,
        Error = 4,
        Critical = 5
    };

    static Logger& getInstance();

    void init(const std::string& appName, const std::string& logPath = "./logs");
    
    void initWithConfig(const std::string& level, 
                        bool consoleOutput, 
                        bool fileOutput, 
                        const std::string& logPath, 
                        const std::string& logName,
                        int maxFileSizeMb = 10,
                        int maxFiles = 3);

    void setLevel(Level level);

    void setConsoleEnabled(bool enabled);

    void setFileEnabled(bool enabled);

    template<typename... Args>
    void trace(const char* fmt, Args&&... args) {
        if (traceLogger_) traceLogger_->trace(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debug(const char* fmt, Args&&... args) {
        if (debugLogger_) debugLogger_->debug(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(const char* fmt, Args&&... args) {
        if (infoLogger_) infoLogger_->info(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(const char* fmt, Args&&... args) {
        if (warnLogger_) warnLogger_->warn(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(const char* fmt, Args&&... args) {
        if (errorLogger_) errorLogger_->error(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void critical(const char* fmt, Args&&... args) {
        if (criticalLogger_) criticalLogger_->critical(fmt, std::forward<Args>(args)...);
    }

    void flush();

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::shared_ptr<spdlog::logger> traceLogger_;
    std::shared_ptr<spdlog::logger> debugLogger_;
    std::shared_ptr<spdlog::logger> infoLogger_;
    std::shared_ptr<spdlog::logger> warnLogger_;
    std::shared_ptr<spdlog::logger> errorLogger_;
    std::shared_ptr<spdlog::logger> criticalLogger_;

    bool consoleEnabled_ = true;
    bool fileEnabled_ = true;
    std::string appName_;
};

#define LOG_TRACE(...) Logger::getInstance().trace(__VA_ARGS__)
#define LOG_DEBUG(...) Logger::getInstance().debug(__VA_ARGS__)
#define LOG_INFO(...) Logger::getInstance().info(__VA_ARGS__)
#define LOG_WARN(...) Logger::getInstance().warn(__VA_ARGS__)
#define LOG_ERROR(...) Logger::getInstance().error(__VA_ARGS__)
#define LOG_CRITICAL(...) Logger::getInstance().critical(__VA_ARGS__)

#define LOG_FLUSH() Logger::getInstance().flush()

#endif