#pragma once
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
	float eval;				// Value from -1 to 1 represents the evaluation for the current player
	unsigned char column;	// The best column to play for the current player
	unsigned char depth;	// Depth at which this position has been evaluated
	SolveResult flag;		// Indicates what kind of position is evaluated
};

// This is the depth value at which the heuristic tree
// stops calling the function orderMoves.

#define NO_ORDERING_DEPTH 2

// This value is the multiplier for the heuristic function value.
// It has to be small enough to maintain the result between +-1.

#define POINT_DISTANCE 0x1.0p-6f // 1/64 in hexadecimal floating-point

#define DEFAULT_TAIL_DEPTH		4

#define WEIGHT_FAVORABLES		(1.f * POINT_DISTANCE)
#define WEIGHT_POSSIBLES		(2.f * POINT_DISTANCE)
#define WEIGHT_FAVORABLE_1MOVE	(1.f * POINT_DISTANCE)
#define WEIGHT_POSSIBLES_1MOVE	(2.f * POINT_DISTANCE)

// Const variables that every heuristic tree uses to generate and evaluate boards.
struct HeuristicData
{
	HeuristicTransTable* HTT = nullptr;					// Stores the heuristic tree transposition table
	TransTable* TT = nullptr;							// Stores the exact tree transposition table

	bool* STOP = nullptr;								// Bool* to cancel the tree generation from a different thread

	// Variables to be set by ML algorithms

	unsigned char ORDERING_DEPTH = NO_ORDERING_DEPTH;	// Depth at which the heuristic tree stops calling orderMoves()
	unsigned char EXACT_TAIL = DEFAULT_TAIL_DEPTH;		// Depth of the exact tree called by the heuristic function

	float FAVORABLES = WEIGHT_FAVORABLES;				// Weight of favorable connect 4 cases for each color
	float POSSIBLES = WEIGHT_POSSIBLES;					// Whight of possible connect 4 cases for each color
	float FAVORABLE_1MOVE = WEIGHT_FAVORABLE_1MOVE;		// Weight of favorable 1 move wins for each color
	float POSSIBLES_1MOVE = WEIGHT_POSSIBLES_1MOVE;		// Weight of possible 1 move wins for each color
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
extern inline float heuristicTree(Board& board, float alpha, float beta, unsigned char depth, const HeuristicData& DATA);

// This will return all the information above after processing a tree of the board to a given depth.
// Every heuristic function call will check for wins using the bitSoler to a given bitDepth.
extern SolveEval evaluateBoard(const Board& initialBoard, unsigned char depth, HeuristicData const* DATA = nullptr);
