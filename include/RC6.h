#ifndef RC6_H
#define RC6_H

#include <vector>
#include <cstdint>
#include <string>

void decrypt_block(uint32_t registers[4], const std::vector<uint32_t> &S);
void encrypt_block(uint32_t registers[4], const std::vector<uint32_t> &S);
std::vector<uint32_t> keySchedule(std::vector<uint8_t> &key);
void print_block(std::uint32_t b[4]);

std::vector<uint32_t> byte_to_uint32_t(std::vector<uint8_t> data_byte);
std::vector<uint8_t> uint32_t_to_byte(std::vector<uint32_t> data);

#endif // RC6_H