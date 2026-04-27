#pragma once

#include <vector>
#include <memory>
#include <spdlog/spdlog.h>

#include "frame/RawFrame.hpp"

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/opt.h>
}

// Encoder uses libavcodec
class Encoder {
public:
    Encoder(const int width, const int height, const int fps);

    bool init();
    bool initFrame();
    void fillFrame(const RawFrame& raw);
    std::vector<uint8_t> encodeFrame(const std::shared_ptr<RawFrame>& raw);

    ~Encoder();

private:
    std::shared_ptr<spdlog::logger> logger_;

    const int width_;
    const int height_;
    const int fps_;

    AVCodecContext* ctx_ = nullptr;
    AVFrame* frame_ = nullptr;
    AVPacket* pkt_ = nullptr;
};

// === C management ===
// the libraries being used are written in C, meaning memory management
// manual, this means we must explicitly allocate and deallocate
// the memory properly, luckily there are API's provided by the libraries
// that you will see in the cpp file. Notice how they are


// === AVCodecContext ===
// this is the actual instance of the encoder. it holds the state, 
// the buffers, and the specific settings (width, height, bitrate) for our streams.
// it also has private settings that can be modified as well that are not as generic
// i.e, they are codec dependant.


// === AVFrame ===
// this is simply a frame with the format that is expected by the encoder
// we will have to transfer our data into this frame object and fill it's
// expected attributes and then pass it to the encoder


// === AVPacket ===
// nend to fill out
