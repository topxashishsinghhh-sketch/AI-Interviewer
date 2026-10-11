// OOP: Utility (Cryptographic Hash)
#include "core/SHA256.h"
#include <cstring>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstdint>
#include <array>

namespace core {

// Static constexpr member definition (required for ODR-use)
constexpr std::array<uint32_t, 64> SHA256::K;

static inline uint32_t rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

static inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

static inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

static inline uint32_t sigma0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

static inline uint32_t sigma1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

static inline uint32_t gamma0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

static inline uint32_t gamma1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

void SHA256::transform(std::array<uint32_t, 8>& state, const uint8_t* data) {
    std::array<uint32_t, 64> w;
    
    // Prepare message schedule
    for (int i = 0; i < 16; ++i) {
        w[i] = (static_cast<uint32_t>(data[i * 4]) << 24) |
               (static_cast<uint32_t>(data[i * 4 + 1]) << 16) |
               (static_cast<uint32_t>(data[i * 4 + 2]) << 8) |
               (static_cast<uint32_t>(data[i * 4 + 3]));
    }
    
    for (int i = 16; i < 64; ++i) {
        w[i] = gamma1(w[i - 2]) + w[i - 7] + gamma0(w[i - 15]) + w[i - 16];
    }
    
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
    
    for (int i = 0; i < 64; ++i) {
        uint32_t t1 = h + sigma1(e) + ch(e, f, g) + K[i] + w[i];
        uint32_t t2 = sigma0(a) + maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

std::string SHA256::hash(const std::string& input) {
    // Initial hash values (first 32 bits of fractional parts of square roots of first 8 primes)
    std::array<uint32_t, 8> state = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    
    // Pre-processing: padding
    size_t originalLen = input.size();
    uint64_t bitLen = static_cast<uint64_t>(originalLen) * 8;
    
    // Pad with 1 bit then zeros until length ≡ 448 (mod 512)
    size_t paddedLen = ((originalLen + 9 + 63) / 64) * 64;
    std::vector<uint8_t> padded(paddedLen, 0);
    std::memcpy(padded.data(), input.data(), originalLen);
    padded[originalLen] = 0x80;
    
    // Append length in bits as 64-bit big-endian
    for (int shift = 56, index = static_cast<int>(paddedLen) - 8; shift >= 0; shift -= 8, ++index) {
        padded[static_cast<size_t>(index)] =
            static_cast<uint8_t>((bitLen >> static_cast<unsigned>(shift)) & 0xFFu);
    }
    
    // Process each 512-bit chunk
    for (size_t i = 0; i < paddedLen; i += 64) {
        transform(state, padded.data() + i);
    }
    
    // Output as binary string
    std::string result(32, 0);
    for (int i = 0; i < 8; ++i) {
        for (int byteIndex = 0; byteIndex < 4; ++byteIndex) {
            const unsigned shift = static_cast<unsigned>(24 - byteIndex * 8);
            result[static_cast<size_t>(i * 4 + byteIndex)] =
                static_cast<char>((state[static_cast<size_t>(i)] >> shift) & 0xFFu);
        }
    }
    return result;
}

std::string SHA256::hashHex(const std::string& input) {
    std::string binary = hash(input);
    std::stringstream ss;
    for (unsigned char c : binary) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }
    return ss.str();
}

} // namespace core