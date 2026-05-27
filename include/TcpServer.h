#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <winsock2.h>

#include "Looper.h"
#include "TcpSocket.h"
#include <unordered_map>
#include <string>
#include <array>

class TcpServer : public Pollable {
private:
    TcpSocket serverSocket;
    Looper* eventLooper;
    int serverPort;

    // windows socket -> TcpSocket
    std::unordered_map<SOCKET, TcpSocket*> activeClients;
    std::unordered_map<SOCKET, Pollable*> activeAdapters;
    
    // username -> tcpSocket
    std::unordered_map<std::string, TcpSocket*> clientRoutingMap; 
    
    void handleNewConnection();
    void handleClientMessage(SOCKET clientFd);
    void handleDisconnect(SOCKET clientFd);

    void getPacketDetails(const std::string& packet, std::array<std::string, 4>& packetDetails);

public:
    TcpServer(int port, Looper* looper);
    void broadcastUserList();
     virtual ~TcpServer();

    virtual void handleEvent(short revents) override;
};

#endif // TCP_SERVER_H