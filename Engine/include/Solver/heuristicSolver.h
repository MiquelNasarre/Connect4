#pragma once
#include "bitBoard.h"
#include "bitSolver.h"
#include "heuristictt.h"

/* HEURISTIC TREE SOLVING FUNTIONS HEADER FILE
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This header includes functions to numerically evaluate a board position
this funtions will return the SolveEval struct, with the values listed below.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// Return values of the evaluation, this includes a numerical value between -1 and 1,
// the column that gives this best result for the current player,
// the depth that this position was evaluated
// and a flag that represents the state of the game.

struct SolveEval
{
	float eval;
	unsigned char column;
	unsigned char depth;
	SolveResult flag;
};

// This is the main function of the heuristic solver, computing the tree to solve the position to a
// certain depth, and telling the heuristic to search further with the bitTree to a certain bitDepth.
//
// The tree for better performance uses transposition tables, that check whether the position has
// already been examined and its indormation is stored in the database.
// 
// It also uses alpha-beta pruning, this time paired with node ordering and PVS, which added to
// the transposition tables makes the pruning a lot more efficient.
//
// Use of this function in particular is only intended for direct interaction by engines.
// For user-end use is safer to use the other functions.

extern inline float heuristicTree(Board& board, float alpha, float beta, unsigned char depth, unsigned char bitDepth, HeuristicTransTable* HTT, TransTable* TT = nullptr, bool* stop = nullptr);

// This will return all the information above after processing a tree of the board to a given depth.
// Every heuristic function call will check for wins using the bitSoler to a given bitDepth.

extern SolveEval evaluateBoard(const Board& initialBoard, unsigned char depth, unsigned char bitDepth, HeuristicTransTable* HTT = nullptr, TransTable* TT = nullptr);

// Evaluates the given board with Iterative Deepening using the evaluateBoard funtion
// until a certain time threshold is reached or it finds a win.
// Then it returns the deepest evaluation computed.

extern SolveEval evaluateBoardTime(const Board& initialBoard, float Max_time, unsigned char bitDepth, HeuristicTransTable* HTT = nullptr, TransTable* TT = nullptr);