#include "heuristictt.h"
#include "Interface.h"
#include "testBoards.h"
#include "Trainer.h"
#include "Engine_NN.h"
#include "bitSolver.h"

#include "rng.h"
#include <cmath>
#include <cstdio>

int main()
{
	// ENGINE LIVE MATCH

	//engineAgainstEngine(5.0f, Connect4());
	//return 0;
	
	// NN vs DEFAULT
	
	// CURRENT BEST STATS (0.50s std, 0.05s min)
	// default  70
	// NN	    183
	// draws	3

	/*NeuralNetwork* nn1 = new NeuralNetwork("training_session/weights0000");
	NeuralNetwork* nn2 = new NeuralNetwork("scheduler");
	
	unsigned wins_default = 0;
	unsigned wins_nn = 0;
	unsigned draws = 0;
	
	for (unsigned game = 0; game < 128; game++)
	{
		Board board = {};
		unsigned rand_moves = rng::random_unsigned(0, 2);
		for (unsigned move = 0; move < rand_moves; move++)
			playMove(board, rng::random_unsigned(0, 7));
	
		float time_per_move = fabsf(rng::random_norm(0.f, 0.50f));
		if (time_per_move < 0.05f) time_per_move = 0.05f;
	
		unsigned char winner = Trainer::NNvsNN(time_per_move, true, board, nn2, nn1, false, false);
	
		printf("Game %u.1 ", game + 1);
		switch (winner)
		{
		case 0:
			printf("ended in a draw.\n");
			draws++;
			break;
		case 1:
			printf("won by default scheduler.\n");
			wins_default++;
			break;
		case 2:
			printf("won by neural network.\n");
			wins_nn++;
			break;
		}
	
		winner = Trainer::NNvsNN(time_per_move, true, board, nn1, nn2, false, false);
	
		printf("Game %u.2 ", game + 1);
		switch (winner)
		{
		case 0:
			printf("ended in a draw.\n");
			draws++;
			break;
		case 2:
			printf("won by default scheduler.\n");
			wins_default++;
			break;
		case 1:
			printf("won by neural network.\n");
			wins_nn++;
			break;
		}
	}
	
	printf("\nFINAL STANDINGS:\nDefault scheduler wins %u\nNeural Network wins %u\nDraws %u\n", wins_default, wins_nn, draws);
	
	return 0;*/

	// DEPTH TOURNAMENT

	//while (true)
	//	Trainer::training_session("training_session/", "training_session/", 16, 64, 16, 0.025f, DEEP_SEARCH, DEPTHS, false);

	// ELO TOURNAMENT
	
	unsigned epoch = 0;
	Timer timer;
	while (true)
	{
		Trainer::training_session("training_session/", "training_session/", 16, 64, 16, 0.01f, Training_Type::WIN_RATE, Training_Weights::ALL, false);
		float time = timer.mark();
		printf("\nEPOCH %u DONE in %.2fs\nAverage time per epoch: %.2fs\n", ++epoch, time, timer.average());
	}

	return 0;
}