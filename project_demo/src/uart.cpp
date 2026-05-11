#include "uart.h"
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <cstring>
#include <stdexcept>

Uart::Uart() : fd_(-1) {}

Uart::~Uart() {
    close();
}

bool Uart::open(const Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
    
    fd_ = ::open(config.port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) {
        return false;
    }
    
    config_ = config;
    
    if (!configurePort(config)) {
        ::close(fd_);
        fd_ = -1;
        return false;
    }
    
    return true;
}

void Uart::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

bool Uart::isOpen() const {
    return fd_ >= 0;
}

bool Uart::configurePort(const Config& config) {
    struct termios termios;
    
    if (tcgetattr(fd_, &termios) != 0) {
        return false;
    }
    
    cfmakeraw(&termios);
    
    if (!setBaudRate(config.baud_rate)) return false;
    if (!setDataBits(config.data_bits)) return false;
    if (!setParity(config.parity)) return false;
    if (!setStopBits(config.stop_bits)) return false;
    if (!setFlowControl(config.flow_control)) return false;
    
    termios.c_cc[VTIME] = 0;
    termios.c_cc[VMIN] = 0;
    
    if (tcsetattr(fd_, TCSANOW, &termios) != 0) {
        return false;
    }
    
    return true;
}

bool Uart::setBaudRate(int baud_rate) {
    struct termios termios;
    if (tcgetattr(fd_, &termios) != 0) {
        return false;
    }
    
    speed_t speed;
    switch (baud_rate) {
        case 300: speed = B300; break;
        case 1200: speed = B1200; break;
        case 2400: speed = B2400; break;
        case 4800: speed = B4800; break;
        case 9600: speed = B9600; break;
        case 19200: speed = B19200; break;
        case 38400: speed = B38400; break;
        case 57600: speed = B57600; break;
        case 115200: speed = B115200; break;
        case 230400: speed = B230400; break;
        case 460800: speed = B460800; break;
        case 500000: speed = B500000; break;
        case 921600: speed = B921600; break;
        case 1000000: speed = B1000000; break;
        default: return false;
    }
    
    if (cfsetispeed(&termios, speed) != 0 || cfsetospeed(&termios, speed) != 0) {
        return false;
    }
    
    return tcsetattr(fd_, TCSANOW, &termios) == 0;
}

bool Uart::setDataBits(int data_bits) {
    struct termios termios;
    if (tcgetattr(fd_, &termios) != 0) {
        return false;
    }
    
    termios.c_cflag &= ~CSIZE;
    switch (data_bits) {
        case 5: termios.c_cflag |= CS5; break;
        case 6: termios.c_cflag |= CS6; break;
        case 7: termios.c_cflag |= CS7; break;
        case 8: termios.c_cflag |= CS8; break;
        default: return false;
    }
    
    return tcsetattr(fd_, TCSANOW, &termios) == 0;
}

bool Uart::setParity(Parity parity) {
    struct termios termios;
    if (tcgetattr(fd_, &termios) != 0) {
        return false;
    }
    
    switch (parity) {
        case PARITY_NONE:
            termios.c_cflag &= ~(PARENB | PARODD);
            break;
        case PARITY_ODD:
            termios.c_cflag |= PARENB | PARODD;
            break;
        case PARITY_EVEN:
            termios.c_cflag |= PARENB;
            termios.c_cflag &= ~PARODD;
            break;
        default:
            return false;
    }
    
    return tcsetattr(fd_, TCSANOW, &termios) == 0;
}

bool Uart::setStopBits(StopBits stop_bits) {
    struct termios termios;
    if (tcgetattr(fd_, &termios) != 0) {
        return false;
    }
    
    switch (stop_bits) {
        case STOPBITS_1:
            termios.c_cflag &= ~CSTOPB;
            break;
        case STOPBITS_2:
            termios.c_cflag |= CSTOPB;
            break;
        default:
            return false;
    }
    
    return tcsetattr(fd_, TCSANOW, &termios) == 0;
}

bool Uart::setFlowControl(bool enable) {
    struct termios termios;
    if (tcgetattr(fd_, &termios) != 0) {
        return false;
    }
    
    if (enable) {
        termios.c_cflag |= CRTSCTS;
    } else {
        termios.c_cflag &= ~CRTSCTS;
    }
    
    return tcsetattr(fd_, TCSANOW, &termios) == 0;
}

int Uart::write(const uint8_t* data, size_t length) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (fd_ < 0) return -1;
    
    ssize_t written = 0;
    while (written < static_cast<ssize_t>(length)) {
        ssize_t ret = ::write(fd_, data + written, length - written);
        if (ret < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(1000);
                continue;
            }
            return -1;
        }
        written += ret;
    }
    
    return static_cast<int>(written);
}

int Uart::write(const std::vector<uint8_t>& data) {
    return write(data.data(), data.size());
}

int Uart::write(const std::string& data) {
    return write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
}

bool Uart::waitForData(int timeout_ms) {
    if (fd_ < 0) return false;
    
    struct pollfd pfd;
    pfd.fd = fd_;
    pfd.events = POLLIN;
    pfd.revents = 0;
    
    int ret = poll(&pfd, 1, timeout_ms);
    
    return ret > 0 && (pfd.revents & POLLIN);
}

ssize_t Uart::read(uint8_t* buffer, size_t max_length, int timeout_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (fd_ < 0) return -1;
    
    if (!waitForData(timeout_ms)) {
        return 0;
    }
    
    return ::read(fd_, buffer, max_length);
}

ssize_t Uart::read(std::vector<uint8_t>& buffer, size_t max_length, int timeout_ms) {
    buffer.resize(max_length);
    ssize_t ret = read(buffer.data(), max_length, timeout_ms);
    if (ret > 0) {
        buffer.resize(static_cast<size_t>(ret));
    }
    return ret;
}

void Uart::setDataCallback(std::function<void(const uint8_t*, size_t)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    data_callback_ = callback;
}

bool Uart::flushInput() {
    if (fd_ < 0) return false;
    return tcflush(fd_, TCIFLUSH) == 0;
}

bool Uart::flushOutput() {
    if (fd_ < 0) return false;
    return tcflush(fd_, TCOFLUSH) == 0;
}

bool Uart::flush() {
    if (fd_ < 0) return false;
    return tcflush(fd_, TCIOFLUSH) == 0;
}
