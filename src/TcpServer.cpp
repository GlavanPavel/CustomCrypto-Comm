#include "../include/TcpServer.h"
#include <iostream>
#include <cstring>

TcpServer::TcpServer(int port, Looper* looper) : eventLooper(looper), serverPort(port) {
    struct addrinfo hints, *res = nullptr;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    std::string portStr = std::to_string(port);
    getaddrinfo(nullptr, portStr.c_str(), &hints, &res);

    SOCKET rawFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (rawFd == INVALID_SOCKET) {
        std::cerr << "[Server Error]: Socket creation failed. Code: " << WSAGetLastError() << std::endl;
    }

    int opt = 1;
    setsockopt(rawFd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
    
    if (bind(rawFd, res->ai_addr, res->ai_addrlen) < 0) {
        std::cerr << "[Server Error]: Bind failed! Code: " << WSAGetLastError() << std::endl;
    }
    
    listen(rawFd, 5);
    freeaddrinfo(res);

    // Explicitly invoke our modern Move Assignment operator safely
    this->serverSocket = std::move(TcpSocket(rawFd));

    // Pass the raw file descriptor handle straight into our Windows Looper pass
    eventLooper->addPollable(this->serverSocket.getFd(), this, POLLIN);
}

TcpServer::~TcpServer() {
    eventLooper->removePollable(serverSocket.getFd());
    for (auto pair : activeClients) {
        delete pair.second;
    }
    for (auto pair : activeAdapters) {
        delete pair.second;
    }
}

// Triggered by the Looper whenever activity happens on our server listener socket
void TcpServer::handleEvent(short revents) {
    if (revents & POLLIN) {
        handleNewConnection();
    }
}

void TcpServer::handleNewConnection() {
    sockaddr_in clientAddr{};
    int clientAddrLen = sizeof(clientAddr);

    // Direct extraction of incoming client Windows handle descriptor
    SOCKET clientFd = accept(serverSocket.getFd(), reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);
    if (clientFd == INVALID_SOCKET) return;

    std::cout << "[Server]: A new raw peer hit accept connection loop." << std::endl;

    // Wrap the raw channel file descriptor inside a dynamic facade instance
    TcpSocket* clientSocketObj = new TcpSocket(clientFd);
    activeClients[clientFd] = clientSocketObj;

    // Custom class wrapper adapter to bridge the client events back into our routing loop
    class ClientConnectionAdapter : public Pollable {
    private:
        TcpServer* outerServer;
        SOCKET targetFd;
    public:
        ClientConnectionAdapter(TcpServer* s, SOCKET fd) : outerServer(s), targetFd(fd) {}
        
        virtual void handleEvent(short revents) override {
            if (revents & POLLIN) {
                outerServer->handleClientMessage(targetFd);
            } else if (revents & (POLLERR | POLLHUP)) {
                outerServer->handleDisconnect(targetFd);
            }
        }
    } *adapter = new ClientConnectionAdapter(this, clientFd);

    activeAdapters[clientFd] = adapter;

    // Register this fresh client connection socket back into the Windows Looper
    eventLooper->addPollable(clientFd, adapter, POLLIN | POLLHUP | POLLERR);
}

void TcpServer::handleClientMessage(SOCKET clientFd) {
    TcpSocket* client = activeClients[clientFd];
    std::string data = client->receiveData();

    if (data.empty()) {
        handleDisconnect(clientFd);
        return;
    }

    // Step A: Determine if this packet is an Initial Account Identity Registration Frame
    bool foundInRouting = false;
    for (auto const& pair : clientRoutingMap) {
        if (pair.second->getFd() == clientFd) {
            foundInRouting = true;
            break;
        }
    }

    if (!foundInRouting) {
        // First payload packet received from an anonymous node is treated as username
        clientRoutingMap[data] = client;
        client->sendData("Registered successfully!");
        std::cout << "[Server]: User registered handle identifier -> " << data << std::endl;
        return;
    }

    // Step B: Route message frame directly to requested end point
    std::array<std::string, 4> packetDetails;
    getPacketDetails(data, packetDetails);

    std::string sender = packetDetails[0];
    std::string targetReceiver = packetDetails[1];
    
    if (clientRoutingMap.find(targetReceiver) != clientRoutingMap.end()) {
        // Rebuild clean transmission payload and inject it directly to target's output buffer
        std::string payloadRelay = sender + "|" + targetReceiver + "|" + packetDetails[2] + "|" + packetDetails[3];
        clientRoutingMap[targetReceiver]->sendData(payloadRelay);
        client->sendData("msg sent");
    } else {
        client->sendData("user not found");
    }
}

void TcpServer::handleDisconnect(SOCKET clientFd) {
    std::cout << "[Server]: Peer socket closed connection stream." << std::endl;
    eventLooper->removePollable(clientFd);
    
    TcpSocket* obj = activeClients[clientFd];
    activeClients.erase(clientFd);

    if (activeAdapters.find(clientFd) != activeAdapters.end()) {
        Pollable* adapterObj = activeAdapters[clientFd];
        activeAdapters.erase(clientFd);
        delete adapterObj;
    }

    for (auto it = clientRoutingMap.begin(); it != clientRoutingMap.end();) {
        if (it->second == obj) {
            it = clientRoutingMap.erase(it);
        } else {
            ++it;
        }
    }
    delete obj;
}

void TcpServer::getPacketDetails(const std::string& packet, std::array<std::string, 4>& packetDetails) {
    size_t senderIndex = packet.find("|");
    std::string sender = packet.substr(0, senderIndex);
    std::string remainingPacket = packet.substr(senderIndex + 1);
                           
    size_t receiverIndex = remainingPacket.find("|");    
    std::string receiver = remainingPacket.substr(0, receiverIndex);
    remainingPacket = remainingPacket.substr(receiverIndex + 1);
    
    size_t msgIndex = remainingPacket.find("|");
    std::string message = remainingPacket.substr(0, msgIndex);
    std::string timeStamp = remainingPacket.substr(msgIndex + 1);

    packetDetails = {sender, receiver, message, timeStamp};
}