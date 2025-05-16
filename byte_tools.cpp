#include "byte_tools.h"
#include <openssl/sha.h>
#include <vector>
#include <stdexcept>

int32_t BytesToInt(std::string_view bytes){
    if (bytes.size() > 4){
        throw std::invalid_argument("Too many bytes to cast to int");
    }
    
    int number = 0;
    for (size_t i = 0; i < bytes.size(); ++i){
        number = (number << 8) | static_cast<uint8_t>(bytes[i]);
    }
    return number;
}

std::string CalculateSHA1(const std::string& data) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), hash);
    return std::string(reinterpret_cast<char*>(hash), SHA_DIGEST_LENGTH);
}

std::string IntToBytes(uint32_t n){
    std::string str(4, '\0');
    for (int i = 3; i >= 0; --i){
        str[i] = static_cast<char>(n%256);
        n >>= 8;
    }
    return str;
}
