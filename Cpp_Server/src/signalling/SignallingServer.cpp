#include "signalling/SignallingServer.hpp"
#include "logging/logger.hpp"

std::shared_ptr<SignallingServer> SignallingServer::init(const std::string ip, const int port, std::shared_ptr<PeerSession> session) {

    auto server = std::shared_ptr<SignallingServer>(new SignallingServer(ip, port, session));

    server->startServer();

    return server;
}

SignallingServer::SignallingServer(const std::string ip, const int port, std::shared_ptr<PeerSession> session)
        : port_(port), ip_(ip), peerSession_(session){

    // initialize logger
    logger_ = Logger::createLogger("SignallingServer");
    logger_->info("SignallingServer Initialized");
}

void SignallingServer::startServer() {

    // init WebSocket Server config
    rtc::WebSocketServer::Configuration wsConfig;
    wsConfig.port = port_;
    wsConfig.bindAddress = ip_;

    // start WebSocket
    try{
        wsServer_ = std::make_shared<rtc::WebSocketServer>(wsConfig);
    } catch (const std::exception& e) {
        logger_->critical("Failed to start WebSocket Server: {}", e.what());
        return;
    }

    logger_->info("Websocket server started on: {}:{}", ip_, port_);

    auto weakSelf = weak_from_this();

    // set Event Callbacks
    wsServer_->onClient([weakSelf](std::shared_ptr<rtc::WebSocket> wsClient) {
        if (auto self = weakSelf.lock()) {
            self->logger_->info("Client Connection Established!");
            self->registerClient(wsClient);
        }
    });

}

void SignallingServer::registerClient(std::shared_ptr<rtc::WebSocket>& wsClient) {

    if (wsClient_){
        // safe to remove old client since we can assume we will only have one client
        // therefore old client may be a connection that is closing
        logger_->warn("New client attempting to connect while old client exists. Closing old client...");
        deRegisterClient();
    }

    // register client
    wsClient_ = wsClient;

    // create weak pointers
    std::weak_ptr<rtc::WebSocket> weakClient = wsClient_;
    auto weakSelf = weak_from_this();

    // set up client websocket call backs

    // use weak client to avoid cyclic dependency on wsClient
    // we don't want client to be copied by value otherwise if
    // client disconnects it won't be cleaned up in memory.
    wsClient->onOpen([weakSelf, weakClient](){

        auto ws = weakClient.lock();
        auto self = weakSelf.lock();

        if (!ws || !self){
            // fail fast
            return;
        }

        self->logger_->info("Websocket Client Opened");
        ws->send("Hello from SignallingServer");
        
        self->peerSession_->setEmitHandler([weakClient](const std::string& data){
            if (auto ws = weakClient.lock()){
                ws->send(data);
            }
        });

    });

    wsClient->onMessage([weakSelf](auto data){
        if (auto self = weakSelf.lock()){
            self->messageCallback(data);
        }
    });

    wsClient->onError([weakSelf](auto error){
        if (auto self = weakSelf.lock()){
            self->logger_->error("Websocket Error Occured: {}", error);
            self->deRegisterClient();
        }
    });

    wsClient->onClosed([weakSelf](){
        if (auto self = weakSelf.lock()){
            self->logger_->info("Websocket Client Closed");
            self->deRegisterClient();
        }
    });

    logger_->info("Client Websocket Registered");

}

void SignallingServer::deRegisterClient() {

    if (!wsClient_) return;

    // deregister client websocket by setting it to nullptr
    logger_->info("Deregistering client websocket");
    peerSession_->setEmitHandler([](const std::string& data){});
    wsClient_ = nullptr;
    
}

void SignallingServer::messageCallback(std::variant<rtc::binary, std::string> msg) {
    
    // check what data type we were sent
    if (std::holds_alternative<std::string>(msg)) {

        std::string string_msg = std::get<std::string>(std::move(msg));

        try{

            auto json_msg = nlohmann::json::parse(string_msg);

            if (json_msg.contains("type")) {

                auto type = json_msg["type"];
                logger_->info("Websocket Message Recieved, Type: {}", type);

                if (type == "ping") {
                    sendMessage(json_msg);
                } else if (type == "startPC") {
                    startPeerConnection();
                } else if (type == "closePC") {
                    closePeerConnection();
                } else if (type == "answer") {
                    handleRemoteDescription(json_msg);
                } else if (type == "candidate") {
                    handleCandidates(json_msg);
                } else {
                    logger_->warn("Unsupported message type recieved from peer");
                }

            } else if (json_msg.contains("text")) {
                logger_->info("Peer Message: '{}'", json_msg["text"]);
            } 

        } catch (nlohmann::json::parse_error& e) {
            logger_->error("Error occured when parsing string: {}", e.what());

            //nlohmann::json error_response;
            //error_response["type"] = "error";
            //possible idea of asking for client to resend?

        }
            
    }

}

void SignallingServer::startPeerConnection(){

    logger_->info("Starting PeerConnection");
    peerSession_->initPC();

}

void SignallingServer::closePeerConnection(){

    logger_->info("Closing PeerConnection");
    peerSession_->closePC();

}

void SignallingServer::handleRemoteDescription(const nlohmann::json& remoteDescription){

    logger_->info("Remote description recieved from peer");
    peerSession_->recieveAnswer(remoteDescription);

}

void SignallingServer::handleCandidates(const nlohmann::json& candidatesJson){

    logger_->info("ICE Candidates recieved from peer");
    peerSession_->recieveCandidates(candidatesJson);

}


void SignallingServer::sendMessage(const nlohmann::json& message) {

    if (message["type"] == "ping") {
        if (wsClient_) {
            logger_->info("Sending ping response to client");
            wsClient_->send("pong");
        } else {
            logger_->warn("Client Websocket is inactive");
        }
    }
}

SignallingServer::~SignallingServer() {

}