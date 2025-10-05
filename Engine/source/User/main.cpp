#include "Interface.h"
#include "testBoards.h"

int main()
{
	playAgainstEngine(1.f, Connect4(T, 1), 1, true);
	playAgainstEngine(1.f, Connect4(T, 1), 2, true);
	return 0;
}