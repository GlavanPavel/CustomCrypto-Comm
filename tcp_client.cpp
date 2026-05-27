#include "include/Looper.h"
#include "include/Client.h"
#include <iostream>

int main() {
    std::cout << "Starting client channel" << std::endl;
    Looper clientLooper;
    
    Client client("127.0.0.1", "4000", &clientLooper);
    client.start();

    return clientLooper.run();
}