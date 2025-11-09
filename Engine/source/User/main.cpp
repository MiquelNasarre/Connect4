#include "Interface.h"
#include "testBoards.h"

int main()
{
	for (;;)
		engineAgainstEngine(0.25f, Connect4());

	playAgainstEngine(1.f, Connect4(T, 1), 1);
	playAgainstEngine(1.f, Connect4(T, 1), 2);
	return 0;
}