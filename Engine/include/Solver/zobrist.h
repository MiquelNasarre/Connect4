#pragma once
#include <cstdint>

/* ZOBRIST VALUES GENERATION HEADER FILE
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This header stores the functions needed to initialize the zobrist values 
in the program, the variable where these will be hold and a function to 
compute the hash of a given board position.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

/*
-------------------------------------------------------------------------------------------------------
Macros for Zobrist generation
-------------------------------------------------------------------------------------------------------
*/

// The initial hash is the hash value of an empty board.
// This is important otherwise the initial board hash would be 0.

#define INITIAL_HASH 0x89B84566FD5845A4ULL

// This seed is a random number used to generate the zobrist values.
// It can be modified with no problem.

#define ZOBRIST_SEED 0x9E3779B97F4A7C15ULL

/*
-------------------------------------------------------------------------------------------------------
Generation of Zobrist Values for each color and position
-------------------------------------------------------------------------------------------------------
*/

// This variable will store all the zobrist values upon initializing.

extern uint64_t Z_PIECE[2][64];

// Initializes the Zobrist values with the seed provided using a simple mixer algorithm.

extern void init_zobrist(uint64_t seed = ZOBRIST_SEED);

// Computes the board hash.
// It is quite an expensive function so it is only used for board validity checks.

static inline uint64_t boardHash(const uint64_t playerBitboards[2])
{
    uint64_t hash = INITIAL_HASH;
    uint64_t p0 = playerBitboards[0];
    uint64_t p1 = playerBitboards[1];
    uint64_t bit = 1ULL;

    for (uint8_t i = 0; i < 64; i++)
    {
        if (p0 & bit)
            hash ^= Z_PIECE[0][i];

        if (p1 & bit)
            hash ^= Z_PIECE[1][i];

        bit <<= 1u;
    }
    return hash;
}

