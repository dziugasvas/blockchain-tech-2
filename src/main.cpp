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
const int CANDIDATES = 5;
const uint64_t START_BUDGET = 5000;
const int MAX_ROUNDS = 10;
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
        uint64_t totalCandidateAttempts = 0;

    cout << "Vartotoju: " << users.size() << ", transakciju: " << generated.size()
         << ", seed: " << SEED << ", difficulty: " << DIFFICULTY << endl;
    cout << "Genesis hash: " << chain[0].hash << endl << endl;

    while (!pool.empty()) {
                vector<Block> candidates;
        for (int c = 0; c < CANDIDATES; c++) {
            candidates.push_back(buildCandidate(pool, balances, chain.back().hash, DIFFICULTY, BLOCK_SIZE, rng));
        }
        if (candidates[0].transactions.empty()) {
            break;
        }

        vector<uint64_t> attempts(CANDIDATES, 0);
        vector<bool> found(CANDIDATES, false);
        uint64_t budget = START_BUDGET;
        int winner = -1;
        int rounds = 0;

        auto start = std::chrono::steady_clock::now();
        while (winner == -1 && rounds < MAX_ROUNDS) {
            rounds++;
            for (int c = 0; c < CANDIDATES; c++) {
                found[c] = mineWithBudget(candidates[c], budget, attempts[c]);
            }
            for (int c = 0; c < CANDIDATES; c++) {
                if (found[c] && (winner == -1 || attempts[c] < attempts[winner])) {
                    winner = c;
                }
            }
            budget *= 2;
        }
        auto end = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();

        for (int c = 0; c < CANDIDATES; c++) {
            totalCandidateAttempts += attempts[c];
        }

        if (winner == -1) {
            cout << "Nei vienas kandidatas nerado nonce per " << MAX_ROUNDS << " raundu, stabdoma" << endl;
            break;
        }

        Block block = candidates[winner];

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
             << " | nonce: " << block.header.nonce << " | bandymu: " << attempts[winner]
             << " | laikas: " << ms << " ms" << endl;
        cout << "  prev_hash: " << block.header.prev_hash << endl;
        cout << "  tx_root:   " << block.header.tx_root << endl;
        cout << "  hash:      " << block.hash << " (difficulty " << block.header.difficulty << ")" << endl;
        cout << "  kandidatai:";
        for (int c = 0; c < CANDIDATES; c++) {
            cout << " #" << c + 1 << "(" << candidates[c].transactions.size() << " tx, "
                 << attempts[c] << (found[c] ? ", rado)" : ")");
        }
        cout << " -> laimejo #" << winner + 1 << ", raundu: " << rounds << endl;
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
    cout << "Kandidatu bandymu is viso: " << totalCandidateAttempts << endl;
    cout << "Bendras laikas: " << totalMs << " ms" << endl;

    return 0;
}