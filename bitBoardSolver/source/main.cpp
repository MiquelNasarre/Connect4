#include "Interface.h"
#include "Timer.h"
#include "testBoards.h"

#include <stdio.h>
#include <random>

int main()
{
    Connect4Board board; // (T2);
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

    playAgainstMachine(board, YELLOW, 8, 10, true);

#elif defined LET_PLAY

    srand(1451534);

    unsigned char winsh0 = 0;
    unsigned char winsh1 = 0;
    unsigned char draws = 0;

    for (unsigned int i = 0; i < 100; i++)
    {
        float time = rand() / 16384.f + 0.1f;
        unsigned char winner = MachineAgainstMachine(board, time, 8, 0, 1, true);

        if (winner == 0)
            winsh0++;
        else if (winner == 1)
            winsh1++;
        else
            draws++;

        winner = MachineAgainstMachine(board, time, 8, 1, 0, true);

        if (winner == 0)
            winsh1++;
        else if (winner == 1)
            winsh0++;
        else
            draws++;
    }

    printf("heuristic0 won %hhu times\n", winsh0);
    printf("heuristic1 won %hhu times\n", winsh1);
    printf("%hhu draws\n", draws);


#elif defined BEST_PATH
    Timer timer;
    timer.reset();
    unsigned char* p = findBestMove(board);
    printf("%hhu, %hhu, %.3f", p[0], p[1], timer.check());


#endif

    return 0;
}