#include "blockchain.h"
#include "hash.h"
#include <fstream>

using std::vector;
using std::string;
using std::to_string;

vector<User> generateUsers(int count, std::mt19937& rng) {
    vector<User> users;
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

string serializeTransaction(const Transaction& tx) {
    return tx.sender + "|" + tx.receiver + "|" +
           to_string(tx.amount) + "|" + to_string(tx.sequence);
}

string computeTransactionId(const Transaction& tx) {
    return hashFunction(serializeTransaction(tx));
}

vector<Transaction> generateTransactions(int count, const vector<User>& users, std::mt19937& rng) {
    vector<Transaction> transactions;
    std::uniform_int_distribution<int> userDist(0, (int)users.size() - 1);
    std::uniform_int_distribution<uint64_t> amountDist(1, 100000);

    for (int i = 0; i < count; i++) {
        int s = userDist(rng);
        int r = userDist(rng);
        while (r == s) {
            r = userDist(rng);
        }

        Transaction tx;
        tx.sender = users[s].public_key;
        tx.receiver = users[r].public_key;
        tx.amount = amountDist(rng);
        tx.sequence = i + 1;
        tx.id = computeTransactionId(tx);
        transactions.push_back(tx);
    }
    return transactions;
}

string serializeHeader(const BlockHeader& h) {
    return h.prev_hash + "|" + to_string(h.timestamp) + "|" + to_string(h.version) + "|" +
           h.tx_root + "|" + to_string(h.nonce) + "|" + to_string(h.difficulty);
}

string computeBlockHash(const BlockHeader& h) {
    return hashFunction(serializeHeader(h));
}

string computeTxRoot(const vector<Transaction>& txs) {
    string joined;
    for (const Transaction& tx : txs) {
        joined += tx.id;
    }
    return hashFunction(joined);
}

bool meetsDifficulty(const string& hash, int difficulty) {
    for (int i = 0; i < difficulty; i++) {
        if (hash[i] != '0') {
            return false;
        }
    }
    return true;
}

uint64_t mineBlock(Block& block) {
    uint64_t attempts = 0;
    block.header.nonce = 0;

    while (true) {
        string hash = computeBlockHash(block.header);
        attempts++;
        if (meetsDifficulty(hash, block.header.difficulty)) {
            block.hash = hash;
            return attempts;
        }
        block.header.nonce++;
    }
}

Block createGenesisBlock() {
    Block genesis;
    genesis.header.prev_hash = string(64, '0');
    genesis.header.timestamp = 0;
    genesis.header.version = 1;
    genesis.header.tx_root = hashFunction("");
    genesis.header.nonce = 0;
    genesis.header.difficulty = 0;
    genesis.hash = computeBlockHash(genesis.header);
    return genesis;
}

Balances createBalances(const vector<User>& users) {
    Balances balances;
    for (const User& u : users) {
        balances[u.public_key] = u.balance;
    }
    return balances;
}

uint64_t totalBalance(const Balances& balances) {
    uint64_t sum = 0;
    for (const auto& entry : balances) {
        sum += entry.second;
    }
    return sum;
}

void saveInitialState(const vector<User>& users, const string& filename) {
    std::ofstream file(filename);
    file << "name,public_key,balance\n";
    for (const User& u : users) {
        file << u.name << "," << u.public_key << "," << u.balance << "\n";
    }
}

bool isValidForPool(const Transaction& tx, const Balances& balances) {
    if (balances.count(tx.sender) == 0 || balances.count(tx.receiver) == 0) return false;
    if (tx.sender == tx.receiver) return false;
    if (tx.amount == 0) return false;
    if (tx.id != computeTransactionId(tx)) return false;
    return true;
}

bool applyTransaction(Balances& balances, const Transaction& tx) {
    if (balances[tx.sender] < tx.amount) {
        return false;
    }
    balances[tx.sender] -= tx.amount;
    balances[tx.receiver] += tx.amount;
    return true;
}

bool validateBlock(const Block& block, const string& expectedPrevHash,
                   int expectedDifficulty, const Balances& confirmed, Balances& newState) {
    if (block.header.prev_hash != expectedPrevHash) return false;
    if (block.header.difficulty != expectedDifficulty) return false;
    if (block.header.tx_root != computeTxRoot(block.transactions)) return false;

    string hash = computeBlockHash(block.header);
    if (hash != block.hash || !meetsDifficulty(hash, expectedDifficulty)) return false;

    newState = confirmed;
    for (const Transaction& tx : block.transactions) {
        if (!isValidForPool(tx, newState) || !applyTransaction(newState, tx)) {
            return false;
        }
    }
    return true;
}