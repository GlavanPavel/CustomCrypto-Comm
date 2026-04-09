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
void decrypt_block(uint32_t registers[4], const std::vector<uint32_t> &S);
void encrypt_block(uint32_t registers[4], const std::vector<uint32_t> &S);
std::vector<uint32_t> keySchedule(std::vector<uint8_t> &key);
void print_block(std::uint32_t b[4]);
#endif