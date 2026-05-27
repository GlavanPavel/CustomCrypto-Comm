#include "../include/Client.h"
#include "../include/RC6.h"  
#include "../include/network_utils.h"
#include <iostream>
#include <chrono>
#include <array>

Client::Client(const std::string& ip, const std::string& port, Looper* looper)
    : eventLooper(looper), cryptoContext(nullptr), secureSessionEstablished(false), running(false) {
    
    std::cout << "[Client]: Attempting connection to server" << std::endl;
    if (socketFacade.connectToServer(ip, port)) {
        std::cout << "[Client]: Connected successfully to server." << std::endl;
        
        eventLooper->addPollable(socketFacade.getFd(), this, POLLIN);
    } else {
        std::cerr << "[Client]: Fatal connection drop out." << std::endl;
    }
}

Client::~Client() {
    running = false;
    if (inputThread.joinable()) {
        inputThread.join();
    }
    eventLooper->removePollable(socketFacade.getFd());
    delete cryptoContext;
}

void Client::start() {
    std::cout << "Enter username to register on server:\n> ";
    std::getline(std::cin, username);
    
    // first message registers the nickname on the server
    socketFacade.sendData(username);

    running = true;
    inputThread = std::thread(&Client::runInputLoop, this);
}

// triggered by the Looper whenever a packet arrives from the server
void Client::handleEvent(short revents) {
    if (revents & POLLIN) {
        std::string rawData = socketFacade.receiveData();
        if (rawData.empty()) {
            std::cout << "\n[System]: Server dropped connection" << std::endl;
            running = false;
            return;
        }

        // check for server acknowledgment
        if (rawData == "Registered successfully!" || rawData == "msg sent" || rawData == "user not found") {
            std::cout << "\n[Server Ack]: " << rawData << "\n> " << std::flush;
            return;
        }

        // Parse incoming message packets
        std::array<std::string, 4> packetDetails;
        getPacketDetails(rawData, packetDetails);

        std::string sender = packetDetails[0];
        std::string payload = packetDetails[2];

        // Diffie-Hellman public key payload exchange
        if (payload.rfind("DH_KEY:", 0) == 0) {
            std::string hexKey = payload.substr(7);
            
            size_t initPos = hexKey.find("_INIT");
            if (initPos != std::string::npos) {
                hexKey = hexKey.substr(0, initPos);
            }
            
            CryptoPP::Integer peerPublicKey(hexKey.c_str());

            if (!cryptoContext) {
                CryptoPP::Integer p("1234567890123456789012345678901234567890");
                CryptoPP::Integer g = 5;
                cryptoContext = new Party(p, g);
            }

            cryptoContext->create_shared_secret(peerPublicKey);
            secureSessionEstablished = true;
            targetPeer = sender;

            std::cout << "\n[System]: Secure channel established with [" << sender << "]!" << std::endl;
            
            // send public key back
            if (payload.find("_INIT") != std::string::npos) {
                std::string myKeyStr = integer_to_string(cryptoContext->sendPublicKey());
                sendPacket(sender, "DH_KEY:" + myKeyStr);
            }
            std::cout << "> " << std::flush;
        }
        // regular message
        else {
            if (secureSessionEstablished && sender == targetPeer) {
                std::vector<uint8_t> bytes(payload.begin(), payload.end()); 
                std::vector<uint32_t> encrypted_uints = byte_to_uint32_t(bytes);
                
                std::string decryptedText = cryptoContext->decrypt_message(encrypted_uints);
                std::cout << "\n[" << sender << " (Encrypted)]: " << decryptedText << "\n> " << std::flush;
            } else {
                std::cout << "\n[" << sender << " (Unencrypted)]: " << payload << "\n> " << std::flush;
            }
        }
    }
}

void Client::runInputLoop() {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "\nCommands:\n /connect <user> - Initiate secure key exchange\n <message> - Send a text message\n";
    
    while (running) {
        std::string input;
        std::cout << "> " << std::flush;
        std::getline(std::cin, input);

        if (input.empty()) continue;

        if (input.rfind("/connect ", 0) == 0) {
            targetPeer = input.substr(9);
            
            std::cout << "[System]: Generating DH keys and initiating handshake with " << targetPeer << std::endl;
            
            CryptoPP::Integer p("1234567890123456789012345678901234567890");
            CryptoPP::Integer g = 5;
            cryptoContext = new Party(p, g);

            std::string myKeyStr = integer_to_string(cryptoContext->sendPublicKey());
            
            // _INIT so the target peer knows to send their public key back
            sendPacket(targetPeer, "DH_KEY:" + myKeyStr + "_INIT");
        } 
        // message
        else {
            if (targetPeer.empty()) {
                std::cout << "[Error]: Specify a recipient first using: /connect <username>" << std::endl;
                continue;
            }

            if (secureSessionEstablished && cryptoContext) {
                std::vector<uint32_t> encrypted_uints = cryptoContext->encrypt_message(input);
                std::vector<uint8_t> bytes = uint32_t_to_byte(encrypted_uints);
                
                std::string cipherText(bytes.begin(), bytes.end());
                sendPacket(targetPeer, cipherText);
            } else {
                sendPacket(targetPeer, input);
            }
        }
    }
}

void Client::sendPacket(const std::string& receiver, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::string timeStamp = std::ctime(&time_t_now);
    if (!timeStamp.empty()) timeStamp.pop_back();

    std::string fullPacket = username + "|" + receiver + "|" + message + "|" + timeStamp;
    socketFacade.sendData(fullPacket);
}

void Client::getPacketDetails(const std::string& packet, std::array<std::string, 4>& packetDetails) {
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