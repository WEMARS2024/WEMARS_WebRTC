
#include <nlohmann/json.hpp>

#include "signalling/PeerSession.hpp"
#include "logging/logger.hpp"

std::shared_ptr<PeerSession> PeerSession::init(const int cameraCount) {

    auto peerSession = std::shared_ptr<PeerSession>(new PeerSession(cameraCount));

    return peerSession;

}

PeerSession::PeerSession(const int cameraCount): cameraCount_(cameraCount) {

    // initialize logger
    logger_ = Logger::createLogger("PeerSession");
    logger_->info("PeerSession Initialized");

}

void PeerSession::peerSessionSetup() {

    // set peer connection configurations
    // as per https://libdatachannel.org/pages/reference.html
    rtc::Configuration config;
    //config.iceServers.emplace_back("stun:://google.com");

    pc_ = std::make_shared<rtc::PeerConnection>(config);
    auto weakSelf = weak_from_this();

    // set the peer connections callback for when it set's it's local description
    pc_->onLocalDescription([weakSelf](rtc::Description description){
        if (auto self = weakSelf.lock()){
            // we need to convert to JSON for formatting and 
            // then dump the msg as a string for peer to handle
            self->logger_->info("Sending Local Description to Peer");
            std::string sdpString = std::string(description);
            std::string sdpType = description.typeString();
            
            nlohmann::json sdpMessage;

            sdpMessage["type"] = sdpType;
            sdpMessage["sdp"] = sdpString;

            if (self->sendMessage_) {
                self->sendMessage_(sdpMessage.dump());
            }
            else {
                self->logger_->error("SDP Offer failed to send, message sender not initialized");
            }
        }

    });

    pc_->onLocalCandidate([weakSelf](rtc::Candidate candidate){
        if (auto self = weakSelf.lock()){

            self->logger_->info("Sending Local Candidate to Peer");
            std::string candidateStr = std::string(candidate);
            std::string mid = candidate.mid();

            nlohmann::json candidateMessage;

            candidateMessage["type"] = "candidate";
            candidateMessage["candidate"] = candidateStr;
            candidateMessage["mid"] = mid;

            if (self->sendMessage_) {
                self->sendMessage_(candidateMessage.dump());
            }
            else {
                self->logger_->error("Candidate failed to send, message sender not initialized");
            }
        }

    });

    // we add all possible tracks at the start to avoid re-negotiation
    // caused by adding tracks mid-connection.
    initTracks();

}

void PeerSession::setEmitHandler(messageSender handler){

    // sendMessage is of type messageSender
    // essentially, it expects a lamda, and 
    // is a function on it's own
    sendMessage_ = handler;

}

void PeerSession::initTracks(){
    for (int i=1; i <= cameraCount_; ++i) {
        std::string trackName = "Track" + std::to_string(i);
        auto initDirection = rtc::Description::Direction::SendOnly;

        auto video = rtc::Description::Video(trackName, initDirection);
        video.addH264Codec(96);

        uint32_t ssrc = 100 + i;
        std::string cName = "Camera" + std::to_string(i);

        auto rtpConfig = std::make_shared<rtc::RtpPacketizationConfig>(ssrc, cName, 96, 90000);
        auto rtpPacketizer = std::make_shared<rtc::H264RtpPacketizer>(
            rtc::H264RtpPacketizer::Separator::LongStartSequence, 
            rtpConfig
        );

        // --- NEW: Add RTCP and Extensions ---
        auto nackResponder = std::make_shared<rtc::RtcpNackResponder>();
        rtpPacketizer->addToChain(nackResponder);
        
        // Helps Chrome map the incoming packets to the correct track
        auto sdpHandler = std::make_shared<rtc::SdpExtensionHandler>(rtpConfig);
        rtpPacketizer->addToChain(sdpHandler);

        auto track = pc_->addTrack(video);
        
        // --- CRITICAL: Explicitly set SSRC for the track ---
        track->setSSRC(ssrc); 
        
        track->setMediaHandler(rtpPacketizer);
        trackMap_[trackName] = track;
    }
}
std::shared_ptr<rtc::Track> PeerSession::getTrack(const std::string& trackName){

    if (trackMap_.contains(trackName)){
        return trackMap_[trackName];
    } else{
        logger_->info("Track does not exist");
        return nullptr;
    }
    
}

void PeerSession::initPC(){

    peerSessionSetup();
    pc_->setLocalDescription();

}

void PeerSession::recieveAnswer(const nlohmann::json& remoteDescription){

    std::string type = remoteDescription["type"];
    std::string sdp = remoteDescription["sdp"];

    // create remote description 
    rtc::Description remoteDesc(sdp, type);

    try{
        pc_->setRemoteDescription(remoteDesc);
    } catch (std::exception& e) {
        logger_->error("Error occured when setting remote sdp {}", e.what());
    }
    
}

void PeerSession::recieveCandidates(const nlohmann::json& candidatesJson){

    std::string candidate = candidatesJson["candidate"];
    std::string mid = candidatesJson["mid"];

    if (candidate.empty()){
        logger_->info("Candidate Exchange Completed");
        return;
    }

    rtc::Candidate remoteCandidate(candidate, mid);

    try {
        pc_->addRemoteCandidate(remoteCandidate);
    } catch (std::exception& e) {
        logger_->error("Error occured when setting remote candidate {}", e.what());
    }

}

void PeerSession::closePC(){
    if (pc_) {
        pc_->close();
    }
    trackMap_.clear();
    pc_ = nullptr;
}

PeerSession::~PeerSession() {
    closePC();
}