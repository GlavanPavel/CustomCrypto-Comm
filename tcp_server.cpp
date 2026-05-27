#include "include/Looper.h"
#include "include/TcpServer.h"
#include <iostream>

int main() {
    std::cout << "Starting server" << std::endl;
    
    std::cout << "[Main]: Instantiating Looper" << std::endl;
    Looper mainLooper;
    
    std::cout << "[Main]: Started TcpServer on port 4000" << std::endl;
    TcpServer server(4000, &mainLooper);
    
    return mainLooper.run();
}