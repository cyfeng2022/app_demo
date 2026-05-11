#ifndef UART_MANAGER_H
#define UART_MANAGER_H

#include "uart.h"
#include <map>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <thread>
#include <memory>

class UartManager {
public:
    using UartId = int;
    using DataCallback = std::function<void(UartId, const uint8_t*, size_t)>;
    using ErrorCallback = std::function<void(UartId, const std::string&)>;

    UartManager();
    ~UartManager();

    UartId addUart(const std::string& port, const Uart::Config& config);
    bool removeUart(UartId id);
    void removeAll();
    
    Uart* getUart(UartId id);
    
    bool startPolling(int interval_ms = 10);
    void stopPolling();
    bool isPolling() const { return polling_; }
    
    void setDataCallback(DataCallback callback);
    void setErrorCallback(ErrorCallback callback);
    
    size_t readUart(UartId id, uint8_t* buffer, size_t max_length);
    int writeUart(UartId id, const uint8_t* data, size_t length);
    
    std::map<UartId, std::string> getUartList() const;

private:
    void pollingThread(int interval_ms);
    int pollUarts();

    struct UartEntry {
        std::unique_ptr<Uart> uart;
        std::string port;
    };

    std::map<UartId, UartEntry> uarts_;
    std::map<UartId, std::string> uart_ports_;
    UartId next_id_;
    mutable std::mutex mutex_;
    
    bool polling_;
    std::thread polling_thread_;
    
    DataCallback data_callback_;
    ErrorCallback error_callback_;
};

#endif
