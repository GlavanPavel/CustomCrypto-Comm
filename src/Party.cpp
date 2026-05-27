#include "../include/Party.h"
#include "../include/RC6.h"
#include <iostream>
#include <iomanip>
using namespace std;
// Standalone mathematical utility function used for modular exponentiation
CryptoPP::Integer calc(CryptoPP::Integer base, CryptoPP::Integer exp, CryptoPP::Integer module) {
    CryptoPP::Integer result = 1;
    while (exp != 1) {
        if (exp % 2 == 1) {
            result = (result * base) % module;
            exp--;
        }
        exp = exp / 2;
        base = (base * base) % module;
    }
    result = (result * base) % module;
    return result;
}

Party::Party() {
    CryptoPP::AutoSeededRandomPool rnd;
    CryptoPP::PrimeAndGenerator pg;
    pg.Generate(1, rnd, k, k - 1);
    p = pg.Prime();
    g = pg.Generator();
    createKeys();
}

Party::Party(CryptoPP::Integer P, CryptoPP::Integer G) {
    p = P;
    g = G;
    createKeys();
}

CryptoPP::Integer Party::getP() { 
    return p; 
}

CryptoPP::Integer Party::getG() { 
    return g; 
}

CryptoPP::Integer Party::sendPublicKey() { 
    return public_key; 
}

void Party::create_shared_secret(CryptoPP::Integer key_received) {
    shared_secret = calc(key_received, private_key, p);
    getKeyFromSecret();
}

std::vector<uint32_t> Party::encrypt_data(std::vector<uint32_t> data) {
    while (data.size() % 4 != 0)
        data.push_back(0);
    
    CryptoPP::AutoSeededRandomPool rnd;
    std::vector<uint32_t> iv(4);
    for(int i = 0; i < 4; i++) {
        iv[i] = (uint32_t)rnd.GenerateWord32();
    }
    data.insert(data.begin(), iv.begin(), iv.end());

    for (size_t i = 4; i < data.size(); i += 4) {
        uint32_t block[4];
        for(int j = 0; j < 4; j++) {
            block[j] = data[i+j] ^ data[i+j-4];
        }
        encrypt_block(block, S);
        for (int j = 0; j < 4; j++) {
            data[i + j] = block[j];
        }
    }
    return data;
}

std::vector<uint32_t> Party::decrypt_data(std::vector<uint32_t> data) {
    std::vector<uint32_t> prev = {data[0], data[1], data[2], data[3]};
    for (size_t i = 4; i < data.size(); i += 4) {
        std::vector<uint32_t> next_prev = {data[i], data[i + 1], data[i + 2], data[i + 3]};
        uint32_t block[4] = {data[i], data[i + 1], data[i + 2], data[i + 3]};
        decrypt_block(block, S);
        for (int j = 0; j < 4; j++) {
            data[i + j] = block[j] ^ prev[j];
        }
        prev = next_prev;
    }
    data.erase(data.begin(), data.begin() + 4);
    return data;
}

std::vector<uint32_t> Party::encrypt_message(std::string message) {
    std::vector<uint8_t> msg(message.begin(), message.end());
    std::vector<uint32_t> data = byte_to_uint32_t(msg);
    return encrypt_data(data);
}

std::string Party::decrypt_message(std::vector<uint32_t> data) {
    std::vector<uint32_t> plaintext = decrypt_data(data);
    std::vector<uint8_t> data_byte = uint32_t_to_byte(plaintext);

    // Filter out padding artifacts (0x00 null bytes) from the text stream
    std::string cleanMessage = "";
    for (size_t i = 0; i < data_byte.size(); ++i) {
        if (data_byte[i] != 0x00) {
            cleanMessage += static_cast<char>(data_byte[i]);
        }
    }
    return cleanMessage;
}

void Party::createKeys() {
    CryptoPP::AutoSeededRandomPool rnd;
    private_key = CryptoPP::Integer(rnd, 2, p - 2);
    public_key = calc(g, private_key, p);
}

void Party::getKeyFromSecret() {
    size_t key_size = 32;
    std::vector<uint8_t> key(key_size);
    size_t secret_size = shared_secret.MinEncodedSize();
    std::vector<uint8_t> secret(secret_size);
    shared_secret.Encode(secret.data(), secret_size);
    CryptoPP::SHA256 hash;
    hash.Update(secret.data(),secret.size());
    hash.Final(key.data());
    S = keySchedule(key);
}

void print_data(std::vector<uint32_t> data) {
    for (size_t i = 0; i < data.size(); i++)
        std::cout << std::hex << std::setw(8) << std::setfill('0') << data[i] << " ";
    std::cout << std::dec << std::endl;
}
