#include "Interface.h"
#include "bitSolver.h"
#include "heuristicSolver.h"
#include "Timer.h"

#include <stdlib.h>
#include <cstdio>

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
	if (!Z_PIECE[0][0])
		init_zobrist();

	Board* bitBoard = new Board();
	for (unsigned char col = 0; col < 8; col++)
	{
		for (unsigned char row = 0; row < 8; row++)
		{
			if (board.board[row][col] == RED)
			{
				bitBoard->playerBitboard[0] |= bit_at(col, row);
				bitBoard->hash ^= Z_PIECE[0][8 * col + row];

				bitBoard->heights[col]++;
				bitBoard->moveCount++;
			}
			else if (board.board[row][col] == YELLOW)
			{
				bitBoard->playerBitboard[1] |= bit_at(col, row);
				bitBoard->hash ^= Z_PIECE[1][8 * col + row];

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

void fancyPrintBoard(const Connect4Board& board)
{
	constexpr int N = 8;
	char** T = (char**)calloc(N, sizeof(void*));
	for (unsigned char i = 0; i < N; i++)
	{
		T[i] = (char*)calloc(N, sizeof(char));
		for (unsigned char j = 0; j < N; j++)
			T[i][j] = (char)board.board[7 - i][j];
	}

	for (int f = 0; f < N - 3; f++) {
		for (int c = 0; c < N; c++) {
			if (T[f][c] != 0 && T[f][c] == T[f + 1][c] && T[f][c] == T[f + 2][c] && T[f][c] == T[f + 3][c]) { T[f][c] = T[f][c] + 10; T[f + 1][c] = T[f + 1][c] + 10; T[f + 2][c] = T[f + 2][c] + 10; T[f + 3][c] = T[f + 3][c] + 10; }
		}
	}
	for (int f = 0; f < N; f++) {
		for (int c = 0; c < N - 3; c++) {
			if (T[f][c] != 0 && T[f][c] == T[f][c + 1] && T[f][c] == T[f][c + 2] && T[f][c] == T[f][c + 3]) { T[f][c] = T[f][c] + 10; T[f][c + 1] = T[f][c + 1] + 10; T[f][c + 2] = T[f][c + 2] + 10; T[f][c + 3] = T[f][c + 3] + 10; }
		}
	}
	for (int f = 0; f < N - 3; f++) {
		for (int c = 0; c < N - 3; c++) {
			if (T[f][c] != 0 && T[f][c] == T[f + 1][c + 1] && T[f][c] == T[f + 2][c + 2] && T[f][c] == T[f + 3][c + 3]) { T[f][c] = T[f][c] + 10; T[f + 1][c + 1] = T[f + 1][c + 1] + 10; T[f + 2][c + 2] = T[f + 2][c + 2] + 10; T[f + 3][c + 3] = T[f + 3][c + 3] + 10; }
		}
	}
	for (int f = 3; f < N; f++) {
		for (int c = 0; c < N - 3; c++) {
			if (T[f][c] != 0 && T[f][c] == T[f - 1][c + 1] && T[f][c] == T[f - 2][c + 2] && T[f][c] == T[f - 3][c + 3]) { T[f][c] = T[f][c] + 10; T[f - 1][c + 1] = T[f - 1][c + 1] + 10; T[f - 2][c + 2] = T[f - 2][c + 2] + 10; T[f - 3][c + 3] = T[f - 3][c + 3] + 10; }
		}
	}
	system("color");
	printf("\033[0;34m");
	for (int i = 0; i < N; i++) {
		if (!i)printf("\n\n  %c", 201);
		printf("%c%c%c", 205, 205, 205);
		if (i != N - 1)printf("%c", 203);
		else printf("%c", 187);
	}

	for (int i = 0; i < N; i++) {
		printf("\n  ");
		printf("%c", 186);
		for (int j = 0; j < N; j++) {
			if (!T[i][j])printf("   ");
			else {
				if (T[i][j] > 10) {
					T[i][j] = T[i][j] % 10;
					if (T[i][j] == 1)printf("\033[1;31m");
					else if (T[i][j] == 2)printf("\033[1;33m");
					else if (T[i][j] == 3)printf("\033[1;36m");
					else if (T[i][j] == 4)printf("\033[1;32m");
					else if (T[i][j] == 5)printf("\033[1;35m");
					else if (T[i][j] == 6)printf("\033[1;37m");
				}
				else if (T[i][j] == 1)printf("\033[0;31m");
				else if (T[i][j] == 2)printf("\033[0;33m");
				else if (T[i][j] == 3)printf("\033[0;36m");
				else if (T[i][j] == 4)printf("\033[0;32m");
				else if (T[i][j] == 5)printf("\033[0;35m");
				else if (T[i][j] == 6)printf("\033[0;37m");
				printf("%c%c%c", 219, 219, 219);
				printf("\033[0;34m");
			}
			printf("%c", 186);
		}
		printf("\n  ");
		if (i != N - 1) {
			for (int k = 0; k < N; k++) {
				if (!k)printf("%c", 204);
				printf("%c%c%c", 205, 205, 205);
				if (k != N - 1)printf("%c", 206);
				else printf("%c", 185);
			}
		}
		else {
			for (int k = 0; k < N; k++) {
				if (!k)printf("%c", 200);
				printf("%c%c%c", 205, 205, 205);
				if (k != N - 1)printf("%c", 202);
				else printf("%c\n ", 188);
			}
		}
	}
	printf("\033[1;30m");
	for (int i = 0; i < N; i++)printf("   %i", i + 1);
	printf("\n\n");
	printf("\033[0m");
}

Player solveConnect4(const Connect4Board& board, unsigned char depth)
{
	Board* bitBoard = translateBoard(board);
	SolveResult result = solveBoard(*bitBoard, depth, true);
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

void playAgainstMachine(const Connect4Board board, const Player YourPlayer, unsigned char depth, unsigned char bitDepth, bool increase_depth)
{
	Timer timer;

	char you = YourPlayer;

	printf("Initial Board:\n\n\n\n");
	
	fancyPrintBoard(board);

	Board bitBoard = *translateBoard(board);

	while (!is_win(bitBoard.playerBitboard[0]) && !is_win(bitBoard.playerBitboard[1]) && bitBoard.moveCount < 64)
	{
		if (you - 1 == bitBoard.sideToPlay)
		{
			unsigned char column;
			printf("\nChoose a column:  ");
			scanf("%hhu", &column);
			if (column == 9)
			{
				you = 3 - you;
				continue;
			}
			if (!column || column > 8 || !canPlay(bitBoard, column - 1))
			{
				printf("Invalid column!!");
				continue;
			}
			playMove(bitBoard, column - 1);

			system("cls");

			printf("You have played at column %u\n\n\n\n", column);
		}
		else
		{
			timer.reset();
			SolveEval eval = evaluateBoard(bitBoard, depth, bitDepth);
			float time = timer.check();

			system("cls");
			
			printf("The code has chosen column %u after thinking for %.3fs \nIt evaluates the position as ", eval.column + 1, time);
			
			switch (eval.flag)
			{
			case CURRENT_PLAYER_WIN:
				printf("winning for %s.", you == RED ? "YELLOW" : "RED");
				break;
			case OTHER_PLAYER_WIN:
				printf("winning for %s.", you == RED ? "RED" : "YELLOW");
				break;
			case CURRENT_PLAYER_BETTER:
				printf("better for %s.", you == RED ? "YELLOW" : "RED");
				break;
			case OTHER_PLAYER_BETTER:
				printf("better for %s.", you == RED ? "RED" : "YELLOW");
				break;
			case DRAW:
				printf("a draw. Value: %.3f", eval.eval);
				break;
			}

			printf("\nValue: %.3f \n", eval.eval);

			if (eval.eval == 1.f || eval.eval == -1.f)
				printf("Distance to mate: %hhu\n", eval.depth - 1);
			else if (depth + bitDepth + bitBoard.moveCount >= 64 || eval.depth == 255)
				printf("Depth: Until the end of the game\n");
			else
				printf("Depth: %hhu + %hhu = %hhu\n", eval.depth, bitDepth, eval.depth + bitDepth);
			
			playMove(bitBoard, eval.column);

		}

		fancyPrintBoard(*decodeBoard(bitBoard));

	}

	if (is_win(bitBoard.playerBitboard[0]))
		printf("\nPlayer RED has won!!\n");

	else if (is_win(bitBoard.playerBitboard[1]))
		printf("\nPlayer YELLOW has won!!\n");

	else
		printf("\nThe game has ended in a draw\n");
}

void MachineAgainstMachine(const Connect4Board board, float Max_time, unsigned char bitDepth, bool clear_screen)
{
	Timer timer;

	printf("Initial Board:\n\n\n\n");

	fancyPrintBoard(board);

	Board bitBoard = *translateBoard(board);

	while (!is_win(bitBoard.playerBitboard[0]) && !is_win(bitBoard.playerBitboard[1]) && bitBoard.moveCount < 64)
	{

		timer.reset();
		SolveEval eval = evaluateBoardTime(bitBoard, Max_time, bitDepth);
		float time = timer.check();

		if(clear_screen)
			system("cls");

		printf("The code has chosen column %u after thinking for %.3fs \nIt evaluates the position as ", eval.column + 1, time);

		switch (eval.flag)
		{
		case CURRENT_PLAYER_WIN:
			printf("winning for %s.", bitBoard.sideToPlay ? "YELLOW" : "RED");
			break;
		case OTHER_PLAYER_WIN:
			printf("winning for %s.", bitBoard.sideToPlay ? "RED" : "YELLOW");
			break;
		case CURRENT_PLAYER_BETTER:
			printf("better for %s.", bitBoard.sideToPlay ? "YELLOW" : "RED");
			break;
		case OTHER_PLAYER_BETTER:
			printf("better for %s.", bitBoard.sideToPlay ? "RED" : "YELLOW");
			break;
		case DRAW:
			printf("a draw.");
			break;
		}

		printf("\nValue: %.3f \n", eval.eval);

		if (eval.eval == 1.f || eval.eval == -1.f)
			printf("Distance to mate: %hhu\n", eval.depth - 1);
		else if (eval.depth == 255)
			printf("Depth: Until the end of the game\n");
		else
			printf("Depth: %hhu + %hhu = %hhu\n", eval.depth, bitDepth, eval.depth + bitDepth);

		playMove(bitBoard, eval.column);

		fancyPrintBoard(*decodeBoard(bitBoard));

	}
}

float evaluatePosition(const Connect4Board& board, unsigned char depth, unsigned char bitDepth)
{
	Board* bitBoard = translateBoard(board);
	float result = evaluateBoard(*bitBoard, depth, bitDepth).eval;
	delete bitBoard;

	return result;
}

unsigned char* findBestMove(const Connect4Board& board)
{
	Board* bitBoard = translateBoard(board);
	return findBestPath(*bitBoard);
}