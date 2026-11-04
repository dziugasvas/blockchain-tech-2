#include "blockchain.h"
#include "hash.h"
#include <iostream>

using std::cout;
using std::endl;
using std::string;
using std::vector;

int passed = 0;
int failed = 0;

void check(const string& name, bool ok) {
    cout << (ok ? "[PASS] " : "[FAIL] ") << name << endl;
    if (ok) passed++;
    else failed++;
}

Transaction makeTx(const string& from, const string& to, uint64_t amount, uint64_t seq) {
    Transaction tx;
    tx.sender = from;
    tx.receiver = to;
    tx.amount = amount;
    tx.sequence = seq;
    tx.id = computeTransactionId(tx);
    return tx;
}

Block makeBlock(const string& prevHash, const vector<Transaction>& txs) {
    Block block;
    block.transactions = txs;
    block.header.prev_hash = prevHash;
    block.header.timestamp = 1700000000;
    block.header.version = BLOCK_VERSION;
    block.header.difficulty = 3;
    block.header.tx_root = computeTxRoot(txs);
    mineBlock(block);
    return block;
}

Balances startBalances() {
    Balances b;
    b["A"] = 100;
    b["B"] = 50;
    b["C"] = 0;
    return b;
}

void testValidPayment() {
    Balances confirmed = startBalances();
    std::unordered_set<string> confirmedIds;
    string prev = createGenesisBlock().hash;
    Block block = makeBlock(prev, {makeTx("A", "B", 30, 1)});

    Balances newState;
    string reason = validateBlock(block, prev, 3, confirmed, confirmedIds, newState);
    check("Galiojantis mokejimas priimamas", reason.empty());
    check("Balansai teisingi (A=70, B=80)", newState["A"] == 70 && newState["B"] == 80);
    check("Lesu suma nepasikeite", totalBalance(newState) == totalBalance(confirmed));
    check("Blokas su <100 transakciju veikia", block.transactions.size() < MAX_BLOCK_TX);
}

void testInvalidTransactions() {
    Balances b = startBalances();
    check("Nezinomas gavejas atmetamas",
          checkTransaction(makeTx("A", "X", 10, 1), b) == "nezinomas gavejas");
    check("Suma 0 atmetama",
          checkTransaction(makeTx("A", "B", 0, 2), b) == "suma turi buti teigiama");

    Transaction tampered = makeTx("A", "B", 10, 3);
    tampered.amount = 99;
    check("Pakeistas ID (pakeista suma) atmetamas",
          checkTransaction(tampered, b) == "neteisingas transakcijos ID");
}

void testDoubleSpend() {
    Balances confirmed = startBalances();
    Transaction t1 = makeTx("A", "B", 70, 1);
    Transaction t2 = makeTx("A", "C", 60, 2);

    Balances temp = confirmed;
    bool firstOk = applyTransaction(temp, t1).empty();
    bool secondOk = applyTransaction(temp, t2).empty();
    check("Is dvieju mokejimu (70 + 60 > 100) priimamas tik pirmas", firstOk && !secondOk);
    check("Kandidato tikrinimas nekeicia tikru balansu", confirmed["A"] == 100);

    string prev = createGenesisBlock().hash;
    Block block = makeBlock(prev, {t1, t2});
    Balances newState;
    std::unordered_set<string> confirmedIds;
    check("Blokas su abiem mokejimais atmetamas",
          validateBlock(block, prev, 3, confirmed, confirmedIds, newState) == "nepakankamas balansas");
}

void testDuplicates() {
    Balances confirmed = startBalances();
    string prev = createGenesisBlock().hash;
    Transaction t = makeTx("A", "B", 10, 1);
    Balances newState;

    std::unordered_set<string> noIds;
    Block dup = makeBlock(prev, {t, t});
    check("Dubliuotas ID bloke atmetamas",
          validateBlock(dup, prev, 3, confirmed, noIds, newState) == "transakcija bloke kartojasi");

    std::unordered_set<string> confirmedIds = {t.id};
    Block replay = makeBlock(prev, {t});
    check("Jau patvirtintas mokejimas neiskaitomas antra karta",
          validateBlock(replay, prev, 3, confirmed, confirmedIds, newState) == "transakcija jau patvirtinta");
}

