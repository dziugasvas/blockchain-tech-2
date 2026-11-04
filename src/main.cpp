#include "hash.h"
#include "blockchain.h"
#include <iostream>

using std::cout;
using std::endl;

int main() {
    std::mt19937 rng(42);   // fiksuotas seed, kad rezultatai kartotusi

    std::vector<User> users = generateUsers(1000, rng);

    cout << "Sugeneruota vartotoju: " << users.size() << endl;
    cout << "Pirmi 5 vartotojai:" << endl;
    for (int i = 0; i < 5; i++) {
        cout << "  " << users[i].name
             << " | " << users[i].public_key
             << " | balansas: " << users[i].balance << endl;
    }

    uint64_t suma = 0;
    for (const User& u : users) {
        suma += u.balance;
    }
    cout << "Bendra visu balansu suma: " << suma << endl;

    std::vector<Transaction> transactions = generateTransactions(10000, users, rng);
    cout << endl << "Sugeneruota transakciju: " << transactions.size() << endl;
    cout << "Pirmos 3 transakcijos:" << endl;
    for (int i = 0; i < 3; i++) {
        const Transaction& tx = transactions[i];
        cout << "  #" << tx.sequence << ": " << tx.sender.substr(0, 8) << "... -> "
             << tx.receiver.substr(0, 8) << "... | suma: " << tx.amount
             << " | id: " << tx.id << endl;
    }

    return 0;
}