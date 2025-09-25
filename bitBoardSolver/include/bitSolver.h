#pragma once
#include "bitBoard.h"

// Possible results of the solver
// The set values are important to simplify minimax algorithm

enum SolveResult : char
{
	CURRENT_PLAYER_WIN	=  1,
	DRAW				=  0,
	OTHER_PLAYER_WIN	= -1,
	INVALID_BOARD		= -2,
};

// Simple operator to avoid stuffing the code with type casts

inline SolveResult operator-(const SolveResult& other)
{
	return (SolveResult)-(char)other;
}

// Returns the score of the board found after generating a tree
// The tree uses alpha-beta pruning and its depth moves deep

SolveResult solveBoard(const Board& initialBoard, unsigned char depth);

// Calls the function clear on the transposition table

void clearTranspositionTable();