#pragma once

#include <memory>
#include <string>
#include <spdlog/spdlog.h>

#include "frame/RawFrame.hpp"

class CameraSource{
public:

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual std::shared_ptr<RawFrame> getFrame() = 0;

    virtual ~CameraSource() = default;

};
