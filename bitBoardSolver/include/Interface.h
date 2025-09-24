#pragma once

// Functions to be used by main() to operate with and solve Connect4 boards

enum Player : unsigned char
{
	NONE = 0,
	RED = 1,
	YELLOW = 2,
	INVALID = 3
};

struct Connect4Board
{
	Player board[8][8] = { NONE };

	Player currentPlayer = RED;

	Connect4Board(const unsigned char T[8][8]);
	Connect4Board() = default;
};

typedef struct eval
{
	unsigned char column;
	Player winner;
} Eval;

Player solveConnect4(const Connect4Board& board, unsigned char depth);

Eval decideNextMove(const Connect4Board& board, unsigned char depth);

void printBoard(const Connect4Board& board);

void playAgainstMachine(const Connect4Board board, const Player YourPlayer, const unsigned char depth);