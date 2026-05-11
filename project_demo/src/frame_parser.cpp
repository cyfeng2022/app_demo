#include "frame_parser.h"
#include <cstring>
#include <cstdint>

FrameParser::FrameParser(const Config& config) : config_(config), in_frame_(false) {}

FrameParser::FrameParser() : in_frame_(false) {}

void FrameParser::reset() {
    buffer_.clear();
    frames_.clear();
    in_frame_ = false;
}

void FrameParser::setFrameCallback(std::function<void(const std::vector<uint8_t>&)> callback) {
    frame_callback_ = callback;
}

const std::vector<std::vector<uint8_t>>& FrameParser::getFrames() const {
    return frames_;
}

size_t FrameParser::feed(const uint8_t* data, size_t length) {
    if (length == 0) return 0;
    
    buffer_.insert(buffer_.end(), data, data + length);
    
    switch (config_.type) {
        case FRAME_FIXED_LENGTH:
            processFixedLength();
            break;
        case FRAME_DELIMITED:
            processDelimited();
            break;
        case FRAME_LENGTH_PREFIXED:
            processLengthPrefixed();
            break;
        case FRAME_START_END_MARKER:
            processStartEndMarker();
            break;
    }
    
    return frames_.size();
}

void FrameParser::processFixedLength() {
    if (config_.fixed_length == 0) return;
    
    while (buffer_.size() >= config_.fixed_length) {
        std::vector<uint8_t> frame(buffer_.begin(), buffer_.begin() + config_.fixed_length);
        frames_.push_back(frame);
        
        if (frame_callback_) {
            frame_callback_(frame);
        }
        
        buffer_.erase(buffer_.begin(), buffer_.begin() + config_.fixed_length);
    }
}

void FrameParser::processDelimited() {
    size_t pos = 0;
    while (pos < buffer_.size()) {
        if (buffer_[pos] == config_.delimiter) {
            std::vector<uint8_t> frame(buffer_.begin(), buffer_.begin() + pos);
            frames_.push_back(frame);
            
            if (frame_callback_) {
                frame_callback_(frame);
            }
            
            buffer_.erase(buffer_.begin(), buffer_.begin() + pos + 1);
            pos = 0;
        } else {
            pos++;
        }
    }
}

void FrameParser::processLengthPrefixed() {
    size_t header_size = config_.length_offset + config_.length_size;
    
    while (buffer_.size() >= header_size) {
        size_t frame_length = readUint(buffer_.data() + config_.length_offset, 
                                       config_.length_size, 
                                       config_.length_big_endian);
        
        if (frame_length == 0 || frame_length > config_.max_frame_length) {
            buffer_.erase(buffer_.begin());
            continue;
        }
        
        size_t total_length = header_size + frame_length;
        
        if (buffer_.size() >= total_length) {
            std::vector<uint8_t> frame(buffer_.begin() + header_size, 
                                       buffer_.begin() + total_length);
            frames_.push_back(frame);
            
            if (frame_callback_) {
                frame_callback_(frame);
            }
            
            buffer_.erase(buffer_.begin(), buffer_.begin() + total_length);
        } else {
            break;
        }
    }
}

void FrameParser::processStartEndMarker() {
    size_t pos = 0;
    while (pos < buffer_.size()) {
        if (!in_frame_) {
            if (buffer_[pos] == config_.start_marker) {
                in_frame_ = true;
                buffer_.erase(buffer_.begin(), buffer_.begin() + pos + 1);
                pos = 0;
            } else {
                pos++;
            }
        } else {
            if (buffer_[pos] == config_.end_marker) {
                std::vector<uint8_t> frame(buffer_.begin(), buffer_.begin() + pos);
                frames_.push_back(frame);
                
                if (frame_callback_) {
                    frame_callback_(frame);
                }
                
                buffer_.erase(buffer_.begin(), buffer_.begin() + pos + 1);
                in_frame_ = false;
                pos = 0;
            } else {
                pos++;
            }
        }
    }
    
    if (buffer_.size() > config_.max_frame_length) {
        buffer_.clear();
        in_frame_ = false;
    }
}

size_t FrameParser::readUint(const uint8_t* data, size_t size, bool big_endian) {
    size_t result = 0;
    
    if (big_endian) {
        for (size_t i = 0; i < size; i++) {
            result = (result << 8) | data[i];
        }
    } else {
        for (size_t i = 0; i < size; i++) {
            result |= static_cast<size_t>(data[i]) << (8 * i);
        }
    }
    
    return result;
}
