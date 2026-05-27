#ifndef CLIENT_H
#define CLIENT_H

#include "Looper.h"
#include "TcpSocket.h"
#include "Party.h"
#include <string>
#include <thread>

class Client : public Pollable {
private:
    TcpSocket socketFacade;
    Looper* eventLooper;
    std::string username;
    std::string targetPeer; // username of the one we're messaging
    
    Party* cryptoContext;
    bool secureSessionEstablished;

    std::thread inputThread;
    bool running;

    void runInputLoop();
    void sendPacket(const std::string& receiver, const std::string& message);
    void getPacketDetails(const std::string& packet, std::array<std::string, 4>& packetDetails);

public:
    Client(const std::string& ip, const std::string& port, Looper* looper);
    virtual ~Client();

    void start();

    virtual void handleEvent(short revents) override;
};

#endif // CLIENT_H