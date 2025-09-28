#pragma once

// Functions to be used by main() to operate with and solve Connect4 boards

enum Player : unsigned char
{
	NONE = 0,
	RED = 1,
	YELLOW = 2,
	INVALID = 3,
};

struct Connect4Board
{
	Player board[8][8] = { NONE };

	Player currentPlayer = RED;

	Connect4Board(const unsigned char T[8][8]);
	Connect4Board() = default;
};

extern Player solveConnect4(const Connect4Board& board, unsigned char depth);

extern void fancyPrintBoard(const Connect4Board& board);

extern void playAgainstMachine(const Connect4Board board, const Player YourPlayer, unsigned char depth, unsigned char bitDepth, bool increase_depth);

extern void MachineAgainstMachine(const Connect4Board board, float Max_time, unsigned char bitDepth, bool clear_screen = true);

extern float evaluatePosition(const Connect4Board& board, unsigned char depth, unsigned char bitDepth);

extern unsigned char* findBestMove(const Connect4Board& board);