#include "encoder/Encoder.hpp"
#include "logging/logger.hpp"

#include <sstream>



Encoder::Encoder(const int width, const int height, const int fps) 
        : width_(width), height_(height), fps_(fps) {
    init();
    initFrame();

    logger_ = Logger::createLogger("encoder");
    logger_->info("Encoder Initialized");


}

bool Encoder::init() {

    const AVCodec* codec = avcodec_find_encoder_by_name("libx264");
    if (!codec) {
        logger_->error("Unable to retrieve codec type");
        return false;
    }

    // consider passing nullptr to intialize empty
    // codec context, context is then verified on `avcodec_open2`
    ctx_ = avcodec_alloc_context3(codec);

    //Allocate the packet that's used by encoder to push compressed data
    pkt_ = av_packet_alloc();

    if (!ctx_) {
        logger_->error("Codec context allocation failed");
        return false;
    }

    //Set encoder public options
    ctx_->profile = FF_PROFILE_H264_CONSTRAINED_BASELINE;
    ctx_->level = 31;
    ctx_->width = width_;
    ctx_->height = height_;
    ctx_->framerate = {fps_, 1};
    ctx_->time_base = {1, fps_};

    //Setting pix_fmt should be dynamic in future
    ctx_->pix_fmt = AV_PIX_FMT_NV12;
    ctx_->bit_rate = 2000000;
    ctx_->rc_max_rate = 2200000;
    ctx_->rc_buffer_size = 4000000; 
    
    ctx_->gop_size = fps_;
    ctx_->max_b_frames = 0;
    ctx_->flags &= ~AV_CODEC_FLAG_GLOBAL_HEADER;

    // Set encoder private options
    av_opt_set(ctx_->priv_data, "preset", "ultrafast", 0);
    av_opt_set(ctx_->priv_data, "tune", "zerolatency", 0);
    av_opt_set(ctx_->priv_data, "profile", "baseline", 0);
    av_opt_set(ctx_->priv_data, "x264-params", "force-cfr=1:repeat-headers=1", 0);

    // Initialize & Start the encoder by passing the:
    // - AVCodecContext [actual encoder]
    // - Codec [type]
    // - Private Options [Currently nullptr as they were set above]
    if (avcodec_open2(ctx_, codec, nullptr) < 0){
        return false;
    }

    return true;
}

bool Encoder::initFrame() {

    // allocate ffmpeg frame structure and store it inside
    // frame private member
    frame_ = av_frame_alloc();

    if (!frame_) {
        logger_->error("Frame initializion failed");
        return false;
    }

    // set expected pixel format, height, and width
    frame_->format = AV_PIX_FMT_NV12;
    frame_->height = height_;
    frame_->width = width_;

    return true;
}

void Encoder::fillFrame(const RawFrame& raw) {

    // frame_.data[] is an array of pointers which points to the address
    // of the starting byte, i filled this according to NV12 structure
    frame_->data[0] = const_cast<uint8_t*>(raw.data.data());
    frame_->data[1] = const_cast<uint8_t*>(raw.data.data() + (width_ * height_));

    frame_->linesize[0] = width_;
    frame_->linesize[1] = width_;

    // use the actual raw frames time stamp
    //frame_->pts = raw.timestamp.time_since_epoch().count();
    static int64_t pts_counter = 0;
    frame_->pts = pts_counter++;

}

std::vector<uint8_t> Encoder::encodeFrame(const std::shared_ptr<RawFrame>& raw) {

    std::vector<uint8_t> compressedData;

    // fill the frame before sending to encoder
    // dereference raw before sending to fillFrame
    fillFrame(*raw);

    // send to encoder and check return
    int ret = avcodec_send_frame(ctx_, frame_);
    if (ret < 0) {
        return {};
    }

    // re-assign return to value of receiving packet from encoder
    ret = avcodec_receive_packet(ctx_, pkt_);

    // two return codes we are checking for EAGAIN (try again)
    // and ERROR_EOF (The encoder has been flushed/shut down)
    // need to add logic for  specific handling of these
    if (ret == AVERROR_EOF) {
        logger_->error("Encoder has already completed the current frame");
        return {};
    } if (ret == AVERROR(EAGAIN)) {
        logger_->error("Encoder requires more frames to output data, please try again");
        return {};
    } else if (ret < 0) {
        return {};
    }

    // copy the compressed data to a vector of uint8_t 
    compressedData.insert(compressedData.end(), pkt_->data, pkt_->data + pkt_->size);

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < 5; ++i) {
        ss << std::setw(2) << static_cast<int>(pkt_->data[i]) << " ";
    }

    logger_->info("FULL PACKET ({} bytes): {}", pkt_->size, ss.str());

    // unload the AV packet & AV frame
    av_packet_unref(pkt_);

    return compressedData;

}

Encoder::~Encoder() {
    // For each ffmpeg related variable pass a
    // reference so that the free methods can properly
    // deallocate them
    if (ctx_) avcodec_free_context(&ctx_);
    if (frame_) av_frame_free(&frame_);
    if (pkt_) av_packet_free(&pkt_);
}