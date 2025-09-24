#include "Interface.h"
#include "Timer.h"
#include "testBoards.h"

#include <stdio.h>

int main()
{
    Connect4Board board; // (T0);
    //board.currentPlayer = YELLOW;

    //playAgainstMachine(board, YELLOW, 20);
    //return 0;

	printf("Current board:\n");
	printBoard(board);

    unsigned int repetitions = 10000;
    unsigned char depth = 10;

    Timer timer;
	timer.reset();

//#define EVAL

#ifndef EVAL

    for(unsigned int i = 0; i < repetitions - 1; i++)
        solveConnect4(board, depth);

	Player player = solveConnect4(board, depth);

	float time = timer.check();

    switch (player)
    {
    case INVALID:
        printf("Invalid board position\n");
        break;
    case NONE:
        printf("Draw or unresolved position\n");
        break;
    case RED:
        printf("Red can force a win\n");
        break;
    case YELLOW:
        printf("Yellow can force a win\n");
        break;
    }

#else

    for (unsigned int i = 0; i < repetitions - 1; i++)
        decideNextMove(board,depth);

    Eval eval = decideNextMove(board, depth);
    float time = timer.check();

    switch (eval.winner)
    {
    case INVALID:
        printf("Invalid board position\n");
        break;
    case NONE:
        printf("Draw or unresolved position if %s plays at column %u\n", (board.currentPlayer == RED) ? "RED" : "YELLOw", eval.column);
        break;
    case RED:
		printf("Red can force a win %s%u\n", (board.currentPlayer == RED) ? "if it plays at column " : "", eval.column);
        break;
    case YELLOW:
		printf("Yellow can force a win %s%u\n", (board.currentPlayer == YELLOW) ? "if it plays at column " : "", eval.column);
        break;
    }

#endif

	printf("Solved %u times at depth %u in %.3fs\n", repetitions, depth, time);

	return 0;
}