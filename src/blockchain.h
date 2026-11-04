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

struct Transaction {
    std::string sender;      
    std::string receiver;    
    uint64_t amount;         
    uint64_t sequence;       
    std::string id;          
};

std::string serializeTransaction(const Transaction& tx);
std::string computeTransactionId(const Transaction& tx);
std::vector<Transaction> generateTransactions(int count, const std::vector<User>& users, std::mt19937& rng);

#endif