#include "../include/TcpSocket.h"
#include <iostream>
#include <cstring>

void initializeNetwork() {
    static bool initialized = false;
    if (!initialized) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "Failed to initialize Winsock\n";
        }
        initialized = true;
    }
}

TcpSocket::TcpSocket() {
    initializeNetwork();
    sockFd = INVALID_SOCKET;
}

TcpSocket::TcpSocket(SOCKET fd) : sockFd(fd) {}

TcpSocket::TcpSocket(TcpSocket&& other) noexcept {
    this->sockFd = other.sockFd;
    other.sockFd = INVALID_SOCKET;
}

TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept {
    std::cout << "[Socket Trace]: Move ASSIGNMENT operator triggered." << std::endl;
    if (this != &other) {
        this->closeSocket();
        
        this->sockFd = other.sockFd;
        std::cout << "  -> Stole FD " << this->sockFd << " from temporary object." << std::endl;
        other.sockFd = INVALID_SOCKET;
    }
    return *this;
}

TcpSocket::~TcpSocket() {
    if (isValid()) {
        closeSocket();
    }
}

bool TcpSocket::isValid() const {
    return sockFd != INVALID_SOCKET;
}

bool TcpSocket::connectToServer(const std::string& ip, const std::string& port) {
    struct addrinfo hints, *res = nullptr;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(ip.c_str(), port.c_str(), &hints, &res) != 0) {
        std::cerr << "Failed to resolve address\n";
        return false;
    }

    sockFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (!isValid()) {
        std::cerr << "Failed to create socket\n";
        freeaddrinfo(res);
        return false;
    }

    if (connect(sockFd, res->ai_addr, res->ai_addrlen) < 0) {
        std::cerr << "Failed to connect to server\n";
        closeSocket();
        freeaddrinfo(res);
        return false;
    }

    freeaddrinfo(res);
    return true;
}

bool TcpSocket::sendData(const std::string& data) {
    if (!isValid()) return false;
    
    int sent = send(sockFd, data.c_str(), static_cast<int>(data.size()), 0);
    return sent > 0;
}

std::string TcpSocket::receiveData(size_t maxBytes) {
    if (!isValid()) return "";

    std::string buffer(maxBytes, '\0');
    int bytesRead = recv(sockFd, &buffer[0], static_cast<int>(maxBytes - 1), 0);

    if (bytesRead <= 0) {
        closeSocket();
        return "";
    }

    buffer.resize(bytesRead);
    return buffer;
}

void TcpSocket::closeSocket() {
    if (isValid()) {
        SOCKET temp = sockFd;
        sockFd = INVALID_SOCKET; 
        closesocket(temp);
    }
}