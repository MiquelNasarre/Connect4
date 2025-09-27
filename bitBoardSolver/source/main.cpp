#include "Interface.h"
#include "Timer.h"
#include "testBoards.h"

#include <stdio.h>

int main()
{
    Connect4Board board;// (T2);
    //board.currentPlayer = YELLOW;

//#define BIT_TREE
#define EVAL_TREE
//#define PLAY
//#define LET_PLAY

#ifdef BIT_TREE

    printf("Current board:\n");
    fancyPrintBoard(board);

    unsigned int repetitions = 1;
    unsigned char depth = 7;

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

#elif defined EVAL_TREE

    printf("Current board:\n");
    fancyPrintBoard(board);

    Timer timer;
    timer.reset();

    constexpr unsigned int repetitions = 1;
    constexpr unsigned char depth = 12;
    constexpr unsigned char bitDepth = 10;

    for (unsigned int i = 0; i < repetitions - 1; i++)
        evaluatePosition(board, depth, bitDepth);

    float evaluation = evaluatePosition(board, depth, bitDepth);

    float time = timer.check();

    printf("Board evaluation: %.3f\n\n", evaluation);

    printf("Solved %u times at depths %u + %u = %u in %.3fs\n", repetitions, depth, bitDepth, depth + bitDepth, time);

#elif defined PLAY

    playAgainstMachine(board, YELLOW, 14, 6, true);

#elif defined LET_PLAY

    MachineAgainstMachine(board, 20.f, 14, false);

#endif

    return 0;
}