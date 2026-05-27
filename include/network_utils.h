#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

#include <string>
#include <cryptopp/integer.h>

std::string integer_to_string(const CryptoPP::Integer& num);

#endif // NETWORK_UTILS_H