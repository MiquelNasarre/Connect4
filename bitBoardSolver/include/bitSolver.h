#pragma once
#include "bitBoard.h"

/* ABSOLUTE TREE SOLVING FUNTIONS HEADER FILE
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This header includes functions to solve board positions up to 
a given depth, its return values are either win, loss or draw.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// Returns the score of the board found after generating a tree.
// The tree uses alpha-beta pruning and its depth moves deep.

extern SolveResult solveBoard(const Board& initialBoard, unsigned char depth, bool clearTT = false);

// Same as the previous one but assumes validity checks have been done.
// Used for win checks on bigger heuristic trees.

extern SolveResult noChecksSolveBoard(const Board& initialBoard, unsigned char depth);

// If the position is stored on the transposition table it retrieves the best column.
// if the position is a mate situation it does not guarantee best path.

extern unsigned char retrieveColumn(const Board& board);

// If there is a forced win by any of the players it will find the best move.
// For the winning player is the one that wins the fastest.
// For the losing player is the one that delays the loss the longest.
// First value is the column second value is the distance.

extern unsigned char* findBestPath(const Board& board);