void testTamperedBlocks() {
    Balances confirmed = startBalances();
    std::unordered_set<string> confirmedIds;
    string prev = createGenesisBlock().hash;
    Balances newState;

    Block changed = makeBlock(prev, {makeTx("A", "B", 10, 1)});
    changed.transactions[0] = makeTx("A", "B", 90, 1);
    check("Pakeistas bloko turinys atmetamas",
          validateBlock(changed, prev, 3, confirmed, confirmedIds, newState) == "neteisingas tx_root");

    Block wrongPrev = makeBlock(string(64, 'f'), {makeTx("A", "B", 10, 1)});
    check("Neteisingas prev_hash atmetamas",
          validateBlock(wrongPrev, prev, 3, confirmed, confirmedIds, newState) == "neteisingas prev_hash");

    Block badPow = makeBlock(prev, {makeTx("A", "B", 10, 1)});
    badPow.header.nonce++;
    badPow.hash = computeBlockHash(badPow.header);
    check("Netinkamas PoW atmetamas",
          validateBlock(badPow, prev, 3, confirmed, confirmedIds, newState) == "netinkamas PoW");

    Block lowDifficulty = makeBlock(prev, {makeTx("A", "B", 10, 1)});
    lowDifficulty.header.difficulty = 1;
    lowDifficulty.hash = computeBlockHash(lowDifficulty.header);
    check("Blokas negali pats sumazinti sudetingumo",
          validateBlock(lowDifficulty, prev, 3, confirmed, confirmedIds, newState) == "neteisingas sudetingumas");

    check("Po atmestu bloku patvirtinta busena nepasikeite",
          confirmed["A"] == 100 && confirmed["B"] == 50 && confirmed["C"] == 0);
}

void testOverflow() {
    Balances b;
    b["A"] = 100;
    b["B"] = UINT64_MAX - 5;
    check("Balanso perpildymas atmetamas",
          applyTransaction(b, makeTx("A", "B", 10, 1)) == "balanso perpildymas");
}

void testMerkle() {
    string a = hashFunction("a"), b = hashFunction("b"), c = hashFunction("c");
    string d = hashFunction("d"), e = hashFunction("e");
    auto node = [](const string& left, const string& right) {
        return hashFunction(hexToBytes(left) + hexToBytes(right));
    };

    check("Merkle: 0 lapu -> H(\"\")", merkleRoot({}) == hashFunction(""));
    check("Merkle: 1 lapas -> pats lapas", merkleRoot({a}) == a);
    check("Merkle: 2 lapai -> H(a||b)", merkleRoot({a, b}) == node(a, b));
    check("Merkle: 3 lapai -> c kopijuojamas",
          merkleRoot({a, b, c}) == node(node(a, b), node(c, c)));

    string ab = node(a, b), cd = node(c, d), ee = node(e, e);
    string expected5 = node(node(ab, cd), node(ee, ee));
    check("Merkle: 5 lapai", merkleRoot({a, b, c, d, e}) == expected5);

    check("Merkle: pakeitus lapa saknis pasikeicia", merkleRoot({a, b, c}) != merkleRoot({a, b, d}));
    check("Merkle: sukeitus tvarka saknis pasikeicia", merkleRoot({a, b, c}) != merkleRoot({b, a, c}));
}

void testCandidates() {
    Balances confirmed = startBalances();
    vector<Transaction> pool = {makeTx("A", "B", 70, 1), makeTx("A", "C", 60, 2), makeTx("B", "C", 20, 3)};
    std::mt19937 rng(1);
    string prev = createGenesisBlock().hash;

    bool allValid = true;
    for (int i = 0; i < 5; i++) {
        Block candidate = buildCandidate(pool, confirmed, prev, 3, MAX_BLOCK_TX, rng);
        mineBlock(candidate);
        Balances newState;
        std::unordered_set<string> noIds;
        if (!validateBlock(candidate, prev, 3, confirmed, noIds, newState).empty()) allValid = false;
    }
    check("Visi 5 kandidatai atskirai teisingi", allValid);
    check("Kandidatu kurimas nekeicia patvirtintu balansu",
          confirmed["A"] == 100 && confirmed["B"] == 50 && confirmed["C"] == 0);
    check("Kandidatu kurimas nekeicia pool", pool.size() == 3);
}

