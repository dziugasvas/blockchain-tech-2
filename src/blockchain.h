#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <string>
#include <vector>
#include <cstdint>
#include <random>

struct User {
    std::string name;
    std::string public_key;
    uint64_t balance;
};

std::vector<User> generateUsers(int count, std::mt19937& rng);

#endif