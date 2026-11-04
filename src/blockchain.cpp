#include "blockchain.h"
#include "hash.h"
#include <fstream>
#include <algorithm>
#include <ctime>

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

string hexToBytes(const string& hex) {
    string bytes;
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        bytes += (char)std::stoi(hex.substr(i, 2), nullptr, 16);
    }
    return bytes;
}

string merkleRoot(vector<string> level) {
    if (level.empty()) {
        return hashFunction("");
    }

    while (level.size() > 1) {
        if (level.size() % 2 == 1) {
            level.push_back(level.back());
        }

        vector<string> next;
        for (size_t i = 0; i < level.size(); i += 2) {
            next.push_back(hashFunction(hexToBytes(level[i]) + hexToBytes(level[i + 1])));
        }
        level = next;
    }
    return level[0];
}

string computeTxRoot(const vector<Transaction>& txs) {
    vector<string> ids;
    for (const Transaction& tx : txs) {
        ids.push_back(tx.id);
    }
    return merkleRoot(ids);
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

bool mineWithBudget(Block& block, uint64_t budget, uint64_t& attempts) {
    const BlockHeader& h = block.header;
    string prefix = h.prev_hash + "|" + to_string(h.timestamp) + "|" + to_string(h.version) + "|" + h.tx_root + "|";
    string suffix = "|" + to_string(h.difficulty);

    for (uint64_t i = 0; i < budget; i++) {
        string hash = hashFunction(prefix + to_string(block.header.nonce) + suffix);
        attempts++;
        if (meetsDifficulty(hash, block.header.difficulty)) {
            block.hash = hash;
            return true;
        }
        block.header.nonce++;
    }
    return false;
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

string checkTransaction(const Transaction& tx, const Balances& balances) {
    if (balances.count(tx.sender) == 0) return "nezinomas siuntejas";
    if (balances.count(tx.receiver) == 0) return "nezinomas gavejas";
    if (tx.sender == tx.receiver) return "siuntejas ir gavejas sutampa";
    if (tx.amount == 0) return "suma turi buti teigiama";
    if (tx.id != computeTransactionId(tx)) return "neteisingas transakcijos ID";
    return "";
}

string applyTransaction(Balances& balances, const Transaction& tx) {
    uint64_t senderBalance = balances.at(tx.sender);
    uint64_t receiverBalance = balances.at(tx.receiver);

    if (senderBalance < tx.amount) return "nepakankamas balansas";
    if (receiverBalance > UINT64_MAX - tx.amount) return "balanso perpildymas";

    balances[tx.sender] = senderBalance - tx.amount;
    balances[tx.receiver] = receiverBalance + tx.amount;
    return "";
}

string validateBlock(const Block& block, const string& expectedPrevHash, int expectedDifficulty,
                     const Balances& confirmed, const std::unordered_set<string>& confirmedIds,
                     Balances& newState) {
    if (block.header.version != BLOCK_VERSION) return "nepalaikoma versija";
    if (block.header.prev_hash != expectedPrevHash) return "neteisingas prev_hash";
    if (block.header.difficulty != expectedDifficulty) return "neteisingas sudetingumas";
    if (block.transactions.size() > MAX_BLOCK_TX) return "per daug transakciju";
    if (block.header.tx_root != computeTxRoot(block.transactions)) return "neteisingas tx_root";

    string hash = computeBlockHash(block.header);
    if (hash != block.hash) return "neteisinga bloko maisa";
    if (!meetsDifficulty(hash, expectedDifficulty)) return "netinkamas PoW";

    newState = confirmed;
    std::unordered_set<string> blockIds;
    for (const Transaction& tx : block.transactions) {
        if (confirmedIds.count(tx.id) > 0) return "transakcija jau patvirtinta";
        if (blockIds.count(tx.id) > 0) return "transakcija bloke kartojasi";
        blockIds.insert(tx.id);

        string reason = checkTransaction(tx, newState);
        if (reason.empty()) reason = applyTransaction(newState, tx);
        if (!reason.empty()) return reason;
    }
    return "";
}

Block buildCandidate(vector<Transaction> pool, const Balances& confirmed, const string& prevHash,
                     int difficulty, size_t blockSize, std::mt19937& rng) {
    std::shuffle(pool.begin(), pool.end(), rng);

    Block block;
    Balances temp = confirmed;
    for (const Transaction& tx : pool) {
        if (block.transactions.size() == blockSize) break;
        if (applyTransaction(temp, tx).empty()) {
            block.transactions.push_back(tx);
        }
    }

    block.header.prev_hash = prevHash;
    block.header.timestamp = (uint64_t)std::time(nullptr);
    block.header.version = BLOCK_VERSION;
    block.header.difficulty = difficulty;
    block.header.tx_root = computeTxRoot(block.transactions);
    block.header.nonce = 0;
    return block;
}