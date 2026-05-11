#include "mqtt_test.h"
#include <iostream>
#include <iomanip>

// 测试状态标志
std::atomic<bool> g_testComplete{false};
std::atomic<int> g_receivedCount{0};
std::atomic<int> g_publishedCount{0};
std::atomic<bool> g_verboseOutput{true};

void printStatus(const MqttClient& mqtt) {
    std::cout << "\n[Status] Connected: " << (mqtt.isConnected() ? "YES" : "NO") 
              << ", State: " << static_cast<int>(mqtt.getState()) << std::endl;
}

void runConnectionTest(MqttClient& mqtt) {
    std::cout << "\n=== [测试1] 连接测试 ===" << std::endl;
    
    std::cout << "正在连接到MQTT Broker..." << std::endl;
    bool success = mqtt.connect();
    
    if (success) {
        std::cout << "连接请求已发送" << std::endl;
        for (int i = 0; i < 5; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (mqtt.isConnected()) {
                std::cout << "✅ 连接成功！" << std::endl;
                break;
            }
            std::cout << "等待连接... " << i+1 << "s" << std::endl;
        }
    } else {
        std::cout << "❌ 连接失败！" << std::endl;
    }
    
    printStatus(mqtt);
}

void runSubscribeTest(MqttClient& mqtt) {
    std::cout << "\n=== [测试2] 订阅测试 ===" << std::endl;
    
    if (!mqtt.isConnected()) {
        std::cout << "跳过订阅测试 - 未连接" << std::endl;
        return;
    }
    
    std::vector<std::string> topics = {
        "test/topic1",
        "test/topic2",
        "test/qos0",
        "test/qos1",
        "test/qos2"
    };
    
    for (const auto& topic : topics) {
        int qos = (topic == "test/qos0") ? 0 : 
                  (topic == "test/qos2") ? 2 : 1;
        
        bool success = mqtt.subscribe(topic, qos);
        std::cout << (success ? "✅" : "❌") << " 订阅 [" << std::setw(12) << topic << "] QoS=" << qos << std::endl;
    }
}

