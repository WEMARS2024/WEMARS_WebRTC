#pragma once

#include "camera/CameraSource.hpp"
#include "depthai/depthai.hpp"

class OakDLiteSource : public CameraSource {
public:
    OakDLiteSource(const std::string& id, const int);

    void start() override;
    void stop() override;
    std::shared_ptr<RawFrame> getFrame() override;
    //std::string id() const override;

private:
    std::string id_;
    int fps_;

    //Depth AI Related Variables
    dai::Pipeline pipeline_;
    std::unique_ptr<dai::Device> device_;
    std::shared_ptr<dai::DataOutputQueue> frameQueue_;

    std::shared_ptr<spdlog::logger> logger_;

};