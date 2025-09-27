#pragma once
#include "bitBoard.h"

// Moment at which the algorithm switches to fully solving the position without heuristic

#define MOVE_COUNT_TRIGGER 30

// Return values of the evaluation, this includes a numerical value between -1 and 1
// the column that gives this best result for the current player
// the depth that this position was evaluated
// and a flag that calls the state of the game

struct SolveEval
{
	float eval;
	unsigned char column;
	unsigned char depth;
	SolveResult flag;

	SolveEval(float eval, unsigned char column, unsigned char depth, SolveResult flag) 
		: eval{ eval }, column{ column }, depth{ depth }, flag{ flag } {}
};

extern SolveEval evaluateBoard(const Board& initialBoard, unsigned char depth, unsigned char bitDepth);

extern SolveEval evaluateBoardTime(const Board& initialBoard, float Max_time, unsigned char bitDepth);