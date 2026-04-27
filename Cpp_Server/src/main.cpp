#include "signalling/PeerSession.hpp"
#include "signalling/SignallingServer.hpp"
#include "logging/logger.hpp"
#include "camera/OakDLiteSource.hpp"
#include "encoder/Encoder.hpp"


int main() {

    Logger::init();
    auto logger_ = spdlog::stdout_color_mt("Main");

    logger_->info("=== Main Session Initiating ===");

    auto camera1 = OakDLiteSource("camera1");
    
    camera1.start();
    auto initFrame = camera1.getFrame();
    logger_->info("Initial Frame Captured");

    logger_->info("frame height {}", initFrame->height);
    logger_->info("frame width {}", initFrame->width);

    auto encoder1 = Encoder(initFrame->width, initFrame->height, 10);

    auto peerSession = PeerSession::init(1);
    auto server = SignallingServer::init("0.0.0.0", 5000, peerSession);


    std::thread cameraThread([logger_, peerSession, &camera1, &encoder1]() {
        std::shared_ptr<rtc::Track> track;
        std::shared_ptr<rtc::RtpPacketizationConfig> rtpConfig;
        
        while (true) {
            if (track && track->isOpen()) {
                auto frame = camera1.getFrame();
                auto encodedFrame = encoder1.encodeFrame(frame);

                if (encodedFrame.empty()) {
                    continue;
                }

                rtpConfig->timestamp += 9000;
                track->send(reinterpret_cast<const std::byte*>(encodedFrame.data()), encodedFrame.size());
            } else { 
                track = peerSession->getTrack("Track1");
                rtpConfig = peerSession->getRtpConfig("Track1");
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            }
        }
    });


    cameraThread.join(); 

    logger_->info("=== Main Session Destructing ===");

    return 0;
}