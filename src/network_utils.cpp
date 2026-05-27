#include "../include/network_utils.h"
#include <sstream>

std::string integer_to_string(const CryptoPP::Integer& num) {
    std::stringstream ss;
    ss << num;
    return ss.str();
}