void runPublishTest(MqttClient& mqtt) {
    std::cout << "\n=== [测试3] 发布测试 ===" << std::endl;
    
    if (!mqtt.isConnected()) {
        std::cout << "跳过发布测试 - 未连接" << std::endl;
        return;
    }
    
    struct TestMsg {
        std::string topic;
        std::string payload;
        int qos;
    };
    
    std::vector<TestMsg> messages = {
        {"test/topic1", "Hello from C++ MQTT Client!", 0},
        {"test/topic2", "QoS 1 message", 1},
        {"test/qos0", "QoS 0 - Fire and forget", 0},
        {"test/qos1", "QoS 1 - At least once", 1},
        {"test/qos2", "QoS 2 - Exactly once", 2},
    };
    
    for (const auto& msg : messages) {
        bool success = mqtt.publish(msg.topic, msg.payload, msg.qos);
        if (success) {
            g_publishedCount++;
            std::cout << "✅ 发布 [" << std::setw(12) << msg.topic << "] QoS=" << msg.qos << std::endl;
        } else {
            std::cout << "❌ 发布失败 [" << msg.topic << "]" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "\n等待消息接收..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "已发布: " << g_publishedCount.load() << " 条消息" << std::endl;
    std::cout << "已接收: " << g_receivedCount.load() << " 条消息" << std::endl;
}

void runPerformanceTest(MqttClient& mqtt) {
    std::cout << "\n=== [测试4] 性能测试 ===" << std::endl;
    
    if (!mqtt.isConnected()) {
        std::cout << "跳过敏能测试 - 未连接" << std::endl;
        return;
    }
    
    const int testCount = 100;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < testCount; ++i) {
        mqtt.publish("test/perf", "Perf test msg #" + std::to_string(i), 0);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "发送 " << testCount << " 条消息耗时: " << duration.count() << " ms" << std::endl;
    std::cout << "吞吐量: " << std::fixed << std::setprecision(2) 
              << (testCount * 1000.0 / duration.count()) << " msg/sec" << std::endl;
}

void runStressTest(MqttClient& mqtt) {
    std::cout << "\n=== [测试5] 收发压力测试 ===" << std::endl;
    
    if (!mqtt.isConnected()) {
        std::cout << "跳过压力测试 - 未连接" << std::endl;
        return;
    }

    // 保存原始输出设置，关闭详细消息输出
    bool originalVerbose = g_verboseOutput.load();
    g_verboseOutput.store(false);

    // 订阅测试主题（闭环测试）
    const std::string stressTopic = "test/stress";
    std::cout << "订阅压力测试主题: " << stressTopic << std::endl;
    mqtt.subscribe(stressTopic, 1);
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 测试配置
    const int testIterations = 5;
    const std::vector<int> messageCounts = {100, 500, 1000, 2000, 5000};
    const std::vector<int> qosLevels = {0, 1, 2};
    
    for (int qos : qosLevels) {
        std::cout << "\n--- QoS " << qos << " 压力测试 ---" << std::endl;
        
        for (int msgCount : messageCounts) {
            // 重置计数器
            g_receivedCount.store(0);
            g_publishedCount.store(0);
            
            auto startTime = std::chrono::high_resolution_clock::now();
            
            // 批量发布消息（带流控）
            for (int i = 0; i < msgCount; ++i) {
                std::string payload = "Stress msg " + std::to_string(i) + " of " + std::to_string(msgCount);
                mqtt.publish(stressTopic, payload, qos);
                // 每发送50条消息等待一下，避免缓冲区溢出
                if ((i + 1) % 50 == 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
            
            // 等待消息接收 (根据QoS级别调整等待时间)
            int waitMs = (qos == 2) ? msgCount * 5 : (qos == 1) ? msgCount * 3 : msgCount * 2;
            auto publishEndTime = std::chrono::high_resolution_clock::now();
            std::this_thread::sleep_for(std::chrono::milliseconds(waitMs));
            
            auto endTime = std::chrono::high_resolution_clock::now();
            
            // 计算统计数据
            auto publishDuration = std::chrono::duration_cast<std::chrono::milliseconds>(publishEndTime - startTime);
            auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            
            int published = msgCount;
            int received = g_receivedCount.load();
            int lost = published - received;
            double lossRate = (lost * 100.0) / published;
            double throughput = (published * 1000.0) / publishDuration.count();
            
            // 输出统计摘要（单独一行，清晰可见）
            std::cout << "\n[" << std::setw(4) << msgCount << "条] 已发布: " << published 
                      << " | 已接收: " << received 
                      << " | 丢失: " << lost 
                      << " (" << std::fixed << std::setprecision(2) << lossRate << "%)"
                      << " | 耗时: " << totalDuration.count() << "ms"
                      << " | 吞吐: " << std::fixed << std::setprecision(0) << throughput << " msg/s"
                      << std::endl;
        }
    }
    
    // 长时间持续测试
    std::cout << "\n--- 长时间持续测试 (30秒) ---" << std::endl;
    g_receivedCount.store(0);
    g_publishedCount.store(0);
    
    auto start = std::chrono::high_resolution_clock::now();
    int msgId = 0;
    
    for (int sec = 0; sec < 30; sec++) {
        for (int i = 0; i < 100; i++) {
            mqtt.publish(stressTopic, "Continuous msg " + std::to_string(msgId++), 0);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        if ((sec + 1) % 10 == 0) {
            std::cout << "第 " << sec + 1 << " 秒: 已发送 " << msgId << " 条消息" << std::endl;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    
    std::cout << "\n持续测试完成:" << std::endl;
    std::cout << "总消息数: " << msgId << std::endl;
    std::cout << "总耗时: " << duration.count() << " 秒" << std::endl;
    std::cout << "平均吞吐量: " << std::fixed << std::setprecision(2) 
              << (msgId * 1.0 / duration.count()) << " msg/sec" << std::endl;
    
    // 取消订阅
    mqtt.unsubscribe(stressTopic);
    
    // 恢复详细输出设置
    g_verboseOutput.store(originalVerbose);
}

void runOfflineQueueTest(MqttClient& mqtt) {
    std::cout << "\n=== [测试6] 离线队列测试 ===" << std::endl;
    
    bool wasConnected = mqtt.isConnected();
    
    if (wasConnected) {
        std::cout << "断开连接模拟离线状态..." << std::endl;
        mqtt.disconnect();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    std::cout << "离线状态下发布消息..." << std::endl;
    for (int i = 0; i < 5; ++i) {
        mqtt.publish("test/offline", "Offline msg #" + std::to_string(i), 1);
        std::cout << "📤 离线发布消息 #" << i+1 << std::endl;
    }
    
    std::cout << "\n重新连接..." << std::endl;
    mqtt.connect();
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "离线队列消息已发送" << std::endl;
}

void runReconnectTest(MqttClient& mqtt) {
    std::cout << "\n=== [测试7] 断线重连测试 ===" << std::endl;
    
    if (!mqtt.isConnected()) {
        std::cout << "跳过重连测试 - 未连接" << std::endl;
        return;
    }
    
    std::cout << "测试将在10秒后模拟断开... (可手动停止broker测试)" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    std::cout << "\n等待自动重连... (请重启MQTT broker)" << std::endl;
    for (int i = 0; i < 30; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (mqtt.isConnected()) {
            std::cout << "✅ 在第 " << i+1 << " 秒重新连接成功！" << std::endl;
            break;
        }
        std::cout << "⏳ 等待重连... " << i+1 << "s" << std::endl;
    }
    
    printStatus(mqtt);
}

void setupMqttCallbacks(MqttClient& mqtt) {
    mqtt.setConnectionCallback([](bool connected, const std::string& reason) {
        if (connected) {
            std::cout << "\n🔌 [回调] 连接成功! (" << reason << ")" << std::endl;
        } else {
            std::cout << "\n🔌 [回调] 连接断开! (" << reason << ")" << std::endl;
        }
    });

    mqtt.setMessageCallback([](const std::string& topic, const std::string& payload) {
        g_receivedCount++;
        if (g_verboseOutput.load()) {
            std::cout << "\n📥 [消息] Topic: " << std::setw(15) << topic 
                      << " | Payload: " << payload << std::endl;
        }
    });

    mqtt.setDeliveryCallback([](bool delivered) {
        std::cout << "📤 [投递] 消息投递: " << (delivered ? "成功" : "失败") << std::endl;
    });
}

int runMqttTests(int argc, char* argv[]) {
    std::cout << "============================================" << std::endl;
    std::cout << "          MQTT Client Test Suite" << std::endl;
    std::cout << "============================================" << std::endl;

    ConfigManager config;
    bool configLoaded = config.loadConfig("config.json");
    
    MqttClient::Options opts;
    
    if (configLoaded) {
        const MqttConfig& mqttConfig = config.getMqttConfig();
        opts.server_uri = "tcp://" + mqttConfig.broker;
        opts.client_id = mqttConfig.client_id.empty() ? "test_client" : mqttConfig.client_id;
        opts.keep_alive_interval = mqttConfig.keep_alive_interval;
        opts.connection_timeout = mqttConfig.timeout;
        std::cout << "📋 使用配置文件: " << opts.server_uri << std::endl;
    } else {
        opts.server_uri = "tcp://localhost:1883";
        opts.client_id = "test_client";
        std::cout << "📋 使用默认配置: " << opts.server_uri << std::endl;
    }
    
    opts.auto_reconnect = true;
    opts.reconnect_delay_ms = 1000;
    opts.max_reconnect_delay_ms = 30000;
    
    MqttClient mqtt(opts);
    setupMqttCallbacks(mqtt);

    bool runAll = true;
    int testNum = 0;
    
    if (argc > 1) {
        try {
            testNum = std::stoi(argv[1]);
            runAll = false;
        } catch (...) {
            std::cout << "参数错误，运行全部测试" << std::endl;
        }
    }

    if (runAll) {
        runConnectionTest(mqtt);
        runSubscribeTest(mqtt);
        runPublishTest(mqtt);
        runPerformanceTest(mqtt);
        // runStressTest(mqtt);  // 压力测试耗时较长，默认不运行
    } else {
        switch (testNum) {
            case 1: runConnectionTest(mqtt); break;
            case 2: runConnectionTest(mqtt); runSubscribeTest(mqtt); break;
            case 3: runConnectionTest(mqtt); runSubscribeTest(mqtt); runPublishTest(mqtt); break;
            case 4: runConnectionTest(mqtt); runPerformanceTest(mqtt); break;
            case 5: runConnectionTest(mqtt); runStressTest(mqtt); break;
            case 6: runConnectionTest(mqtt); runOfflineQueueTest(mqtt); break;
            case 7: runConnectionTest(mqtt); runReconnectTest(mqtt); break;
            default: std::cout << "未知测试编号: " << testNum << std::endl;
        }
    }

    std::cout << "\n=== 清理资源 ===" << std::endl;
    mqtt.disconnect();
    std::cout << "✅ 测试完成" << std::endl;
    
    return 0;
}
