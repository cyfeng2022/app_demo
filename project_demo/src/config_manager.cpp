#include "config_manager.h"
#include <fstream>

bool ConfigManager::loadConfig(const std::string& filename) {
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        return false;
    }
    
    Json::CharReaderBuilder builder;
    Json::Value root;
    std::string errs;
    
    if (!Json::parseFromStream(builder, ifs, &root, &errs)) {
        return false;
    }
    
    parseMqttConfig(root);
    parseUartConfig(root);
    parseLoggerConfig(root);
    
    return true;
}

bool ConfigManager::parseMqttConfig(const Json::Value& root) {
    if (!root.isMember("mqtt")) {
        return false;
    }
    
    Json::Value mqttConfig = root["mqtt"];
    
    if (mqttConfig.isMember("broker") && mqttConfig["broker"].isString()) {
        config.mqtt.broker = mqttConfig["broker"].asString();
    }
    if (mqttConfig.isMember("client_id") && mqttConfig["client_id"].isString()) {
        config.mqtt.client_id = mqttConfig["client_id"].asString();
    }
    if (mqttConfig.isMember("username") && mqttConfig["username"].isString()) {
        config.mqtt.username = mqttConfig["username"].asString();
    }
    if (mqttConfig.isMember("password") && mqttConfig["password"].isString()) {
        config.mqtt.password = mqttConfig["password"].asString();
    }
    if (mqttConfig.isMember("keep_alive_interval") && mqttConfig["keep_alive_interval"].isInt()) {
        config.mqtt.keep_alive_interval = mqttConfig["keep_alive_interval"].asInt();
    }
    if (mqttConfig.isMember("timeout") && mqttConfig["timeout"].isInt()) {
        config.mqtt.timeout = mqttConfig["timeout"].asInt();
    }
    
    return true;
}

bool ConfigManager::parseUartConfig(const Json::Value& root) {
    if (!root.isMember("uart")) {
        return false;
    }
    
    Json::Value uartConfig = root["uart"];
    
    if (uartConfig.isMember("port") && uartConfig["port"].isString()) {
        config.uart.port = uartConfig["port"].asString();
    }
    if (uartConfig.isMember("baud_rate") && uartConfig["baud_rate"].isInt()) {
        config.uart.baud_rate = uartConfig["baud_rate"].asInt();
    }
    if (uartConfig.isMember("data_bits") && uartConfig["data_bits"].isInt()) {
        config.uart.data_bits = uartConfig["data_bits"].asInt();
    }
    if (uartConfig.isMember("stop_bits") && uartConfig["stop_bits"].isInt()) {
        config.uart.stop_bits = uartConfig["stop_bits"].asInt();
    }
    if (uartConfig.isMember("parity") && uartConfig["parity"].isString()) {
        config.uart.parity = uartConfig["parity"].asString();
    }
    
    return true;
}

bool ConfigManager::parseLoggerConfig(const Json::Value& root) {
    if (!root.isMember("logger")) {
        return false;
    }
    
    Json::Value loggerConfig = root["logger"];
    
    if (loggerConfig.isMember("level") && loggerConfig["level"].isString()) {
        config.logger.level = loggerConfig["level"].asString();
    }
    if (loggerConfig.isMember("console_output") && loggerConfig["console_output"].isBool()) {
        config.logger.console_output = loggerConfig["console_output"].asBool();
    }
    if (loggerConfig.isMember("file_output") && loggerConfig["file_output"].isBool()) {
        config.logger.file_output = loggerConfig["file_output"].asBool();
    }
    if (loggerConfig.isMember("log_path") && loggerConfig["log_path"].isString()) {
        config.logger.log_path = loggerConfig["log_path"].asString();
    }
    if (loggerConfig.isMember("log_name") && loggerConfig["log_name"].isString()) {
        config.logger.log_name = loggerConfig["log_name"].asString();
    }
    if (loggerConfig.isMember("max_file_size_mb") && loggerConfig["max_file_size_mb"].isInt()) {
        config.logger.max_file_size_mb = loggerConfig["max_file_size_mb"].asInt();
    }
    if (loggerConfig.isMember("max_files") && loggerConfig["max_files"].isInt()) {
        config.logger.max_files = loggerConfig["max_files"].asInt();
    }
    
    return true;
}
