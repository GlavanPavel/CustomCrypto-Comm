#include "../include/Looper.h"
#include <iostream>
#include <thread>
#include <chrono>

Looper::Looper() : running(false) {}

void Looper::addPollable(SOCKET fd, Pollable* handler, short events) {
    WSAPOLLFD pfd;
    pfd.fd = fd;
    
    pfd.events = POLLIN; 
    pfd.revents = 0;
    
    pfds.push_back(pfd);
    handlers[fd] = handler;
    
    std::cout << "[Looper Track]: Added FD " << fd << " with final mask: " << pfd.events << std::endl;
}

void Looper::removePollable(SOCKET fd) {
    for (auto it = pfds.begin(); it != pfds.end(); ++it) {
        if (it->fd == fd) {
            pfds.erase(it);
            break;
        }
    }
    handlers.erase(fd);
}

int Looper::run() {
    running = true;
    std::cout << "[Looper]: Main event loop activated." << std::endl;

    while (running) {
        if (pfds.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        int pollCount = WSAPoll(pfds.data(), static_cast<ULONG>(pfds.size()), -1);

        if (pollCount < 0) {
            std::cerr << "[Looper]: WSAPoll error: " << WSAGetLastError() << std::endl;
            return -1;
        }

        size_t currentSize = pfds.size();
        for (size_t i = 0; i < currentSize; ++i) {
            if (pfds[i].revents != 0) {
                auto it = handlers.find(pfds[i].fd);
                if (it != handlers.end()) {
                    it->second->handleEvent(pfds[i].revents);
                }
                pfds[i].revents = 0;
            }
        }
    }
    return 0;
}

void Looper::stop() {
    running = false;
}