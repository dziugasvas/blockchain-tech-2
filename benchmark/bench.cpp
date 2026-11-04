#include "blockchain.h"
#include "hash.h"
#include <iostream>
#include <chrono>

using std::cout;
using std::endl;

const uint64_t HASHES = 200000;
const int RUNS = 5;

double measureOnce() {
    Block block;
    block.header.prev_hash = std::string(64, '0');
    block.header.timestamp = 1700000000;
    block.header.version = 1;
    block.header.tx_root = hashFunction("bench");
    block.header.nonce = 0;
    block.header.difficulty = 64;

    uint64_t attempts = 0;
    auto start = std::chrono::steady_clock::now();
    mineWithBudget(block, HASHES, attempts);
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

int main() {
    measureOnce();

    double sum = 0, minMs = 1e18, maxMs = 0;
    for (int i = 0; i < RUNS; i++) {
        double ms = measureOnce();
        cout << "Matavimas " << i + 1 << ": " << ms << " ms" << endl;
        sum += ms;
        if (ms < minMs) minMs = ms;
        if (ms > maxMs) maxMs = ms;
    }

    double avg = sum / RUNS;
    cout << endl << HASHES << " bandymu, " << RUNS << " matavimai" << endl;
    cout << "Vidurkis: " << avg << " ms (min " << minMs << ", max " << maxMs << ")" << endl;
    cout << "Greitis: " << (HASHES / (avg / 1000.0)) << " bandymu/s" << endl;
    return 0;
}