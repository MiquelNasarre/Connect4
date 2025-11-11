#include "Interface.h"
#include "testBoards.h"
#include "Trainer.h"

int main()
{
	while(true) Trainer::training_session("training_session/", "training_session/", 8, 64, 8, 0.015f, DEEP_SEARCH, true);

	//Trainer::training_session("training_session/", "training_session/", 8, 64, 8, 0.1f);
	//Trainer::training_session("training_session/", "training_session/", 8, 64, 8, 0.1f);
	//Trainer::training_session("training_session/", "training_session/", 8, 64, 8, 0.1f);
	//Trainer::training_session("training_session/", "training_session/", 8, 64, 8, 0.1f);
	//Trainer::training_session("training_session/", "training_session/", 8, 64, 8, 0.1f);
	//Trainer::training_session("training_session/", "training_session/", 8, 64, 8, 0.1f);
	//Trainer::training_session("training_session/", "training_session/", 8, 64, 8, 0.1f);
	//Trainer::training_session("training_session/", "training_session/", 8, 64, 8, 0.1f);
	return 0;

	for (;;)
		engineAgainstEngine(1.25f, Connect4());

	playAgainstEngine(1.f, Connect4(T, 1), 1);
	playAgainstEngine(1.f, Connect4(T, 1), 2);
	return 0;
}