void testDeterminism() {
    check("Ta pati transakcija -> tas pats ID", makeTx("A", "B", 5, 1).id == makeTx("A", "B", 5, 1).id);
    check("Fiksuota antraste -> ta pati bloko maisa",
          createGenesisBlock().hash == createGenesisBlock().hash);
}

void testMoreTransactionCases() {
    Balances b = startBalances();
    check("Nezinomas siuntejas atmetamas",
          checkTransaction(makeTx("X", "B", 10, 1), b) == "nezinomas siuntejas");
    check("Siuntejas ir gavejas negali sutapti",
          checkTransaction(makeTx("A", "A", 10, 2), b) == "siuntejas ir gavejas sutampa");

    vector<Transaction> pool = {makeTx("C", "A", 10, 3)};
    std::mt19937 rng(1);
    Block candidate = buildCandidate(pool, b, createGenesisBlock().hash, 3, MAX_BLOCK_TX, rng);
    check("Jei niekas negali sumoketi, kandidatas tuscias", candidate.transactions.empty());
}

void testMoreBlockCases() {
    Balances confirmed = startBalances();
    std::unordered_set<string> confirmedIds;
    string prev = createGenesisBlock().hash;
    Balances newState;

    Block wrongVersion = makeBlock(prev, {makeTx("A", "B", 10, 1)});
    wrongVersion.header.version = 2;
    mineBlock(wrongVersion);
    check("Nepalaikoma versija atmetama",
          validateBlock(wrongVersion, prev, 3, confirmed, confirmedIds, newState) == "nepalaikoma versija");

    vector<Transaction> many;
    for (uint64_t i = 1; i <= MAX_BLOCK_TX + 1; i++) {
        many.push_back(makeTx("A", "B", 1, i));
    }
    Block tooBig = makeBlock(prev, many);
    check("Blokas su daugiau nei 100 transakciju atmetamas",
          validateBlock(tooBig, prev, 3, confirmed, confirmedIds, newState) == "per daug transakciju");

    Block wrongHash = makeBlock(prev, {makeTx("A", "B", 10, 1)});
    wrongHash.hash = string(64, '0');
    check("Pakeista bloko maisa atmetama",
          validateBlock(wrongHash, prev, 3, confirmed, confirmedIds, newState) == "neteisinga bloko maisa");
}

void testMining() {
    Block block = makeBlock(createGenesisBlock().hash, {makeTx("A", "B", 10, 1)});

    Block oneStep = block;
    oneStep.header.nonce = 0;
    uint64_t attemptsOne = 0;
    mineWithBudget(oneStep, 1000000, attemptsOne);
    check("mineWithBudget maisa sutampa su computeBlockHash", oneStep.hash == computeBlockHash(oneStep.header));

    Block twoSteps = block;
    twoSteps.header.nonce = 0;
    uint64_t attemptsTwo = 0;
    uint64_t half = attemptsOne / 2;
    bool foundEarly = mineWithBudget(twoSteps, half, attemptsTwo);
    bool foundLater = mineWithBudget(twoSteps, 1000000, attemptsTwo);
    check("Kasimas tesiamas nuo neisbandyto nonce",
          !foundEarly && foundLater && twoSteps.header.nonce == oneStep.header.nonce && attemptsTwo == attemptsOne);
}

int main() {
    testValidPayment();
    testInvalidTransactions();
    testDoubleSpend();
    testDuplicates();
    testTamperedBlocks();
    testOverflow();
    testMerkle();
    testCandidates();
    testDeterminism();
    testMoreTransactionCases();
    testMoreBlockCases();
    testMining();

    cout << endl << "Is viso: " << passed << " PASS, " << failed << " FAIL" << endl;
    return failed == 0 ? 0 : 1;
}