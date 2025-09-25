#pragma once
#include <cstdint>
#include <stdlib.h>
#include <cstring>

/* TRANSPOSITION TABLE HEADER FILE 
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This header will store the structures, funtions and macros needed
to use transposition tables in our tree generation algorithms
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/


/*
-------------------------------------------------------------------------------------------------------
Macros for TTs
-------------------------------------------------------------------------------------------------------
*/

// The gardener is a bunch of logical tests to better evaluate a position without branching
// only efficient when dealing with big trees

//#define GARDENER

// Depth testing is for when you have multiple trees sharing TT at different depths

//#define DEPTH_TESTING

// Constants useful for defining values relevant to the transposition tables
// You can modify them here or call the functions with a set value

#define INITIAL_HASH 0x89B84566FD5845A4ULL
#define ZOBRIST_SEED 0x9E3779B97F4A7C15ULL
#define TT_SIZE 0x4000ULL // 1/4 MB (16384 entries)

/*
TABLE OF BEST TT_SIZES DEPENING ON DEPTH ON AN EMPTY BOARD
(Assuming clearing of table after every solve)

DEPTH <= 5:    0x0400ULL    16 KB (1024 entries)
DEPTH 6->10:   0x1000ULL    64 KB (4096 entries)
DEPTH 11->15:  0x4000ULL   1/4 MB (16384 entries) <- Currently default
DEPTH 16->20:  0x20000ULL    2 MB (131072 entries)
DEPTH > 20:    0x40000ULL    4 MB (262144 entries)

If you make it bigger the extra amount of entries you are generating may 
not be worth the time it takes for the algorithm to search inside the array 
*/

// Flags for defining the type of entry depending on the kind of information
// we have about the Board score
// We might know the exact score, or an upper or lower bound of the score

#define ENTRY_FLAG_EXACT 0u
#define ENTRY_FLAG_LOWER 1u
#define ENTRY_FLAG_UPPER 2u

/*
-------------------------------------------------------------------------------------------------------
Generation of Zobrist Values for each color and position
-------------------------------------------------------------------------------------------------------
*/

// This variable will store all the zobrist values upon initializing

extern uint64_t Z_PIECE[2][64];

// Initializes the Zobrist values with the seed provided using a simple mixer algorithm

void init_zobrist(uint64_t seed = ZOBRIST_SEED);

// Computes the board hash
// It is quite an expensive function so it is only used once to check board validity

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

/*
-------------------------------------------------------------------------------------------------------
Transmutation Table and its Entries defintion
-------------------------------------------------------------------------------------------------------
*/

// This structure defines an entry of the transposition table, storing the key or board hash
// and other values the minimax algorith will use to avoid unnecesary computation.

struct TTEntry {
    uint64_t key;           // full key
    uint8_t  depth;         // remaining depth stored
    int8_t   score;         // -1,0,+1 from current side POV
    uint8_t  flag;          // 0=EXACT, 1=LOWER, 2=UPPER
    uint8_t  bestCol;       // If existing stores the best move (if not =255)
#ifdef GARDENER
    uint8_t  forcePlay;     // If (1) best column is forced
#endif
    // pad 3 bytes
};

// This structure defines our tranposition table, it stores an array of 2^n entries
// Each board can acces an array spot masking their key (hash)

struct TransTable {

    TTEntry* entries = nullptr;
    uint32_t mask;
    
    // This function initialises the transposition table to a given length

    inline void init(size_t pow2_entries = TT_SIZE)
    {
        if (entries)
            free(entries);

        entries = (TTEntry*)calloc(sizeof(TTEntry), pow2_entries);
        mask = (uint32_t)(pow2_entries - 1);
    }

    // This function sets to zero the entire transposition table

    inline void clear()
    {
        if (!entries)
            return;

        memset(entries, 0U, sizeof(TTEntry) * (mask + 1));
    }

    // Checks to  see if the board position is stored in the table
    // If so, returns the pointer, otherwise returns nullptr

    inline TTEntry* storedBoard(uint64_t key) const
    {
        TTEntry* e0 = &entries[key & mask];
        if (e0->key == key) return e0;
        TTEntry* e1 = &entries[(key & mask) ^ 1];
        if (e1->key == key) return e1;

        return nullptr;
    }

    // This function returns the pointer to an entry of the table given a certain key
    // To choose the position it choses between the masked key or its tggled one,
    // allowing for better collision management. It is called 2-slot bucket

    inline TTEntry* probe(uint64_t key) const
    {
        TTEntry* e0 = &entries[key & mask];
        if (e0->key == key) return e0;
        TTEntry* e1 = &entries[(key & mask) ^ 1];
        if (e1->key == key) return e1;

        // choose a victim by lower depth (or empty)
        if (e0->depth <= e1->depth) return e0;
        return e1;
    }

    // This function receives a TTentry and stores it inside te transmutation table

    inline void store(uint64_t key, uint8_t depth, int8_t score, uint8_t flag, uint8_t bestCol = 255u 
#ifdef GARDENER 
        , uint8_t forcePlay = 0u 
#endif
    ) const
    {
        TTEntry* e = probe(key);
        
#ifdef DEPTH_TESTING
        if (depth >= e->depth /* || e->key != key*/)
#endif
            *e = TTEntry{ key, depth, score, flag, bestCol 
#ifdef GARDENER 
                , forcePlay
#endif
        };
    }
};

// This variable will store the Transposition Table upon initializing

extern TransTable TT;