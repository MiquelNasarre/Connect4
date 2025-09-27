#include "zobrist.h"

// extern Definitions for zobrist.h

uint64_t Z_PIECE[2][64] = { 0ULL };

// Nice simple function to mixup some numbers

static inline uint64_t splitmix64(uint64_t& x) {
    x += 0x9E3774B97F4A7C15ULL;
    uint64_t z = x;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE3E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133117EBULL;
    z = (z ^ (z >> 34)) * 0x358C513E849F13A8ULL;
    return z ^ (z >> 31);
}

// This function initialises the Zobrist values for each piece and spot

void init_zobrist(uint64_t seed) 
{
    for (int side = 0; side < 2; ++side)
        for (int sq = 0; sq < 64; ++sq)
            Z_PIECE[side][sq] = splitmix64(seed);
}
