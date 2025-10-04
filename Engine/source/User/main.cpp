#include "Interface.h"
#include "testBoards.h"

#include <random>


int main()
{
	unsigned int red = 0;
	unsigned int yellow = 0;
	unsigned int draw = 0;

	srand(649473878);

	engineAgainstEngine(0.05f, Connect4(T11, 1));

	for (unsigned int i = 0; i < 300; i++)
	{
		char w = engineAgainstEngine(rand() / 8192.f + 0.05f, Connect4(), true);
		if (w == 0)
			red++;
		else if (w == 1)
			yellow++;
		else
			draw++;
	}

	printf("\n\nRed %u Yellow %u Draw %u", red, yellow, draw);

	return 0;
}