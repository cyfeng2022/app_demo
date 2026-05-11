#include <string>
#include <vector>
#include <mutex>
#include <functional>

class Uart {
public:
    enum Parity {
        PARITY_NONE,
        PARITY_ODD,
        PARITY_EVEN
    };

    enum StopBits {
        STOPBITS_1,
        STOPBITS_1_5,
        STOPBITS_2
    };

    struct Config {
        std::string port = "/dev/ttyS0";
        int baud_rate = 115200;
        int data_bits = 8;
        Parity parity = PARITY_NONE;
        StopBits stop_bits = STOPBITS_1;
        bool flow_control = false;
        int read_buffer_size = 4096;
        int write_buffer_size = 4096;
    };

    Uart();
    ~Uart();

    bool open(const Config& config);
    void close();
    bool isOpen() const;

    int write(const uint8_t* data, size_t length);
    int write(const std::vector<uint8_t>& data);
    int write(const std::string& data);

    ssize_t read(uint8_t* buffer, size_t max_length, int timeout_ms = -1);
    ssize_t read(std::vector<uint8_t>& buffer, size_t max_length, int timeout_ms = -1);
    
    bool waitForData(int timeout_ms);
    
    void setDataCallback(std::function<void(const uint8_t*, size_t)> callback);
    
    int getFd() const { return fd_; }
    
    bool flushInput();
    bool flushOutput();
    bool flush();

private:
    bool configurePort(const Config& config);
    bool setBaudRate(int baud_rate);
    bool setDataBits(int data_bits);
    bool setParity(Parity parity);
    bool setStopBits(StopBits stop_bits);
    bool setFlowControl(bool enable);

    int fd_ = -1;
    Config config_;
    std::mutex mutex_;
    std::function<void(const uint8_t*, size_t)> data_callback_;
};
