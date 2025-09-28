#include "Interface.h"
#include "Timer.h"
#include "testBoards.h"

#include <stdio.h>

int main()
{
    Connect4Board board;// (T6);
    //board.currentPlayer = YELLOW;

//#define BIT_TREE
//#define EVAL_TREE
//#define PLAY
#define LET_PLAY
//#define BEST_PATH

#ifdef BIT_TREE

    printf("Current board:\n");
    fancyPrintBoard(board);

    unsigned int repetitions = 1;
    unsigned char depth = 28;

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

    constexpr unsigned char depth = 12;
    constexpr unsigned char bitDepth = 10;

    float evaluation = evaluatePosition(board, depth, bitDepth);

    float time = timer.check();

    printf("Board evaluation: %.3f\n\n", evaluation);

    printf("Solved at depths %u + %u = %u in %.3fs\n", depth, bitDepth, depth + bitDepth, time);

#elif defined PLAY

    playAgainstMachine(board, YELLOW, 14, 6, true);

#elif defined LET_PLAY

    MachineAgainstMachine(board, 0.5f, 8, false);

#elif defined BEST_PATH

    unsigned char* p = findBestMove(board);
    printf("%hhu, %hhu", p[0], p[1]);

#endif

    return 0;
}