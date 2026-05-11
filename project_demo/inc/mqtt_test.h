#ifndef MQTT_TEST_H
#define MQTT_TEST_H

#include <atomic>
#include "mqtt_client.h"
#include "config_manager.h"

extern std::atomic<bool> g_testComplete;
extern std::atomic<int> g_receivedCount;
extern std::atomic<int> g_publishedCount;
extern std::atomic<bool> g_verboseOutput;

void runConnectionTest(MqttClient& mqtt);
void runSubscribeTest(MqttClient& mqtt);
void runPublishTest(MqttClient& mqtt);
void runPerformanceTest(MqttClient& mqtt);
void runStressTest(MqttClient& mqtt);
void runOfflineQueueTest(MqttClient& mqtt);
void runReconnectTest(MqttClient& mqtt);
void setupMqttCallbacks(MqttClient& mqtt);
int runMqttTests(int argc, char* argv[]);

#endif
