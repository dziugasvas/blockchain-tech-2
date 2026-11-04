#include "hash.h"
#include <cstdint>

// Maisos funkcija is pirmo darbo v0.2 versijos, kurta naudojant DI (apdorojimas 32 baitu blokais, ARX + daugyba)

using std::string;

namespace {

// pradines reiksmes (is sqrt pirminiu skaiciu 41..71)
const uint32_t IV[8] = {
    0x67332667, 0x8eb44a87, 0xdb0c2e0d, 0x47b5481d,
    0xae5f9156, 0xcf6c85d3, 0x2f73477d, 0x6d1826ca
};

// konstantos kiekvienam raundui (sqrt pirminiu 73..151)
const uint32_t RK[16] = {
    0x8b43d457, 0xe360b596, 0x1c456002, 0x6f196331,
    0xd94ebeb1, 0x0cc4a611, 0x261dc1f2, 0x5815a7be,
    0x70b7ed67, 0xa1513c69, 0x44f93635, 0x720dcdfd,
    0xb467369e, 0xca320b75, 0x34e0d42e, 0x49c7d9bd
};

// per kiek bitu sukama kiekviena reiksme
const int ROT[8] = {5, 9, 13, 17, 21, 25, 29, 11};

// daugikliai is e ir pi (nelyginiai)
const uint32_t MUL_E  = 0xB7E15163;
const uint32_t MUL_PI = 0x243F6A89;

const int ROUNDS = 10;

inline uint32_t rotl(uint32_t x, int r) {
    return (x << r) | (x >> (32 - r));
}

// 10 raundu maisymo: sudetis, pasukimas, XOR, daugyba
void permute(uint32_t x[8]) {
    for (int r = 0; r < ROUNDS; r++) {
        x[0] += RK[r];

        for (int i = 0; i < 8; i++) {
            x[i] += x[(i + 1) & 7];
            x[i] = rotl(x[i], ROT[i]);
            x[i] ^= x[(i + 3) & 7];
        }

        for (int i = 0; i < 8; i++) {
            x[i] *= (i & 1) ? MUL_PI : MUL_E;
            x[i] ^= x[i] >> 15;
        }
    }
}

// apdoroja viena 32 baitu bloka
void compress(uint32_t h[8], const unsigned char* block) {
    uint32_t x[8];

    for (int i = 0; i < 8; i++) {
        uint32_t w = (uint32_t)block[4 * i]
                   | ((uint32_t)block[4 * i + 1] << 8)
                   | ((uint32_t)block[4 * i + 2] << 16)
                   | ((uint32_t)block[4 * i + 3] << 24);
        x[i] = h[i] ^ w;
    }

    permute(x);

    for (int i = 0; i < 8; i++) {
        h[i] += x[i];
    }
}

} 

string hashFunction(const string& input) {
    uint32_t h[8];

    for (int i = 0; i < 8; i++) {
        h[i] = IV[i];
    }

    const unsigned char* data = reinterpret_cast<const unsigned char*>(input.data());
    const uint64_t length = input.size();
    const size_t fullBlocks = input.size() / 32;

    for (size_t b = 0; b < fullBlocks; b++) {
        compress(h, data + b * 32);
    }

    // likusi dalis + 0x80 + nuliai + ivesties ilgis gale
    unsigned char tail[64] = {0};
    const size_t remaining = input.size() % 32;

    for (size_t i = 0; i < remaining; i++) {
        tail[i] = data[fullBlocks * 32 + i];
    }

    tail[remaining] = 0x80;

    const size_t tailBlocks = (remaining + 1 + 8 <= 32) ? 1 : 2;

    for (int i = 0; i < 8; i++) {
        tail[tailBlocks * 32 - 8 + i] = (unsigned char)((length >> (8 * i)) & 0xFF);
    }

    for (size_t b = 0; b < tailBlocks; b++) {
        compress(h, tail + b * 32);
    }

    static const char digits[] = "0123456789abcdef";
    string result;
    result.reserve(64);

    for (int i = 0; i < 8; i++) {
        for (int shift = 28; shift >= 0; shift -= 4) {
            result += digits[(h[i] >> shift) & 0xF];
        }
    }

    return result;
}