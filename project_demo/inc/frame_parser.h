#include <vector>
#include <functional>
#include <cstdint>
#include <cstddef>

class FrameParser {
public:
    enum FrameType {
        FRAME_FIXED_LENGTH,
        FRAME_DELIMITED,
        FRAME_LENGTH_PREFIXED,
        FRAME_START_END_MARKER
    };

    struct Config {
        FrameType type;
        size_t fixed_length;
        uint8_t delimiter;
        size_t max_frame_length;
        uint8_t start_marker;
        uint8_t end_marker;
        size_t length_offset;
        size_t length_size;
        bool length_big_endian;
        
        Config() 
            : type(FRAME_FIXED_LENGTH),
              fixed_length(0),
              delimiter('\n'),
              max_frame_length(4096),
              start_marker(0x7E),
              end_marker(0x7E),
              length_offset(0),
              length_size(2),
              length_big_endian(true) {}
    };

    FrameParser(const Config& config);
    FrameParser();
    
    void reset();
    
    size_t feed(const uint8_t* data, size_t length);
    
    void setFrameCallback(std::function<void(const std::vector<uint8_t>&)> callback);
    
    const std::vector<std::vector<uint8_t>>& getFrames() const;
    
    size_t getPendingBytes() const { return buffer_.size(); }

private:
    void processFixedLength();
    void processDelimited();
    void processLengthPrefixed();
    void processStartEndMarker();
    
    size_t readUint(const uint8_t* data, size_t size, bool big_endian);

    Config config_;
    std::vector<uint8_t> buffer_;
    std::vector<std::vector<uint8_t>> frames_;
    std::function<void(const std::vector<uint8_t>&)> frame_callback_;
    bool in_frame_;
};
