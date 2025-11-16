#include "Interface.h"
#include "testBoards.h"
#include "Trainer.h"
#include "Engine_NN.h"

#include "rng.h"
#include <cmath>
#include <cstdio>

int main()
{
	//engineAgainstEngine(2.0f, Connect4());
	//return 0;
	
	//NeuralNetwork best_nn("training_session/weights0000");
	//
	//unsigned wins_default = 0;
	//unsigned wins_nn = 0;
	//unsigned draws = 0;
	//
	//for (unsigned game = 0; game < 128; game++)
	//{
	//	Board board = {};
	//	unsigned rand_moves = random_unsigned(0, 2);
	//	for (unsigned move = 0; move < rand_moves; move++)
	//		playMove(board, random_unsigned(0, 7));
	//
	//	float time_per_move = fabsf(random_norm(0.f, 0.25f)) + 0.05f;
	//
	//	unsigned char winner = Trainer::NNvsNN(time_per_move, true, board, (NeuralNetwork*)nullptr, &best_nn, false, false);
	//
	//	printf("Game %u.1 ", game + 1);
	//	switch (winner)
	//	{
	//	case 0:
	//		printf("ended in a draw.\n");
	//		draws++;
	//		break;
	//	case 1:
	//		printf("won by default scheduler.\n");
	//		wins_default++;
	//		break;
	//	case 2:
	//		printf("won by neural network.\n");
	//		wins_nn++;
	//		break;
	//	}
	//
	//	winner = Trainer::NNvsNN(time_per_move, true, board, &best_nn, (NeuralNetwork*)nullptr, false, false);
	//
	//	printf("Game %u.2 ", game + 1);
	//	switch (winner)
	//	{
	//	case 0:
	//		printf("ended in a draw.\n");
	//		draws++;
	//		break;
	//	case 2:
	//		printf("won by default scheduler.\n");
	//		wins_default++;
	//		break;
	//	case 1:
	//		printf("won by neural network.\n");
	//		wins_nn++;
	//		break;
	//	}
	//}
	//
	//printf("\nFINAL STANDINGS:\nDefault scheduler wins %u\nNeural Network wins %u\nDraws %u\n", wins_default, wins_nn, draws);
	//
	//return 0;


	// DEPTH TOURNAMENT

	//while (true)
	//	Trainer::training_session("training_session/", "training_session/", 16, 64, 16, 0.025f, DEEP_SEARCH, DEPTHS, false);

	// ELO TOURNAMENT
	
	while (true)
		Trainer::training_session("training_session/", "training_session/", 16, 64, 16, 0.01f, WIN_RATE, ALL, false);

	return 0;
}