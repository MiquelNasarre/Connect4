#pragma once
#include <cstdint>
#include <stdlib.h>
#include <cstring>

/* HEURISTIC TRANSPOSITION TABLE HEADER FILE
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This header will store the structures, funtions and macros needed
to use transposition tables in our heuristic tree
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

/*
-------------------------------------------------------------------------------------------------------
Macros for HTTs
-------------------------------------------------------------------------------------------------------
*/

// Depth testing is for when you have multiple trees sharing TT at different depths

#define DEPTH_TESTING

// Number of entries that will be created by defaul in a transposition table

#define HTT_SIZE 0x4000ULL // 3/8 MB (16384 entries)

// Flags for defining the type of entry depending on the kind of information
// we have about the Board score
// We might know the exact score, or an upper or lower bound of the score

#define ENTRY_FLAG_EXACT 0u
#define ENTRY_FLAG_LOWER 1u
#define ENTRY_FLAG_UPPER 2u

/*
-------------------------------------------------------------------------------------------------------
Transmutation Table and its Entries defintion
-------------------------------------------------------------------------------------------------------
*/

// This structure defines an entry of the transposition table, storing the key or board hash
// and other values the minimax algorith will use to avoid unnecesary computation.

struct HTTEntry {
    uint64_t key;           // full key
    uint8_t  order[8];      // stores the moves ordered through the evaluation
    float    eval;          // -1 to +1 from current side POV
    uint8_t  depth;         // remaining depth stored
    uint8_t  flag;          // 0=EXACT, 1=LOWER, 2=UPPER
    // pad 4 bytes

    HTTEntry(uint64_t key, uint8_t order[8], float eval, uint8_t depth, uint8_t flag)
        : key{ key }, eval{ eval }, depth{ depth }, flag{ flag }
    {
        this->order[0] = order[0];
        this->order[1] = order[1];
        this->order[2] = order[2];
        this->order[3] = order[3];
        this->order[4] = order[4];
        this->order[5] = order[5];
        this->order[6] = order[6];
        this->order[7] = order[7];
    }
};

// This structure defines our tranposition table, it stores an array of 2^n entries
// Each board can acces an array spot masking their key (hash)

struct HeuristicTransTable {

    HTTEntry* entries = nullptr;
    uint32_t mask;

    inline HeuristicTransTable(size_t pow2_entries = HTT_SIZE)
    {
        init(pow2_entries);
    }
    inline ~HeuristicTransTable()
    {
        erase();
    }

    // This function initialises the transposition table to a given length

    inline void init(size_t pow2_entries = HTT_SIZE)
    {
        if (entries)
            free(entries);

        entries = (HTTEntry*)calloc(sizeof(HTTEntry), pow2_entries);
        mask = (uint32_t)(pow2_entries - 1);
    }

    // This function sets to zero the entire transposition table

    inline void clear()
    {
        if (!entries)
            return;

        memset(entries, 0U, sizeof(HTTEntry) * (mask + 1));
    }

    // This function erases the memory of the transposition table

    inline void erase()
    {
        if (!entries)
            return;

        free(entries);
        entries = nullptr;
    }

    // Checks to  see if the board position is stored in the table
    // If so, returns the pointer, otherwise returns nullptr

    inline HTTEntry* storedBoard(uint64_t key) const
    {
        HTTEntry* e0 = &entries[key & mask];
        if (e0->key == key) return e0;
        HTTEntry* e1 = &entries[(key & mask) ^ 1];
        if (e1->key == key) return e1;

        return nullptr;
    }

    // This function returns the pointer to an entry of the table given a certain key
    // To choose the position it choses between the masked key or its tggled one,
    // allowing for better collision management. It is called 2-slot bucket

    inline HTTEntry* probe(uint64_t key) const
    {
        HTTEntry* e0 = &entries[key & mask];
        if (e0->key == key) return e0;
        HTTEntry* e1 = &entries[(key & mask) ^ 1];
        if (e1->key == key) return e1;

        // choose a victim by lower depth (or empty)
        if (e0->depth <= e1->depth) return e0;
        return e1;
    }

    // This function receives a TTentry and stores it inside te transmutation table

    inline void store(uint64_t key, uint8_t order[8], uint8_t depth, float score, uint8_t flag) const
    {
        HTTEntry* e = probe(key);

        if (depth >= e->depth || e->key != key)
            *e = HTTEntry(key, order, score, depth, flag);
    }
};