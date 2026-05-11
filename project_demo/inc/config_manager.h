#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <json/json.h>

struct MqttConfig {
    std::string broker = "tcp://localhost:1883";
    std::string client_id = "demo_client";
    std::string username = "";
    std::string password = "";
    int keep_alive_interval = 60;
    int timeout = 10000;
};

struct UartConfig {
    std::string port = "/dev/ttyS0";
    int baud_rate = 115200;
    int data_bits = 8;
    int stop_bits = 1;
    std::string parity = "none";
};

struct LoggerConfig {
    std::string level = "debug";
    bool console_output = true;
    bool file_output = true;
    std::string log_path = "./logs";
    std::string log_name = "app";
    int max_file_size_mb = 10;
    int max_files = 3;
};

struct AppConfig {
    MqttConfig mqtt;
    UartConfig uart;
    LoggerConfig logger;
};

class ConfigManager {
public:
    bool loadConfig(const std::string& filename);
    
    const AppConfig& getConfig() const { return config; }
    const MqttConfig& getMqttConfig() const { return config.mqtt; }
    const UartConfig& getUartConfig() const { return config.uart; }
    const LoggerConfig& getLoggerConfig() const { return config.logger; }

private:
    bool parseMqttConfig(const Json::Value& root);
    bool parseUartConfig(const Json::Value& root);
    bool parseLoggerConfig(const Json::Value& root);
    
    AppConfig config;
};

#endif
