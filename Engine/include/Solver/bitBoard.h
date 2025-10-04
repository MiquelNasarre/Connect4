#pragma once
#include "zobrist.h"
#include "mask.h"

#define BIT_BOARD

/* CONNECT4 BITBOARD HEADER FILE
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This header includes macros, enums, structures and inline functions
to create a Connect4 bitBoard useful for fast computation.
It includes zobrist.h that will provide a hash for any given board.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// In this representation, the board is a 8x8 grid (64 cells).
// The map bit index is calculated as: index = column * 8 + row.

/*
-------------------------------------------------------------------------------------------------------
Constants and enums for the Connect4 bitboard
-------------------------------------------------------------------------------------------------------
*/

// Possible results of the solver.
// The set values are important to simplify minimax algorithm.
enum SolveResult : char
{
	CURRENT_PLAYER_WIN		=  1,
	DRAW					=  0,
	OTHER_PLAYER_WIN		= -1,
	INVALID_BOARD			= -2,
	CURRENT_PLAYER_BETTER	=  2,
	OTHER_PLAYER_BETTER		=  3,
};

// Simple operator to avoid stuffing the code with type casts.
inline SolveResult operator-(const SolveResult& other)
{
	return (SolveResult)-(char)other;
}

/*
-------------------------------------------------------------------------------------------------------
Definition of the bitboard struct that will be used for solving connect4
-------------------------------------------------------------------------------------------------------
*/

// Defines the Connect4 bit-board in a small pack, with all the information needed
// for fast computation. Stores the board encoded as two bitmaps, one per player.
// The heights of each column, for fast legal move recognition, the move count
// and side to play, and the board hash for Transposition Tables integration.
typedef struct board {

	// Each player's pieces are represented in a 64-bit integer (bitboard)
	uint64_t playerBitboard[2] { 0ULL, 0ULL };

	// Board key needed for comparing in the transposition tables
	uint64_t hash = INITIAL_HASH;

	// heights[column] uses the lower 3 bits to indicate the next available row in that column (0-7)
	// The 4th bit (8) indicates if the column is full (0 = full, 1 = not full)
	uint8_t heights[8] = {};

	// Number of moves played so far
	uint8_t moveCount = 0u;

	// Current player's turn (0 = first player, 1 = second player)
	uint8_t sideToPlay = 0u;

} Board;

/*
-------------------------------------------------------------------------------------------------------
Helper inline functions to cleanup the code
-------------------------------------------------------------------------------------------------------
*/

// Returns the board of both player pieces combined.
static inline uint64_t mask(const Board& board)
{
	return board.playerBitboard[0] | board.playerBitboard[1];
}

// Check if the column is not full.
static inline bool canPlay(const Board& board, const unsigned char column)
{
	return board.heights[column] ^ 8u;
}

// Returns a bit located at a given position in the board.
static inline uint64_t bit_at(const unsigned char column, const unsigned char row)
{
	return 0x1ULL << (column * 8 + row);
}

/*
-------------------------------------------------------------------------------------------------------
In game functions for board operations
-------------------------------------------------------------------------------------------------------
*/

// Checks if the board is valid by checking if there are any floating pieces,
// if the hash of the board corresponds with the one it is supposed to be,
// and if the move count matches the pieces on the board.
static inline bool invalidBoard(const Board& board)
{
	unsigned char moveCount = 0;

	if (board.playerBitboard[0] & board.playerBitboard[1])
		return true;

	if (board.hash != boardHash(board.playerBitboard))
		return true;

	uint64_t boardMask = mask(board);
	for (unsigned char column = 0; column < 8; column++)
	{
		const uint8_t height = board.heights[column];

		if (height > 8)
			return true;

		moveCount += height;

		uint64_t expected = ((1ULL << height) - 1) << (8 * column);
		if ((boardMask & COL_MASK(column)) != expected)
			return true;
	}

	if (moveCount != board.moveCount)
		return true;

	return false;
}

// Places a stone for side-to-move in column c (assumes can_play was true).
// Updates board hash and switches player at the end of the move.
static inline void playMove(Board& board, const unsigned char column)
{
	const uint8_t row = board.heights[column]++;
	const uint64_t move = bit_at(column, row);

	board.playerBitboard[board.sideToPlay] |= move;
	board.hash ^= Z_PIECE[board.sideToPlay][column * 8 + row];

	board.sideToPlay ^= 1; // toggle side to move
	++board.moveCount;
}

// Switches player back to previous move and ndoes hash changes.
// Removes last stone placed in that column (assumes it was played by the correct player).
static inline void undoMove(Board& board, const unsigned char column)
{
	board.sideToPlay ^= 1; // untoggle side to move
	--board.moveCount;

	const uint8_t row = --board.heights[column];
	const uint64_t move = bit_at(column, row);

	board.playerBitboard[board.sideToPlay] ^= move;
	board.hash ^= Z_PIECE[board.sideToPlay][column * 8 + row];
}

// Check if the current player has a winning position.
// To do this it collapses the bitboard in each direction in itself 4 times,
// then only the bits that are set 4 times in a row will remain.
// This allows checking for all same type wins in a single operation.
// Then masks are used to filter out cross border connect4's.
inline bool is_win(const uint64_t playerBitboard)
{
	uint64_t m;

	// Horizontal (east = -8)
	m = playerBitboard & SHIFT_EAST(playerBitboard);
	if (m & SHIFT_2_EAST(m) & MASK_3E) return true;

	// Vertical (south = +1)
	m = playerBitboard & SHIFT_SOUTH(playerBitboard);
	if (m & SHIFT_2_SOUTH(m) & MASK_3S) return true;

	// Diagonal SW (+9)
	m = playerBitboard & SHIFT_SW(playerBitboard);
	if (m & SHIFT_2_SW(m) & MASK_3SW) return true;

	// Diagonal SE (-7)
	m = playerBitboard & SHIFT_SE(playerBitboard);
	if (m & SHIFT_2_SE(m) & MASK_3SE) return true;

	return false;
}