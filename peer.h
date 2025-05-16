#pragma once

#include <string>

struct Peer {
    std::string ip;
    int port;

    Peer(const std::string& IP, int Port): 
        ip{IP}, 
        port{Port}
        {}
};