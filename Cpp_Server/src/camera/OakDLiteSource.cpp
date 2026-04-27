#include <string>

#include "camera/OakDLiteSource.hpp"
#include "logging/logger.hpp"

OakDLiteSource::OakDLiteSource(const std::string& id)
    : id_(id) {

        logger_ = Logger::createLogger(id_);
        logger_->info("{} initialized", id_);

    }

void OakDLiteSource::start() {

    // initialize Pipeline
    logger_->info("Starting {}", id_);

    try {
        pipeline_ = dai::Pipeline();
    } catch (std::exception& e){
        logger_->error("Error occured while initializing {} pipeline: {}", id_, e.what());
    }

    // create camera and xout node
    auto cam = pipeline_.create<dai::node::ColorCamera>();
    auto xout = pipeline_.create<dai::node::XLinkOut>();

    // set stream name here, this needs to be unique, consider
    // switching to `id_` when expanding from POC
    xout->setStreamName("video");
    cam->setFps(10);
    cam->setIspScale(1, 3); // SCALE DOWN IMAGE
    cam->video.link(xout->input);

    device_ = std::make_unique<dai::Device>(pipeline_);
    frameQueue_ = device_->getOutputQueue("video", 1, false);
}

std::shared_ptr<RawFrame> OakDLiteSource::getFrame() {

    // Return type: std::shared_ptr<dai::ImgFrame>
    // we copy the shared_ptr address into tempFrame
    // that frameQueue returns and it is released by it
    // === Note: possibly add error handling if !tempFrame ===
    auto tempFrame = frameQueue_->get<dai::ImgFrame>();

    // initialize shared ptr frame of type RawFrame (custom struct)
    // view RawFrame.hpp for more information.
    auto frame = std::make_shared<RawFrame>();

    frame->data = tempFrame->getData();
    frame->timestamp = tempFrame->getTimestamp();
    frame->height = tempFrame->getHeight();
    frame->width = tempFrame->getWidth();
    frame->format = PixelFormat::NV12;

    // move the pointer from tempFrame to storage
    // without copying (incrementing references)
    // since we have no need for tempFrame anymore
    frame->storage = std::move(tempFrame);

    return frame;

}

void OakDLiteSource::stop() {
    frameQueue_ = nullptr;
    device_ = nullptr;
    logger_ = nullptr;
}