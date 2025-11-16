#pragma once
#include <cstdint>
#include <stdlib.h>
#include <cstring>

/* TRANSPOSITION TABLE HEADER FILE 
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This header stores the structures, funtions and macros needed
to use transposition tables in our tree generation algorithms.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

/*
-------------------------------------------------------------------------------------------------------
Macros for TTs
-------------------------------------------------------------------------------------------------------
*/

#define _EXACT_TT

// Number of entries that will be created by defaul in a transposition table.
// 
// The solver generates a TT for every moveCount that it analises. That way it avoids 
// overriding hot boards of different depths. But that comes at the cost of a lor of 
// memory usage. 
// The number below seems to be the perfect balance between maintaining hot paths 
// avaliable on the TT and not making it too big for the program to search for
// different entries. Modify with caution.

#define TT_SIZE 0x4000ULL // 1/4 MB (16384 entries)

// Flags for defining the type of entry depending on the kind of information
// we have about the Board score.
// We might know the exact score, or an upper or lower bound of the score.

#define ENTRY_FLAG_EXACT 0u
#define ENTRY_FLAG_LOWER 1u
#define ENTRY_FLAG_UPPER 2u

/*
-------------------------------------------------------------------------------------------------------
Transposition Table and its Entries defintion
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
    // pad 4 bytes
};

// This structure defines our tranposition table, it stores an array of 2^n entries.
// Each board can acces an entry of the array masking their key (hash).
struct TransTable 
{
private:
    TTEntry* entries = nullptr;
    uint32_t mask;

public:
    inline TransTable(size_t pow2_entries = TT_SIZE)
    {
        init(pow2_entries);
    }
    inline ~TransTable()
    {
        erase();
    }

    // Checks wether the entries have been created or not.
    inline bool is_init()
    {
        if (entries)
            return true;
        return false;
    }

    // This function initialises the transposition table to a given length.
    // The lenght must be a power of 2 for the mask to work properly.
    inline void init(size_t pow2_entries = TT_SIZE)
    {
        if (entries)
            free(entries);

        entries = (TTEntry*)calloc(sizeof(TTEntry), pow2_entries);
        mask = (uint32_t)(pow2_entries - 1);
    }

    // This function sets to zero the entire transposition table.
    inline void clear()
    {
        if (!entries)
            return;

        memset(entries, 0U, sizeof(TTEntry) * (mask + 1));
    }

    // This function erases the memory of the transposition table.
    inline void erase()
    {
        if (!entries)
            return;

        free(entries);
        entries = nullptr;
    }

    // Checks to  see if the board position is stored in the table.
    // If so, returns the pointer, otherwise returns nullptr.
    inline TTEntry* storedBoard(uint64_t key) const
    {
        TTEntry* e0 = &entries[key & mask];
        if (e0->key == key) return e0;
        TTEntry* e1 = &entries[(key & mask) ^ 1];
        if (e1->key == key) return e1;

        return nullptr;
    }

    // This function returns the pointer to an entry of the table given a certain key.
    // To choose the position it choses between the masked key or its tggled one,
    // allowing for better collision management. It is called 2-slot bucket.
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

    // This function receives a TT entry and stores it inside te transposition table.
    inline int8_t store(uint64_t key, uint8_t depth, int8_t score, uint8_t flag, uint8_t bestCol) const
    {
        TTEntry* e = probe(key);
        
        // If keys are different or you are deeper
        if (depth >= e->depth || e->key != key)
            *e = TTEntry{ key, depth, score, flag, bestCol };

        return score;
    }
};