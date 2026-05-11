#include "uart_manager.h"
#include <unistd.h>
#include <poll.h>
#include <algorithm>
#include <cstring>

UartManager::UartManager() : next_id_(1), polling_(false) {}

UartManager::~UartManager() {
    stopPolling();
    removeAll();
}

UartManager::UartId UartManager::addUart(const std::string& port, const Uart::Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::unique_ptr<Uart> uart(new Uart());
    if (!uart->open(config)) {
        return -1;
    }
    
    UartId id = next_id_++;
    UartEntry entry;
    entry.uart = std::move(uart);
    entry.port = port;
    uarts_[id] = std::move(entry);
    uart_ports_[id] = port;
    
    return id;
}

bool UartManager::removeUart(UartId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = uarts_.find(id);
    if (it == uarts_.end()) {
        return false;
    }
    
    it->second.uart->close();
    uarts_.erase(it);
    uart_ports_.erase(id);
    
    return true;
}

void UartManager::removeAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : uarts_) {
        pair.second.uart->close();
    }
    uarts_.clear();
    uart_ports_.clear();
}

Uart* UartManager::getUart(UartId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = uarts_.find(id);
    if (it == uarts_.end()) {
        return nullptr;
    }
    
    return it->second.uart.get();
}

bool UartManager::startPolling(int interval_ms) {
    if (polling_) {
        return false;
    }
    
    polling_ = true;
    polling_thread_ = std::thread(&UartManager::pollingThread, this, interval_ms);
    
    return true;
}

void UartManager::stopPolling() {
    if (!polling_) {
        return;
    }
    
    polling_ = false;
    
    if (polling_thread_.joinable()) {
        polling_thread_.join();
    }
}

void UartManager::pollingThread(int interval_ms) {
    while (polling_) {
        pollUarts();
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
    }
}

int UartManager::pollUarts() {
    std::vector<struct pollfd> pollfds;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& pair : uarts_) {
            int fd = pair.second.uart->getFd();
            if (fd >= 0) {
                struct pollfd pfd;
                pfd.fd = fd;
                pfd.events = POLLIN;
                pfd.revents = 0;
                pollfds.push_back(pfd);
            }
        }
    }
    
    if (pollfds.empty()) {
        return 0;
    }
    
    int ret = poll(pollfds.data(), pollfds.size(), 0);
    
    if (ret <= 0) {
        return ret;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t idx = 0;
    for (auto& pair : uarts_) {
        int fd = pair.second.uart->getFd();
        if (fd >= 0 && idx < pollfds.size()) {
            if (pollfds[idx].revents & POLLIN) {
                uint8_t buffer[4096];
                ssize_t len = ::read(fd, buffer, sizeof(buffer));
                
                if (len > 0 && data_callback_) {
                    data_callback_(pair.first, buffer, static_cast<size_t>(len));
                } else if (len < 0 && error_callback_) {
                    error_callback_(pair.first, strerror(errno));
                } else if (len == 0 && error_callback_) {
                    error_callback_(pair.first, "Device disconnected");
                }
            }
            idx++;
        }
    }
    
    return ret;
}

void UartManager::setDataCallback(DataCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    data_callback_ = callback;
}

void UartManager::setErrorCallback(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    error_callback_ = callback;
}

size_t UartManager::readUart(UartId id, uint8_t* buffer, size_t max_length) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = uarts_.find(id);
    if (it == uarts_.end()) {
        return 0;
    }
    
    return it->second.uart->read(buffer, max_length, 0);
}

int UartManager::writeUart(UartId id, const uint8_t* data, size_t length) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = uarts_.find(id);
    if (it == uarts_.end()) {
        return -1;
    }
    
    return it->second.uart->write(data, length);
}

std::map<UartManager::UartId, std::string> UartManager::getUartList() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return uart_ports_;
}
