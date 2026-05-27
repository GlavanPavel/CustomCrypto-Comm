#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

class TcpSocket {
private:
    SOCKET sockFd;

public:
    TcpSocket();
    TcpSocket(SOCKET fd);
    ~TcpSocket();

    TcpSocket(TcpSocket&& other) noexcept;             
    TcpSocket& operator=(TcpSocket&& other) noexcept;  

    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;

    bool connectToServer(const std::string& ip, const std::string& port);
    bool sendData(const std::string& data);
    std::string receiveData(size_t maxBytes = 2048);
    void closeSocket();

    SOCKET getFd() const { return sockFd; }
    bool isValid() const;
};

#endif // TCP_SOCKET_H