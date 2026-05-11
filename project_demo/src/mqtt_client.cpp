#include "mqtt_client.h"
#include <iostream>

class MqttClientCallback : public mqtt::callback {
public:
    MqttClientCallback(MqttClient* client) : client_(client) {}

    void connection_lost(const std::string& cause) override {
        if (client_) {
            client_->onConnectionLost(cause);
        }
    }

    void connected(const std::string& cause) override {
        if (client_) {
            client_->onConnected(cause);
        }
    }

    void message_arrived(mqtt::const_message_ptr msg) override {
        if (client_ && msg) {
            client_->onMessageArrived(msg);
        }
    }

    void delivery_complete(mqtt::delivery_token_ptr token) override {
        if (client_) {
            client_->onDeliveryComplete(token);
        }
    }

private:
    MqttClient* client_;
};

MqttClient::MqttClient(const Options& options)
    : options_(options)
    , state_(ConnectionState::Disconnected)
    , running_(false)
    , reconnecting_(false)
    , reconnectAttempts_(0)
{
    client_ = std::unique_ptr<mqtt::async_client>(
        new mqtt::async_client(
            options_.server_uri, 
            options_.client_id,
            static_cast<int>(options_.max_inflight_messages)
        )
    );

    callback_ = std::unique_ptr<MqttClientCallback>(new MqttClientCallback(this));

    mqtt::connect_options_builder builder;
    builder.keep_alive_interval(std::chrono::seconds(options_.keep_alive_interval))
           .connect_timeout(std::chrono::seconds(options_.connection_timeout))
           .clean_session(options_.clean_session)
           .automatic_reconnect(options_.auto_reconnect);

    connOptions_ = builder.finalize();
}

MqttClient::~MqttClient() {
    stop();
}

void MqttClient::setConnectionCallback(ConnectionCallback cb) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    connectionCallback_ = std::move(cb);
}

void MqttClient::setMessageCallback(MessageCallback cb) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    messageCallback_ = std::move(cb);
}

void MqttClient::setDeliveryCallback(DeliveryCallback cb) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    deliveryCallback_ = std::move(cb);
}

bool MqttClient::connect() {
    std::lock_guard<std::mutex> lock(stateMutex_);
    
    if (state_.load() != ConnectionState::Disconnected) {
        LOG_WARN("[MQTT] Connect called but state is not Disconnected");
        return false;
    }

    state_ = ConnectionState::Connecting;
    running_ = true;

    LOG_INFO("[MQTT] Connecting to {} with client_id: {}", options_.server_uri, options_.client_id);

    try {
        client_->set_callback(*callback_);
        
        auto token = client_->connect(connOptions_);
        token->wait_for(std::chrono::seconds(options_.connection_timeout));

        if (token->is_complete()) {
            state_ = ConnectionState::Connected;
            reconnectAttempts_ = 0;
            
            messageQueueThread_ = std::thread(&MqttClient::messageQueueLoop, this);
            
            LOG_INFO("[MQTT] Connected successfully");
            
            if (connectionCallback_) {
                connectionCallback_(true, "");
            }
            return true;
        }
    } catch (const mqtt::exception& exc) {
        state_ = ConnectionState::Disconnected;
        LOG_ERROR("[MQTT] Connection failed: mqtt exception: {}", exc.what());
        if (connectionCallback_) {
            connectionCallback_(false, exc.what());
        }
    } catch (const std::exception& exc) {
        state_ = ConnectionState::Disconnected;
        LOG_ERROR("[MQTT] Connection failed: std exception: {}", exc.what());
        if (connectionCallback_) {
            connectionCallback_(false, exc.what());
        }
    }

    return false;
}

void MqttClient::disconnect() {
    std::unique_lock<std::mutex> lock(stateMutex_);
    
    if (state_.load() == ConnectionState::Disconnected) {
        return;
    }

    state_ = ConnectionState::Disconnecting;
    running_ = false;
    reconnecting_ = false;
    
    lock.unlock();

    try {
        if (messageQueueThread_.joinable()) {
            messageQueueCv_.notify_all();
            messageQueueThread_.join();
        }

        if (reconnectThread_.joinable()) {
            reconnectThread_.join();
        }

        if (client_ && client_->is_connected()) {
            auto token = client_->disconnect();
            token->wait_for(std::chrono::seconds(10));
        }
    } catch (const std::exception& exc) {
        std::cerr << "MQTT disconnect error: " << exc.what() << std::endl;
    }

    state_ = ConnectionState::Disconnected;
    
    if (connectionCallback_) {
        connectionCallback_(false, "disconnected");
    }
}

bool MqttClient::isConnected() const {
    return state_.load() == ConnectionState::Connected && 
           client_ && client_->is_connected();
}

MqttClient::ConnectionState MqttClient::getState() const {
    return state_.load();
}

bool MqttClient::publish(const std::string& topic, const std::string& payload, 
                         int qos, bool retained) {
    return publish(topic, payload, nullptr, qos, retained);
}

