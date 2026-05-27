#ifndef LOOPER_H
#define LOOPER_H

#include <winsock2.h>

#include <vector>
#include <unordered_map>

class Pollable {
public:
    virtual ~Pollable() = default;
    virtual void handleEvent(short revents) = 0;
};

class Looper {
private:
    std::vector<WSAPOLLFD> pfds;
    std::unordered_map<SOCKET, Pollable*> handlers;
    bool running;

public:
    Looper();
    ~Looper() = default;

    void addPollable(SOCKET fd, Pollable* handler, short events);
    void removePollable(SOCKET fd);

    int run();
    void stop();
};

#endif // LOOPER_H