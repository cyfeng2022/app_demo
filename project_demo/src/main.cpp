#include <iostream>
#include "mqtt_test.h"
#include "logger.h"
#include "config_manager.h"

int main(int argc, char* argv[]) {
    // 加载配置文件
    ConfigManager configManager;
    bool configLoaded = configManager.loadConfig("config.json");
    
    if (configLoaded) {
        // 使用配置文件初始化日志系统
        const auto& loggerConfig = configManager.getLoggerConfig();
        Logger::getInstance().initWithConfig(
            loggerConfig.level,
            loggerConfig.console_output,
            loggerConfig.file_output,
            loggerConfig.log_path,
            loggerConfig.log_name,
            loggerConfig.max_file_size_mb,
            loggerConfig.max_files
        );
    } else {
        // 使用默认配置初始化日志系统
        Logger::getInstance().init("mqtt_client", "./logs");
        Logger::getInstance().setLevel(Logger::Level::Debug);
        std::cout << "[WARN] Config file not found, using default logger settings" << std::endl;
    }
    
    LOG_INFO("=== MQTT Client Test Program Started ===");
    
    int result = runMqttTests(argc, argv);
    
    LOG_INFO("=== Program Exited with code: {} ===", result);
    LOG_FLUSH();
    
    return result;
}
