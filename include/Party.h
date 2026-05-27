#ifndef PARTY_H
#define PARTY_H

#include <vector>
#include <string>
#include <cryptopp/integer.h>
#include <cryptopp/osrng.h>
#include <cryptopp/nbtheory.h>

class Party
{
private:
    CryptoPP::Integer private_key;
    CryptoPP::Integer public_key;
    CryptoPP::Integer p, g;
    CryptoPP::Integer shared_secret;
    int k = 256;
    std::vector<uint32_t> S;

    void createKeys();
    void getKeyFromSecret();

public:
    Party();
    Party(CryptoPP::Integer P, CryptoPP::Integer G);

    CryptoPP::Integer getP();
    CryptoPP::Integer getG();
    CryptoPP::Integer sendPublicKey();
    void create_shared_secret(CryptoPP::Integer key_received);
    
    std::vector<uint32_t> encrypt_data(std::vector<uint32_t> data);
    std::vector<uint32_t> decrypt_data(std::vector<uint32_t> data);
    
    std::vector<uint32_t> encrypt_message(std::string message);
    std::string decrypt_message(std::vector<uint32_t> data);
};

void print_data(std::vector<uint32_t> data);

#endif // PARTY_H