#pragma once

#include <cstdint>
#include <span>
#include <chrono>
#include <memory>

#include "depthai/depthai.hpp"
#include "PixelFormat.hpp"

// RawFrame struct is a generalized frame structure to allow the encoder
// and camera classes to be decoupled.
struct RawFrame{
    std::span<uint8_t> data;
    std::chrono::time_point<std::chrono::steady_clock> timestamp;
    int height;
    int width;
    PixelFormat format;
    std::shared_ptr<dai::ImgFrame> storage;
};


// === std::span<uint8_t> data ===
// utilize span for efficient zero-copy view of frame
// only available in cpp20+, replacement for method of
// manually creating a pointer to first byte in uint8_t
// and creating a size variable.


// === std::shared_ptr<dai::ImgFrame> storage ===
// store ptr to frame to prevent span from dangling during RawFrame's lifetime


// all other attributes are simple to understand, view PixelFormat.hpp for 
// information on it.