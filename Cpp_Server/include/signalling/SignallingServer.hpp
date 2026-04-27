#include <rtc/rtc.hpp>
#include <variant>
#include <nlohmann/json.hpp>

#include "PeerSession.hpp"

// SignallingServer is designed to only handle one websocket, at this
// point in time I don't see a need to have it handle multiple 
// server / client websockets.
class SignallingServer : public std::enable_shared_from_this<SignallingServer> {
public:

    // create a factory method for signalling server to ensure it's created properly
    // the classes constructor has been made private and this method will call the constructor
    static std::shared_ptr<SignallingServer> init(
        const std::string ip, const int port, std::shared_ptr<PeerSession> session
    );

    void startServer();
    void registerClient(std::shared_ptr<rtc::WebSocket>&);
    void deRegisterClient();
    void messageCallback(std::variant<rtc::binary, std::string>);
    void sendMessage(const nlohmann::json&);
    void startPeerConnection();
    void closePeerConnection();
    void handleRemoteDescription(const nlohmann::json&);
    void handleCandidates(const nlohmann::json&);

    ~SignallingServer();

private:

    // private constructor, signalling server must be created through init function
    SignallingServer(const std::string ip, const int port, std::shared_ptr<PeerSession> session);
    
    // websocket resources
    std::shared_ptr<rtc::WebSocketServer> wsServer_;
    std::shared_ptr<rtc::WebSocket> wsClient_;

    // webRTC resources 
    std::shared_ptr<PeerSession> peerSession_;
    
    // server resources
    const int port_;
    std::string ip_;
    std::shared_ptr<spdlog::logger> logger_;


};