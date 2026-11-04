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

std::string serializeTransaction(const Transaction& tx) {
    return tx.sender + "|" + tx.receiver + "|" +
           std::to_string(tx.amount) + "|" + std::to_string(tx.sequence);
}

std::string computeTransactionId(const Transaction& tx) {
    return hashFunction(serializeTransaction(tx));
}

std::vector<Transaction> generateTransactions(int count, const std::vector<User>& users, std::mt19937& rng) {
    std::vector<Transaction> transactions;
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