#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <string>
#include <vector>
#include <cstdint>
#include <random>
#include <unordered_map>

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

struct BlockHeader {
    std::string prev_hash;
    uint64_t timestamp;
    uint32_t version;
    std::string tx_root;
    uint64_t nonce;
    int difficulty;
};

struct Block {
    BlockHeader header;
    std::vector<Transaction> transactions;
    std::string hash;
};

std::string serializeHeader(const BlockHeader& h);
std::string computeBlockHash(const BlockHeader& h);
std::string hexToBytes(const std::string& hex);
std::string merkleRoot(std::vector<std::string> level);
std::string computeTxRoot(const std::vector<Transaction>& txs);
bool meetsDifficulty(const std::string& hash, int difficulty);
uint64_t mineBlock(Block& block);
Block createGenesisBlock();

typedef std::unordered_map<std::string, uint64_t> Balances;

Balances createBalances(const std::vector<User>& users);
uint64_t totalBalance(const Balances& balances);
void saveInitialState(const std::vector<User>& users, const std::string& filename);

bool isValidForPool(const Transaction& tx, const Balances& balances);
bool applyTransaction(Balances& balances, const Transaction& tx);
bool validateBlock(const Block& block, const std::string& expectedPrevHash,
                   int expectedDifficulty, const Balances& confirmed, Balances& newState);

#endif