bool MqttClient::publish(const std::string& topic, const std::string& payload,
                         DeliveryCallback cb, int qos, bool retained) {
    if (!isConnected()) {
        std::lock_guard<std::mutex> lock(messageQueueMutex_);
        PendingMessage msg;
        msg.topic = topic;
        msg.payload = payload;
        msg.qos = qos;
        msg.retained = retained;
        msg.callback = std::move(cb);
        messageQueue_.push(std::move(msg));
        messageQueueCv_.notify_one();
        return true;
    }

    try {
        auto msg = mqtt::make_message(topic, payload);
        msg->set_qos(qos);
        msg->set_retained(retained);
        
        client_->publish(msg);
        
        if (cb) {
            cb(true);
        }
        
        return true;
    } catch (const std::exception& exc) {
        std::cerr << "MQTT publish error: " << exc.what() << std::endl;
        if (cb) {
            cb(false);
        }
        return false;
    }
}

bool MqttClient::subscribe(const std::string& topic, int qos) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    
    if (state_.load() != ConnectionState::Connected) {
        return false;
    }

    try {
        auto token = client_->subscribe(topic, qos);
        token->wait_for(std::chrono::seconds(5));
        return token->get_message_id() != 0;
    } catch (const std::exception& exc) {
        std::cerr << "MQTT subscribe error: " << exc.what() << std::endl;
        return false;
    }
}

bool MqttClient::unsubscribe(const std::string& topic) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    
    if (state_.load() != ConnectionState::Connected) {
        return false;
    }

    try {
        auto token = client_->unsubscribe(topic);
        token->wait_for(std::chrono::seconds(5));
        return true;
    } catch (const std::exception& exc) {
        std::cerr << "MQTT unsubscribe error: " << exc.what() << std::endl;
        return false;
    }
}

void MqttClient::stop() {
    disconnect();
}

void MqttClient::onConnectionLost(const std::string& cause) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    
    if (state_.load() == ConnectionState::Disconnecting) {
        return;
    }

    state_ = ConnectionState::Disconnected;
    
    if (connectionCallback_) {
        connectionCallback_(false, cause);
    }

    if (running_ && options_.auto_reconnect) {
        reconnecting_ = true;
        if (!reconnectThread_.joinable()) {
            reconnectThread_ = std::thread(&MqttClient::reconnectLoop, this);
        }
    }
}

void MqttClient::onConnected(const std::string& cause) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    
    state_ = ConnectionState::Connected;
    reconnectAttempts_ = 0;
    
    if (connectionCallback_) {
        connectionCallback_(true, cause);
    }
}

void MqttClient::onMessageArrived(mqtt::const_message_ptr msg) {
    if (!msg) return;

    std::string topic = msg->get_topic();
    std::string payload = msg->to_string();

    MessageCallback localCb;
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        localCb = messageCallback_;
    }

    if (localCb) {
        localCb(topic, payload);
    }
}

void MqttClient::onDeliveryComplete(mqtt::delivery_token_ptr token) {
    (void)token;
}

void MqttClient::doReconnect() {
    std::lock_guard<std::mutex> lock(stateMutex_);
    
    if (state_.load() != ConnectionState::Disconnected) {
        return;
    }

    state_ = ConnectionState::Connecting;

    try {
        client_->set_callback(*callback_);
        auto token = client_->connect(connOptions_);
        token->wait_for(std::chrono::seconds(options_.connection_timeout));

        if (token->is_complete()) {
            state_ = ConnectionState::Connected;
            reconnectAttempts_ = 0;
            reconnecting_ = false;
            
            if (connectionCallback_) {
                connectionCallback_(true, "reconnected");
            }
            return;
        }
    } catch (const std::exception& exc) {
        std::cerr << "MQTT reconnect error: " << exc.what() << std::endl;
    }

    state_ = ConnectionState::Disconnected;
}

void MqttClient::reconnectLoop() {
    while (reconnecting_ && running_) {
        int delayMs = std::min(
            options_.reconnect_delay_ms * (1 << std::min(reconnectAttempts_, 6)),
            options_.max_reconnect_delay_ms
        );
        
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        
        if (!running_ || !reconnecting_) {
            break;
        }

        doReconnect();
        
        if (state_.load() == ConnectionState::Connected) {
            break;
        }
        
        ++reconnectAttempts_;
    }
}

void MqttClient::messageQueueLoop() {
    while (running_) {
        std::unique_lock<std::mutex> lock(messageQueueMutex_);
        messageQueueCv_.wait_for(lock, std::chrono::milliseconds(100), [this] {
            return !running_ || !messageQueue_.empty();
        });

        while (!messageQueue_.empty()) {
            PendingMessage msg = std::move(messageQueue_.front());
            messageQueue_.pop();
            lock.unlock();

            if (isConnected()) {
                try {
                    auto mqttMsg = mqtt::make_message(msg.topic, msg.payload);
                    mqttMsg->set_qos(msg.qos);
                    mqttMsg->set_retained(msg.retained);
                    client_->publish(mqttMsg);
                    
                    if (msg.callback) {
                        msg.callback(true);
                    }
                } catch (const std::exception& exc) {
                    std::cerr << "MQTT queued publish error: " << exc.what() << std::endl;
                    if (msg.callback) {
                        msg.callback(false);
                    }
                }
            }

            lock.lock();
        }
    }
}
