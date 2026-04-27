cmake_minimum_required(VERSION 3.14)
project(WEMARS_WebRTC_Project)

# 1. libdatachannel
include(FetchContent)
FetchContent_Declare(
    libdatachannel
    GIT_REPOSITORY https://github.com/paullouisageneau/libdatachannel
    GIT_TAG v0.24.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(libdatachannel)

# 2. DepthAI
find_package(depthai CONFIG REQUIRED)

# 3. Find FFmpeg (avcodec and avutil)
find_package(PkgConfig REQUIRED)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVUTIL REQUIRED libavutil)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 4. Define Executable
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(WEMARS_WebRTC ${SOURCES})

# 5. Include Paths (Add FFmpeg include dirs here)
target_include_directories(WEMARS_WebRTC PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${AVCODEC_INCLUDE_DIRS}
    ${AVUTIL_INCLUDE_DIRS}
)

# 6. Linking
target_link_libraries(WEMARS_WebRTC PRIVATE 
    depthai::core 
    LibDataChannel::LibDataChannel
    ${AVCODEC_LIBRARIES} 
    ${AVUTIL_LIBRARIES}
)


# RUN CMAKE WITH:
# cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..