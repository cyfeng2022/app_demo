#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <string>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <vector>
#include <queue>
#include <thread>
#include <condition_variable>
#include <chrono>

#include <mqtt/async_client.h>
#include "logger.h"

class MqttClientCallback;

class MqttClient {
public:
    using ConnectionCallback = std::function<void(bool connected, const std::string& reason)>;
    using MessageCallback = std::function<void(const std::string& topic, const std::string& payload)>;
    using DeliveryCallback = std::function<void(bool delivered)>;

    enum class ConnectionState {
        Disconnected,
        Connecting,
        Connected,
        Disconnecting
    };

    struct Options {
        std::string server_uri;
        std::string client_id;
        int keep_alive_interval;
        int connection_timeout;
        int reconnect_delay_ms;
        int max_reconnect_delay_ms;
        int max_inflight_messages;
        bool clean_session;
        bool auto_reconnect;

        Options();
    };

    explicit MqttClient(const Options& options);
    ~MqttClient();

    MqttClient(const MqttClient&) = delete;
    MqttClient& operator=(const MqttClient&) = delete;

    void setConnectionCallback(ConnectionCallback cb);
    void setMessageCallback(MessageCallback cb);
    void setDeliveryCallback(DeliveryCallback cb);

    bool connect();
    void disconnect();
    
    bool isConnected() const;
    ConnectionState getState() const;

    bool publish(const std::string& topic, const std::string& payload, int qos = 0, bool retained = false);
    bool publish(const std::string& topic, const std::string& payload, DeliveryCallback cb, int qos = 0, bool retained = false);
    
    bool subscribe(const std::string& topic, int qos = 0);
    bool unsubscribe(const std::string& topic);

    void stop();

private:
    friend class MqttClientCallback;

    void onConnectionLost(const std::string& cause);
    void onConnected(const std::string& cause);
    void onMessageArrived(mqtt::const_message_ptr msg);
    void onDeliveryComplete(mqtt::delivery_token_ptr token);

    void doReconnect();
    void reconnectLoop();
    void messageQueueLoop();

    Options options_;
    std::unique_ptr<mqtt::async_client> client_;
    std::unique_ptr<MqttClientCallback> callback_;
    
    mqtt::connect_options connOptions_;
    
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    DeliveryCallback deliveryCallback_;
    
    std::atomic<ConnectionState> state_;
    std::atomic<bool> running_;
    std::atomic<bool> reconnecting_;
    
    std::thread reconnectThread_;
    std::thread messageQueueThread_;
    
    std::mutex stateMutex_;
    std::mutex messageQueueMutex_;
    std::condition_variable messageQueueCv_;
    
    struct PendingMessage {
        std::string topic;
        std::string payload;
        int qos;
        bool retained;
        DeliveryCallback callback;
    };
    std::queue<PendingMessage> messageQueue_;
    
    int reconnectAttempts_;
};

inline MqttClient::Options::Options()
    : server_uri("tcp://localhost:1883")
    , client_id("mqtt_client")
    , keep_alive_interval(60)
    , connection_timeout(30)
    , reconnect_delay_ms(1000)
    , max_reconnect_delay_ms(60000)
    , max_inflight_messages(100)
    , clean_session(true)
    , auto_reconnect(true)
{
}

#endif
