#pragma once
#include "zobrist.h"

/*
-------------------------------------------------------------------------------------------------------
Definition of the board object that will be used for solving connect4
-------------------------------------------------------------------------------------------------------
*/

// Possible results of the solver
// The set values are important to simplify minimax algorithm

enum SolveResult : char
{
	CURRENT_PLAYER_WIN = 1,
	DRAW = 0,
	OTHER_PLAYER_WIN = -1,
	INVALID_BOARD = -2,
	CURRENT_PLAYER_BETTER = 2,
	OTHER_PLAYER_BETTER = 3,
};

// Simple operator to avoid stuffing the code with type casts

inline SolveResult operator-(const SolveResult& other)
{
	return (SolveResult)-(char)other;
}

// In this representation, the board is a 8x8 grid (64 cells)
// the map bit index is calculated as: index = column * 8 + row

inline constexpr uint64_t Column0 = 0x00000000000000FFULL; // Mask for the column 0
inline constexpr uint64_t Column1 = 0x000000000000FF00ULL; // Mask for the column 1
inline constexpr uint64_t Column2 = 0x0000000000FF0000ULL; // Mask for the column 2
inline constexpr uint64_t Column3 = 0x00000000FF000000ULL; // Mask for the column 3
inline constexpr uint64_t Column4 = 0x000000FF00000000ULL; // Mask for the column 4
inline constexpr uint64_t Column5 = 0x0000FF0000000000ULL; // Mask for the column 5
inline constexpr uint64_t Column6 = 0x00FF000000000000ULL; // Mask for the column 6
inline constexpr uint64_t Column7 = 0xFF00000000000000ULL; // Mask for the column 7

#define COL_MASK(c) (Column0 << ((c) * 8)) // Macro to get the mask for a given column

inline constexpr uint64_t Row0 = 0x0101010101010101ULL; // Mask for the row 0
inline constexpr uint64_t Row1 = 0x0202020202020202ULL; // Mask for the row 1
inline constexpr uint64_t Row2 = 0x0404040404040404ULL; // Mask for the row 2
inline constexpr uint64_t Row3 = 0x0808080808080808ULL; // Mask for the row 3
inline constexpr uint64_t Row4 = 0x1010101010101010ULL; // Mask for the row 4
inline constexpr uint64_t Row5 = 0x2020202020202020ULL; // Mask for the row 5
inline constexpr uint64_t Row6 = 0x4040404040404040ULL; // Mask for the row 6
inline constexpr uint64_t Row7 = 0x8080808080808080ULL; // Mask for the row 7

inline constexpr uint64_t collapseSW = 0x0000001F1F1F1F1FULL; // Mask to check win in diagonals collapsed SW
inline constexpr uint64_t collapseSE = 0x1F1F1F1F1F000000ULL; // Mask to check win in diagonals collapsed SE
inline constexpr uint64_t collapseE  = 0xFFFFFFFFFF000000ULL; // Mask to check win in horozontal collapsed E
inline constexpr uint64_t collapseS  = 0x1F1F1F1F1F1F1F1FULL; // Mask to check win in vertical collapsed S

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

// Returns the board of both player pieces combined

static inline uint64_t mask(const Board& board)
{
	return board.playerBitboard[0] | board.playerBitboard[1];
}

// Check if the column is not full

static inline bool canPlay(const Board& board, const unsigned char column)
{
	return board.heights[column] ^ 8u;
}

// Returns a bit located at a given position in the board

static inline uint64_t bit_at(const unsigned char column, const unsigned char row)
{
	return 0x1ULL << (column * 8 + row);
}

/*
-------------------------------------------------------------------------------------------------------
In game functions for board operations
-------------------------------------------------------------------------------------------------------
*/

// Checks if the board is valid
// By checking if there are any floating pieces
// and if the move count matches the pieces on the board

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

// Place a stone for side-to-move in column c (assumes can_play was true)
// Updates board hash and switches player at the end of the move

static inline void playMove(Board& board, const unsigned char column)
{
	const uint8_t row = board.heights[column]++;
	const uint64_t move = bit_at(column, row);

	board.playerBitboard[board.sideToPlay] |= move;
	board.hash ^= Z_PIECE[board.sideToPlay][column * 8 + row];

	board.sideToPlay ^= 1; // toggle side to move
	++board.moveCount;
}

// Switches player back to previous move. Undoes hash changes
// Removes last stone placed in that column (assumes it was played by the correct player)

static inline void undoMove(Board& board, const unsigned char column)
{
	board.sideToPlay ^= 1; // untoggle side to move
	--board.moveCount;

	const uint8_t row = --board.heights[column];
	const uint64_t move = bit_at(column, row);

	board.playerBitboard[board.sideToPlay] ^= move;
	board.hash ^= Z_PIECE[board.sideToPlay][column * 8 + row];
}

// Check if the current player has a winning position
// To do this it collapses the bitboard in each direction in itself 4 times,
// then only the bits that are set 4 times in a row will remain
// This allows checking for all same type wins in a single operation
// Then masks are used to filter out cross border connect4's

inline bool is_win(const uint64_t playerBitboard) 
{
	uint64_t m;

	// Horizontal (east = -8)
	m = playerBitboard & (playerBitboard << 8);
	if (m & (m << 16) & collapseE) return true;

	// Vertical (south = +1)
	m = playerBitboard & (playerBitboard >> 1);
	if (m & (m >> 2) & collapseS) return true;

	// Diagonal SW (+9)
	m = playerBitboard & (playerBitboard >> 9);
	if (m & (m >> 18) & collapseSW) return true;

	// Diagonal SE (-7)
	m = playerBitboard & (playerBitboard << 7);
	if (m & (m << 14) & collapseSE) return true;

	return false;
}