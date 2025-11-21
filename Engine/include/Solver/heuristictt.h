#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <atomic>

/* HEURISTIC TRANSPOSITION TABLE HEADER FILE
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This header stores the structures, funtions and macros needed
to use transposition tables in our heuristic tree algorithms.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

/*
-------------------------------------------------------------------------------------------------------
Macros for HTTs
-------------------------------------------------------------------------------------------------------
*/

#define _HEURISTIC_TT

// Number of entries that will be created by defaul in a transposition table.
// 
// The solver generates a TT for every moveCount that it analises. That way it avoids 
// overriding hot boards of different depths. But that comes at the cost of a lor of 
// memory usage. 
// The number below seems to be the perfect balance between maintaining hot paths 
// avaliable on the TT and not making it too big for the program to search for
// different entries. Modify with caution.

#define HTT_SIZE 0x4000ULL // 3/8 MB (16384 entries)

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
#include <cstdio>
// This structure defines an entry of the transposition table, stores the current information known
// about a board, used for referencing while generating trees and for user evaluations. The board is 
// encoded as a hash value and it saves depths, best move order, and current evaluation.
struct HTTEntry
{
    uint64_t key;           // full key
    uint8_t  order[8];      // stores the moves ordered through the evaluation
    float    eval;          // -1 to +1 from current side POV
    uint8_t  heuDepth;      // remaining heuristic depth stored
    uint8_t  bitDepth;      // exact tree depth stored
    uint8_t  flag;          // 0=EXACT, 1=LOWER, 2=UPPER
private:
    std::atomic_flag _lock; // To lock the entry while writing/reading
public:

    inline void lock() noexcept
    {
        while (_lock.test_and_set(std::memory_order_acquire)) { /* busy waiting */ }
    }
    inline void unlock() noexcept
    {
        _lock.clear(std::memory_order_release);
    }

    inline void set(const uint64_t _key, const uint8_t _order[8], const float _eval, const uint8_t _heuDepth, const uint8_t _bitDepth, const uint8_t _flag)
    {
        lock();

        eval = _eval;
        heuDepth = _heuDepth;
        bitDepth = _bitDepth;
        flag = _flag;
        order[0] = _order[0];
        order[1] = _order[1];
        order[2] = _order[2];
        order[3] = _order[3];
        order[4] = _order[4];
        order[5] = _order[5];
        order[6] = _order[6];
        order[7] = _order[7];
        key = _key;

        unlock();
    }
    inline void set(const uint64_t _key, const uint8_t _bestCol, const float _eval, const uint8_t _heuDepth, const uint8_t _bitDepth, const uint8_t _flag)
    {
        lock();

        eval = _eval;
        heuDepth = _heuDepth;
        bitDepth = _bitDepth;
        flag = _flag;
        order[0] = _bestCol;
        order[1] = 255; // for debugging (if ever analized will raise error)
        key = _key;

        unlock();
    }
};

// This structure defines our tranposition table, it stores an array of 2^n entries.
// Each board can acces an entry of the array masking their key (hash).
struct HeuristicTransTable 
{
private:
    HTTEntry* entries = nullptr;    // Entries of the Transposition Table
    uint32_t mask;                  // Mask used to link a key/hash with an entry

public:
    inline HeuristicTransTable(size_t pow2_entries = HTT_SIZE)
    {
        init(pow2_entries);
    }
    inline ~HeuristicTransTable()
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
    inline void init(size_t pow2_entries = HTT_SIZE)
    {
        if (entries)
            free(entries);

        entries = (HTTEntry*)calloc(sizeof(HTTEntry), pow2_entries);
        mask = (uint32_t)(pow2_entries - 1);
    }

    // This function sets to zero the entire transposition table.
    inline void clear()
    {
        if (!entries)
            return;

        memset(entries, 0U, sizeof(HTTEntry) * (mask + 1));
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
    inline HTTEntry* storedBoard(uint64_t key) const
    {
        HTTEntry* e0 = &entries[key & mask];
        if (e0->key == key) return e0;
        HTTEntry* e1 = &entries[(key & mask) ^ 1];
        if (e1->key == key) return e1;

        return nullptr;
    }

    // This function returns the pointer to an entry of the table given a certain key.
    // To choose the position it choses between the masked key or its tggled one,
    // allowing for better collision management. It is called 2-slot bucket.
    inline HTTEntry* probe(uint64_t key) const
    {
        HTTEntry* e0 = &entries[key & mask];
        if (e0->key == key) return e0;
        HTTEntry* e1 = &entries[(key & mask) ^ 1];
        if (e1->key == key) return e1;

        // choose a victim by lower depth (or empty)
        return (e0->heuDepth <= e1->heuDepth) ? e0 : e1;
    }

    // This function receives a TTentry and stores it inside te transposition table.
    inline float store(uint64_t key, uint8_t order[8], float eval, uint8_t heuDepth, uint8_t bitDepth, uint8_t flag) const
    {
        HTTEntry* e = probe(key);

        // If keys are different or you are deeper and no victory found
        if (e->key != key || (heuDepth >= e->heuDepth && e->eval != -1.f && e->eval != 1.f) || eval == 1.f || eval == -1.f)
            e->set(key, order, eval, heuDepth, bitDepth, flag);

        return eval;
    }

    // This function receives a TTentry and stores it inside te transposition table.
    inline float store(uint64_t key, uint8_t bestCol, float eval, uint8_t heuDepth, uint8_t bitDepth, uint8_t flag) const
    {
        HTTEntry* e = probe(key);

        // If keys are different or you are deeper and no victory found
        if (e->key != key || (heuDepth >= e->heuDepth && e->eval != -1.f && e->eval != 1.f) || eval == 1.f || eval == -1.f)
            e->set(key, bestCol, eval, heuDepth, bitDepth, flag);

        return eval;
    }
};