#include "../include/Client.h"
#include "../include/RC6.h"  
#include "../include/network_utils.h"
#include <iostream>
#include <chrono>
#include <array>
#include <sstream>
#include <vector>

using namespace std;

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
    eventLooper->removePollable(socketFacade.getFd());
    delete cryptoContext;
}

void Client::sendMessage(const std::string&receiver, const std::string& message)
{
    targetPeer = receiver;
    if(secureSessionEstablished && cryptoContext){
        vector<uint32_t> encrypted_message = cryptoContext->encrypt_message(message);
        vector<uint8_t> bytes = uint32_t_to_byte(encrypted_message);
        string cipherText(bytes.begin(), bytes.end());
        sendPacket(receiver,cipherText);
    }else{
        sendPacket(receiver, message);
    }
}

void Client::start(const std::string& user) {
    username = user;
    socketFacade.sendData(username);
    running = true;
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
        size_t userListPos = rawData.find("USERLIST:");
        if (userListPos != std::string::npos)
        {
            std::string payload = rawData.substr(userListPos + 9);

            std::vector<std::string> users;
            std::stringstream ss(payload);
            std::string user;

            while (std::getline(ss, user, ','))
            {
                while (!user.empty() && user.front() == ' ')
                    user.erase(user.begin());

                while (!user.empty() && user.back() == ' ')
                    user.pop_back();

                if (!user.empty() && user != username)
                {
                    users.push_back(user);
                }
            }

            if (onUserListReceived)
            {
                onUserListReceived(users);
            }

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
                if(onMessageReceived){
                    onMessageReceived(sender,decryptedText);
                }
            } else {
                if(onMessageReceived){
                    onMessageReceived(sender,payload);
                }
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