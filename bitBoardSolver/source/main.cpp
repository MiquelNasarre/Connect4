#include "Interface.h"
#include "Timer.h"
#include "testBoards.h"

#include <stdio.h>

int main()
{
    Connect4Board board; // (T0);
    //board.currentPlayer = YELLOW;

	printf("Current board:\n");
	printBoard(board);

//#define PLAY
#ifndef PLAY

    unsigned int repetitions = 10000;
    unsigned char depth = 10;

    Timer timer;
	timer.reset();

    for (unsigned int i = 0; i < repetitions - 1; i++)
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

	printf("Solved %u times at depth %u in %.3fs\n", repetitions, depth, time);
	return 0;

#else
    playAgainstMachine(board, YELLOW, 20);
    return 0;
#endif
}