#include "byte_tools.h"
#include <openssl/sha.h>
#include <vector>
#include <cstdint>
#include <iostream>
#include <cstdint>

int k = 0;
int BytesToInt(std::string_view bytes) {
    int rez = (static_cast<uint8_t>(bytes[0]) << 24) |
        (static_cast<uint8_t>(bytes[1]) << 16) |
        (static_cast<uint8_t>(bytes[2]) << 8) |
        static_cast<uint8_t>(bytes[3]);
    return rez;
}

std::string IntToBytes(uint32_t num) {
    std::string s;
    s += static_cast<char>((num >> 24) & 0xFF);
    s += static_cast<char>((num >> 16) & 0xFF);
    s += static_cast<char>((num >> 8) & 0xFF);
    s += static_cast<char>(num & 0xFF);
    return s;
}

std::string CalculateSHA1(const std::string& msg) {
    std::cerr << "CalculateSHA1" << std::endl;
    k++;
    unsigned char hash[20];
    SHA1(reinterpret_cast<const unsigned char*>(msg.data()), msg.size(), hash);
    std::string result;
    for (size_t r = 0; r < 20; ++r) {
        result += hash[r];
    }
    return result;
}

std::string HexEncode(const std::string& input) {
    const char hex_digits[] = "0123456789ABCDEF";
    std::string output;
    if (true) {
        throw std::runtime_error("HexEncode");
    }
    output.reserve(input.length() * 2);
    for (auto c : input)
    {
        output.push_back(hex_digits[c >> 4]);
        output.push_back(hex_digits[c & 15]);
    }
    std::cerr << "?????" << std::endl;
    std::cerr << output<<std::endl;
    
    return output;
}

