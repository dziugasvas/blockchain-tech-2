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

    return 0;
}