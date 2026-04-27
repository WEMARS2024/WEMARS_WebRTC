#pragma once
#include <functional>
#include <string>
#include <map>
#include <rtc/rtc.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

// similar to signalling server, this PeerSession
// will handle one peer connection
class PeerSession : public std::enable_shared_from_this<PeerSession> {
public:

    // ensures shared pointer is created
    static std::shared_ptr<PeerSession> init(const int cameraCount);

    // init callback type
    using messageSender = std::function<void(const std::string&)>;

    void peerSessionSetup();
    void setEmitHandler(messageSender);
    void initTracks();
    void initPC();
    void recieveAnswer(const nlohmann::json&);
    void sendCandidates();
    void recieveCandidates(const nlohmann::json&);
    std::shared_ptr<rtc::Track> getTrack(const std::string&);
    void startTrack();
    void stopTrack();
    void closePC();

    ~PeerSession();

private:

    // init pc & callbacks in private constructor
    PeerSession(const int cameraCount);

    std::shared_ptr<spdlog::logger> logger_;

    std::map<std::string, std::shared_ptr<rtc::Track>> trackMap_;
    std::shared_ptr<rtc::PeerConnection> pc_;
    messageSender sendMessage_;

    const int cameraCount_;

};