#include "hash.h"
#include "blockchain.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <unordered_set>

using std::cout;
using std::endl;
using std::string;
using std::vector;

const int DIFFICULTY = 3;
const unsigned SEED = 42;

int main(int argc, char* argv[]) {
    bool small = (argc > 1 && string(argv[1]) == "small");
    int USER_COUNT = small ? 5 : 1000;
    int TX_COUNT = small ? 20 : 10000;
    int BLOCK_SIZE = small ? 5 : 100;

    auto programStart = std::chrono::steady_clock::now();
    std::mt19937 rng(SEED);

    vector<User> users = generateUsers(USER_COUNT, rng);
    saveInitialState(users, "initial_state.csv");
    Balances balances = createBalances(users);
    uint64_t initialSum = totalBalance(balances);

    vector<Transaction> generated = generateTransactions(TX_COUNT, users, rng);
    vector<Transaction> pool;
    std::unordered_set<string> knownIds;
    std::unordered_set<string> confirmedIds;
    int rejectedAtPool = 0;
    for (const Transaction& tx : generated) {
        string reason = checkTransaction(tx, balances);
        if (reason.empty() && knownIds.count(tx.id) > 0) {
            reason = "dubliuotas ID";
        }
        if (!reason.empty()) {
            cout << "Atmesta tx #" << tx.sequence << ": " << reason << endl;
            rejectedAtPool++;
            continue;
        }
        knownIds.insert(tx.id);
        pool.push_back(tx);
    }

    vector<Block> chain;
    chain.push_back(createGenesisBlock());
    int includedCount = 0;

    cout << "Vartotoju: " << users.size() << ", transakciju: " << generated.size()
         << ", seed: " << SEED << ", difficulty: " << DIFFICULTY << endl;
    cout << "Genesis hash: " << chain[0].hash << endl << endl;

    while (!pool.empty()) {
        std::shuffle(pool.begin(), pool.end(), rng);

        Block block;
        Balances temp = balances;
        for (const Transaction& tx : pool) {
            if ((int)block.transactions.size() == BLOCK_SIZE) break;
            if (applyTransaction(temp, tx).empty()) {
                block.transactions.push_back(tx);
            }
        }

        if (block.transactions.empty()) {
            break;
        }

        block.header.prev_hash = chain.back().hash;
        block.header.timestamp = (uint64_t)std::time(nullptr);
        block.header.version = BLOCK_VERSION;
        block.header.difficulty = DIFFICULTY;
        block.header.tx_root = computeTxRoot(block.transactions);

        auto start = std::chrono::steady_clock::now();
        uint64_t attempts = mineBlock(block);
        auto end = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();

        Balances newState;
        string reason = validateBlock(block, chain.back().hash, DIFFICULTY, balances, confirmedIds, newState);
        if (!reason.empty()) {
            cout << "Blokas atmestas: " << reason << endl;
            break;
        }

        balances = newState;
        chain.push_back(block);
        includedCount += block.transactions.size();
        for (const Transaction& tx : block.transactions) {
            confirmedIds.insert(tx.id);
        }

        vector<Transaction> remaining;
        for (const Transaction& tx : pool) {
            if (confirmedIds.count(tx.id) == 0) {
                remaining.push_back(tx);
            }
        }
        pool = remaining;

        cout << "Blokas #" << chain.size() - 1 << " | tx: " << block.transactions.size()
             << " | nonce: " << block.header.nonce << " | bandymu: " << attempts
             << " | laikas: " << ms << " ms" << endl;
        cout << "  prev_hash: " << block.header.prev_hash << endl;
        cout << "  tx_root:   " << block.header.tx_root << endl;
        cout << "  hash:      " << block.hash << " (difficulty " << block.header.difficulty << ")" << endl;
    }

    if (small) {
        cout << endl << "Balansai (pradinis -> galutinis):" << endl;
        for (const User& u : users) {
            cout << "  " << u.name << ": " << u.balance << " -> " << balances[u.public_key] << endl;
        }
    }

    auto programEnd = std::chrono::steady_clock::now();
    double totalMs = std::chrono::duration<double, std::milli>(programEnd - programStart).count();

    cout << endl << "===== SUVESTINE =====" << endl;
    cout << "Vartotoju: " << users.size() << endl;
    cout << "Sugeneruota transakciju: " << generated.size() << endl;
    cout << "Priimta bloku (be genesis): " << chain.size() - 1 << endl;
    cout << "Itraukta transakciju: " << includedCount << endl;
    cout << "Atmesta pries pool: " << rejectedAtPool << endl;
    cout << "Liko pool (nepakankamas balansas): " << pool.size() << endl;
    cout << "Pradine lesu suma: " << initialSum << endl;
    cout << "Galutine lesu suma: " << totalBalance(balances) << endl;
    cout << "Bendras laikas: " << totalMs << " ms" << endl;

    return 0;
}