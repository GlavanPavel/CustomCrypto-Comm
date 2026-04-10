#ifndef HEADER_H
#define HEADER_H

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <bitset>
#include <cassert>

#include <cryptopp/integer.h>
#include <cryptopp/osrng.h>
#include <cryptopp/nbtheory.h>

using namespace std;
using namespace CryptoPP;

//RC6
void decrypt_block(uint32_t registers[4], const std::vector<uint32_t> &S);
void encrypt_block(uint32_t registers[4], const std::vector<uint32_t> &S);
std::vector<uint32_t> keySchedule(std::vector<uint8_t> &key);
void print_block(std::uint32_t b[4]);

//Party
class Party
{
    Integer private_key;
    Integer public_key;
    Integer p, g;
    Integer shared_secret;
    int k = 128;
    vector<uint32_t> S;

public:
    Party();
    Party(Integer P, Integer G);

    Integer getP();
    Integer getG();
    Integer sendPublicKey();
    void create_shared_secret(Integer key_recived);
    vector<uint32_t> encrypt_data(vector<uint32_t> data);
    vector<uint32_t> decrypt_data(vector<uint32_t> data);

private:
    void createKeys();
    void getKeyFromSecret();
};
void print_data(vector<uint32_t> data);

// Tests
void test_simetrie(const vector<uint32_t> &S);
void test_avalansa(const vector<uint32_t> &S);
void test_performanta(const vector<uint32_t> &S);
void test_vectori_oficiali();
void test_data_received();

#endif