#include "blockchain.h"
#include "hash.h"

std::vector<User> generateUsers(int count, std::mt19937& rng) {
    std::vector<User> users;
    std::uniform_int_distribution<uint64_t> dist(100, 1000000);

    for (int i = 0; i < count; i++) {
        User u;
        u.name = "User" + std::to_string(i + 1);
        u.public_key = hashFunction(u.name);
        u.balance = dist(rng);
        users.push_back(u);
    }
    return users;
}