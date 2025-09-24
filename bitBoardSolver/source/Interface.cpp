#include "Interface.h"
#include "bitSolver.h"
#include "Timer.h"

#include <cstdio>

// Preferred move order for better pruning

constexpr unsigned char moveOrder[8] = { 3,4,2,5,1,6,0,7 };


static inline Connect4Board* decodeBoard(const Board& bitBoard)
{
	Connect4Board* board = new(Connect4Board);

	for(unsigned int col = 0; col<8;col++)
		for (unsigned int row = 0; row < 8; row++)
		{
			if (bitBoard.playerBitboard[0] & bit_at(col, row))
				board->board[row][col] = RED;
			if (bitBoard.playerBitboard[1] & bit_at(col, row))
				board->board[row][col] = YELLOW;
		}

	board->currentPlayer = bitBoard.sideToPlay ? YELLOW : RED;

	return board;
}

static inline Board* translateBoard(const Connect4Board& board)
{
	Board* bitBoard = new Board();
	for (unsigned char col = 0; col < 8; col++)
	{
		for (unsigned char row = 0; row < 8; row++)
		{
			if (board.board[row][col] == RED)
			{
				bitBoard->playerBitboard[0] |= bit_at(col, row);
#ifdef usingTT
				bitBoard->hash ^= Z_PIECE[0][8 * col + row];
#endif
				bitBoard->heights[col]++;
				bitBoard->moveCount++;
			}
			else if (board.board[row][col] == YELLOW)
			{
				bitBoard->playerBitboard[1] |= bit_at(col, row);
#ifdef usingTT
				bitBoard->hash ^= Z_PIECE[1][8 * col + row];
#endif
				bitBoard->heights[col]++;
				bitBoard->moveCount++;
			}
		}
	}
	bitBoard->sideToPlay = (board.currentPlayer == RED) ? 0 : 1;
	return bitBoard;
}

Connect4Board::Connect4Board(const unsigned char T[8][8])
{
	for (unsigned char row = 0; row < 8; row++)
		for (unsigned char col = 0; col < 8; col++)
			board[row][col] = (Player)T[7 - row][col];
}

void printBoard(const Connect4Board& board)
{
	printf("\n");
	for (int row = 0; row < 8; row++)
	{
		for (int col = 0; col < 8; col++)
		{
			char cell;
			switch (board.board[7 - row][col])
			{
			case RED:
				cell = 'R';
				break;
			case YELLOW:
				cell = 'Y';
				break;
			default:
				cell = ' ';
				break;
			}
			printf("|%c", cell);
		}
		printf("|\n");
	}
	printf("\n");
}

Player solveConnect4(const Connect4Board& board, unsigned char depth)
{
	Board* bitBoard = translateBoard(board);
	SolveResult result = solveBoard(*bitBoard, depth);
	delete bitBoard;
	switch (result)
	{
	case CURRENT_PLAYER_WIN:
		return board.currentPlayer;
	case OTHER_PLAYER_WIN:
		return (board.currentPlayer == RED) ? YELLOW : RED;
	case DRAW:
		return NONE;
	default:
		return INVALID;
	}
}

Eval decideNextMove(const Connect4Board& board, unsigned char depth)
{
	Board bitBoard = *translateBoard(board);

	Eval eval;

	eval.column = 8;

	SolveResult best = OTHER_PLAYER_WIN;

	for (unsigned char column : moveOrder)
		if (canPlay(bitBoard, column))
		{
			playMove(bitBoard, column);
			SolveResult result = (SolveResult)-solveBoard(bitBoard, depth);
			undoMove(bitBoard, column);

			if (result > best)
			{
				best = result;
				eval.column = column;
			}
		}

	if (eval.column == 8)
		for (unsigned char column : moveOrder)
			if (canPlay(bitBoard, column))
			{
				eval.column = column;
				break;
			}

	switch (best)
	{
	case CURRENT_PLAYER_WIN:
		eval.winner = board.currentPlayer;
		break;
	case OTHER_PLAYER_WIN:
		eval.winner = (board.currentPlayer == RED) ? YELLOW : RED;
		break;
	case DRAW:
		eval.winner = NONE;
		break;
	default:
		eval.winner = INVALID;
		break;
	}
	return eval;
}

void playAgainstMachine(const Connect4Board board, const Player YourPlayer, const unsigned char depth)
{
	Timer timer;

	printf("Initial Board:\n");
	
	printBoard(board);

	Board bitBoard = *translateBoard(board);

	while (!is_win(bitBoard.playerBitboard[0]) && !is_win(bitBoard.playerBitboard[1]) && bitBoard.moveCount < 64)
	{
		if (YourPlayer - 1 == bitBoard.sideToPlay)
		{
			unsigned char column;
			printf("\nChoose a column:  ");
			scanf("%hhu", &column);
			if (column >= 8 || !canPlay(bitBoard, column))
			{
				printf("Invalid column!!");
				continue;
			}
			playMove(bitBoard, column);

			printBoard(*decodeBoard(bitBoard));
		}
		else
		{
			timer.reset();
			Eval eval = decideNextMove(*decodeBoard(bitBoard), depth);
			float time = timer.check();
			
			printf("\nThe code has chosen column %u after thinking for %.3fs and evaluates the position as %s", eval.column, time,
				eval.winner == RED ? "winning for RED\n" : ((eval.winner == YELLOW) ? "winning for YELLOW\n" : "a draw\n"));
			
			playMove(bitBoard, eval.column);

			printBoard(*decodeBoard(bitBoard));
		}
	}

	if (is_win(bitBoard.playerBitboard[0]))
		printf("\nPlayer RED has won!!\n");

	else if (is_win(bitBoard.playerBitboard[1]))
		printf("\nPlayer YELLOW has won!!\n");

	else
		printf("\nThe game has ended in a draw\n");
}