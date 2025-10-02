#pragma once
#include "bitBoard.h"
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

// This will return all the information above after processing a tree of the board to a given depth.
// Every heuristic function call will check for wins using the bitSoler to a given bitDepth.

extern SolveEval evaluateBoard(const Board& initialBoard, unsigned char depth, unsigned char bitDepth, unsigned char heuristic, HeuristicTransTable* HTT = nullptr);

// Evaluates the given board with Iterative Deepening using the evaluateBoard funtion
// until a certain time threshold is reached or it finds a win.
// Then it returns the deepest evaluation computed.

extern SolveEval evaluateBoardTime(const Board& initialBoard, float Max_time, unsigned char bitDepth, unsigned char heuristic);