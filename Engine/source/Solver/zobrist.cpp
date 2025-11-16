#include "zobrist.h"

// Helper class to initialize Zobrist upon start of the process.

class _initializer_helper
{
    static _initializer_helper helper;
    _initializer_helper() { init_zobrist(); }
};
_initializer_helper _initializer_helper::helper;

// extern Definitions for zobrist.h.

uint64_t Z_PIECE[2][64] = { 0ULL };

// This is Sebastiano Vigna’s mixer.
// 
// A proven and tested mixer funtion to
// generate semi random 64bit integers.

static inline uint64_t splitmix64(uint64_t& x) {
    x += 0x9E3779B97F4A7C15ULL;
    uint64_t z = x;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

// This function initialises the Zobrist values for each
// piece and spot by iterating the seed through the mixer.

void init_zobrist(uint64_t seed) 
{
    uint64_t test = seed;
    if (Z_PIECE[0][0] == splitmix64(test))
        return;

    for (int side = 0; side < 2; ++side)
        for (int sq = 0; sq < 64; ++sq)
            Z_PIECE[side][sq] = splitmix64(seed);
}
