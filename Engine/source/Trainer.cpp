#include "Timer.h"
#include "heuristicSolver.h"
#include "Trainer.h"

#include <stdio.h>
#include <stdlib.h>
#include <cmath>

/*
-------------------------------------------------------------------------------------------------------
Random Number Generators
-------------------------------------------------------------------------------------------------------
*/

// Mixes up the randomizer seed.
static inline unsigned long long& splitmix(unsigned long long& seed)
{
	seed += 0x9E3779B97F4A7C15ull;
	seed = (seed ^ (seed >> 30)) * 0xBF58476D1CE4E5B9ull;
	seed = (seed ^ (seed >> 27)) * 0x94D049BB133111EBull;
	seed ^= (seed >> 31);
	return seed;
}

// Generates random values with the specified normal distribution.
static inline float random_norm(float mean, float deviation)
{
	thread_local static unsigned long long seed = Timer::get_system_time_ns()
		^ (unsigned long long)(uintptr_t) & seed;

	float rand_1 = (splitmix(seed) >> 8) * (1.0f / 72057594037927936.0f); // Random value between 0 and 1
	float rand_2 = (splitmix(seed) >> 8) * (1.0f / 72057594037927936.0f); // Random value between 0 and 1

	float z = sqrtf(-2.0f * logf(rand_1)) * cosf(2.0f * 3.1415926535f * rand_2); // Normal distribution N(0,1) via Box-Muller

	return z * deviation + mean; // Normal distribution N(mean,std)
}

// Uses a randomizer to generate a shuffled set of integers from 0 to size.
static inline unsigned* shuffled_integers(unsigned size)
{
	thread_local static unsigned long long seed = Timer::get_system_time_ns()
		^ (unsigned long long)(uintptr_t) & seed;

	unsigned* out = (unsigned*)malloc(size * sizeof(unsigned));
	for (unsigned i = 0; i < size; ++i) out[i] = i;

	// Fisher–Yates
	for (unsigned i = size; i > 1; --i) {
		unsigned j = (unsigned)(splitmix(seed) % i); // 0..i-1
		unsigned tmp = out[i - 1];
		out[i - 1] = out[j];
		out[j] = tmp;
	}
	return out;
}

// Generates a random natural number between begin and end (included).
static inline unsigned random_unsigned(unsigned begin, unsigned end)
{
	thread_local static unsigned long long seed = Timer::get_system_time_ns()
		^ (unsigned long long)(uintptr_t) & seed;

	return splitmix(seed) % (end + 1 - begin) + begin;
}

// Generates a random number between 0 and 1.
static inline float random_0_1()
{
	thread_local static unsigned long long seed = Timer::get_system_time_ns()
		^ (unsigned long long)(uintptr_t) & seed;

	return (splitmix(seed) >> 8) * (1.0f / 72057594037927936.0f); // Random value between 0 and 1
}

/*
-------------------------------------------------------------------------------------------------------
Neural Network Generators and Modifiers
-------------------------------------------------------------------------------------------------------
*/

// Generates a set of Neural Networks copied from the parent Neural Network.
// For random search a random noise is applied to each weight with the specified deviation.

NeuralNetwork** Trainer::generate_inherited_NN(NeuralNetwork* parent, unsigned amount, float deviation)
{
	NeuralNetwork** sons = (NeuralNetwork**)calloc(amount, sizeof(NeuralNetwork*));
	for (unsigned i = 0; i < amount; i++)
	{
		sons[i] = new NeuralNetwork(*parent);
		add_random_noise(sons[i], deviation);
	}
	return sons;
}

// Generates a set of Neural Networks with weights initialized by a
// normally distributed random variable with the specified deviation.

NeuralNetwork** Trainer::generate_NN(unsigned amount, float deviation)
{
	NeuralNetwork** nns = (NeuralNetwork**)calloc(amount, sizeof(NeuralNetwork*));
	for (unsigned i = 0; i < amount; i++)
	{
		nns[i] = new NeuralNetwork();
		add_random_noise(nns[i], deviation);
	}
	return nns;
}

// Add a random noise to the Neural Network weights with the specified deviation.

void Trainer::add_random_noise(NeuralNetwork* nn, float deviation)
{
	for (unsigned n = 0; n < HIDDEN_LAYER_DIM; n++)
		for (unsigned i = 0; i < INPUT_DIM + 1; i++)
			nn->input_weights[n][i] += random_norm(0, deviation);

	for (unsigned n = 0; n < HIDDEN_LAYER_DIM; n++)
		for (unsigned i = 0; i < HIDDEN_LAYER_DIM + 1; i++)
			nn->hidden_layer_1_weights[n][i] += random_norm(0, deviation);

	for (unsigned n = 0; n < OUTPUT_DIM; n++)
		for (unsigned i = 0; i < HIDDEN_LAYER_DIM + 1; i++)
			nn->hidden_layer_2_weights[n][i] += random_norm(0, deviation);
}

// Sets the heuristic weights output to the default defined values.
// It does that by setting all the weights to the specific output 
// nodes to 0 and the biases to the default values.

void Trainer::set_default_heuristic_weights(NeuralNetwork* nn)
{
	float* weights_heu_favorables			= nn->hidden_layer_2_weights[0];
	float* weights_heu_favorables_1move		= nn->hidden_layer_2_weights[1];
	float* weights_heu_possibles			= nn->hidden_layer_2_weights[2];
	float* weights_heu_possibles_1move		= nn->hidden_layer_2_weights[3];

	for (unsigned node = 0; node < HIDDEN_LAYER_DIM; node++)
	{
		weights_heu_favorables[node]		= 0;
		weights_heu_favorables_1move[node]	= 0;
		weights_heu_possibles[node]			= 0;
		weights_heu_possibles_1move[node]	= 0;
	}

	weights_heu_favorables[HIDDEN_LAYER_DIM]		= WEIGHT_FAVORABLES;
	weights_heu_favorables_1move[HIDDEN_LAYER_DIM]	= WEIGHT_FAVORABLE_1MOVE;
	weights_heu_possibles[HIDDEN_LAYER_DIM]			= WEIGHT_POSSIBLES;
	weights_heu_possibles_1move[HIDDEN_LAYER_DIM]	= WEIGHT_POSSIBLES_1MOVE;
}

/*
-------------------------------------------------------------------------------------------------------
Training Related Functions
-------------------------------------------------------------------------------------------------------
*/

// Player versus player tournament that during multiple rounds randomly matches the players
// against each other for a set of games, the score of each player is based on an ELO system 
// updated through single game wins and losses.

void Trainer::play_survival_tournament(NeuralNetwork** NNs, float* scores, unsigned n_players, unsigned match_rounds, unsigned games_per_match)
{
	for (unsigned round = 0; round < match_rounds; round++)
	{
		unsigned* shuffled = shuffled_integers(n_players);

		for (unsigned match = 0; match < n_players / 2; match++)
		{
			NeuralNetwork* player1 = NNs[shuffled[2 * match]];
			NeuralNetwork* player2 = NNs[shuffled[2 * match + 1]];
			float& score1 = scores[shuffled[2 * match]];
			float& score2 = scores[shuffled[2 * match + 1]];

			for (unsigned game = 0; game < games_per_match; game++)
			{
				Board initial_board = {};
				playMove(initial_board, random_unsigned(0, 7));
				playMove(initial_board, random_unsigned(0, 7));

				float time_per_move = random_0_1() / 3.f;
				if (time_per_move < 0.05f) time_per_move = 0.05f;

				char winner = NNvsNN(time_per_move, initial_board, player1, player2, true, true);
				float y = 0.5f;
				switch (winner)
				{
				case 1:
					y = 0.f;
					break;
				case 2:
					y = 1.f;
					break;
				}
				float p = 1.f / (1.f + expf(-(score1 - score2) / 2.f));
				float g = y - p;
				score1 += g;
				score2 -= g;

				winner = NNvsNN(time_per_move, initial_board, player2, player1, true, true);
				y = 0.5f;
				switch (winner)
				{
				case 1:
					y = 0.f;
					break;
				case 2:
					y = 1.f;
					break;
				}
				p = 1.f / (1.f + expf(-(score2 - score1) / 2.f));
				g = y - p;
				score2 += g;
				score1 -= g;
			}
		}
		free(shuffled);
	}
}

// Game between two different engines using the specified schedulers, given a certain time per move 
// and an initial board. It returns 0 if it is a draw, 1 if nn_1 wins and 2 if nn_2 wins.

unsigned char Trainer::NNvsNN(float sec_per_move, Board board, NeuralNetwork* nn_1, NeuralNetwork* nn_2, bool print_boards, bool clear_console)
{
	EngineConnect4 player1(&board, nn_1);
	EngineConnect4 player2(&board, nn_2);

	while (board.moveCount < 64)
	{
		player1.update_position(&board);
		player2.update_position(&board);

		PositionEval eval = {};
		if (board.sideToPlay)
			eval = player2.evaluate_for(sec_per_move);
		else 
			eval = player1.evaluate_for(sec_per_move);

		if (print_boards)
		{
			if (clear_console)
				system("cls");
			printf("The code has chosen column %u\nIt evaluates the position as ", eval.column + 1);

			switch (eval.flag)
			{
			case CURRENT_PLAYER_WIN:
				printf("winning for %s.", board.sideToPlay == 1 ? "YELLOW" : "RED");
				break;
			case OTHER_PLAYER_WIN:
				printf("winning for %s.", board.sideToPlay == 1 ? "RED" : "YELLOW");
				break;
			case CURRENT_PLAYER_BETTER:
				printf("better for %s.", board.sideToPlay == 1 ? "YELLOW" : "RED");
				break;
			case OTHER_PLAYER_BETTER:
				printf("better for %s.", board.sideToPlay == 1 ? "RED" : "YELLOW");
				break;
			case DRAW:
				printf("a draw.");
				break;
			}

			printf("\nValue: %.3f \n", eval.eval);

			if (eval.eval == 1.f || eval.eval == -1.f)
			{
				if (eval.depth != 255)
					printf("Distance to mate: %hhu\n", eval.depth - 1);
				else
					printf("Distance to mate: Unknown");
			}
			else if (eval.depth + board.moveCount >= 64 || eval.depth == 255)
				printf("Depth: Until the end of the game\n");
			else
				printf("Depth: %hhu\n", eval.depth);
		}

		playMove(board, eval.column);

		if(print_boards)
			EngineConnect4::decodeBoard(board).console_fancy_print();

		if (is_win(board.playerBitboard[board.sideToPlay ^ 1]))
			return board.sideToPlay ? 1 : 2;
	}

	return 0;
}

// The specified amount of games are played by an engine using the default scheduler,
// and during the games the different NNs are attached to an engine and asked to evaluate 
// a the current position with a random amount of time. The scores ara based on the
// heuristic depth, tail depth and exact depth of the transposition table entries generated.

void Trainer::deeper_search_tournament(NeuralNetwork* default_scheduler, NeuralNetwork** NNs, float* scores, unsigned n_players, unsigned games)
{
	init_zobrist();
	for (unsigned game = 0; game < games; game++)
	{
		Board board = {};
		playMove(board, random_unsigned(0, 7));
		playMove(board, random_unsigned(0, 7));

		EngineConnect4 scheduler(&board, default_scheduler);

		unsigned max_moves = 31;
		while (board.moveCount < 64 && max_moves--)
		{
			printf("Current position to evaluate:\n");
			EngineConnect4::decodeBoard(board).console_fancy_print();

			float time_per_move = fabsf(random_norm(0.f, 0.5f));
			if (time_per_move < 0.025f) time_per_move = 0.025f;

			if (max_moves % 5 == 0) 
			{
				{
					printf("Time per eval: %.3fs\n", time_per_move);
					EngineConnect4 player(&board, default_scheduler);
					player.evaluate_for(time_per_move);
					HTTEntry* entry = player.get_entry(&board);
					TTEntry* exact_entry = player.get_exact_entry(&board);
					uint8_t exact_depth = 0;
					if (exact_entry)
						exact_depth = exact_entry->depth;

					if (!entry || entry->flag != ENTRY_FLAG_EXACT)
						printf("Default did not manage to get a single tree done.\n\n");
					else if (entry->eval == 1.f || entry->eval == -1.f)
						printf("Default solved the board.\n\n");
					else
						printf("Default evaluated to depths (%hhu, %hhu, %hhu)\n\n", entry->heuDepth, entry->bitDepth, exact_depth);
				}
				for (unsigned n = 0; n < n_players; n++)
				{
					EngineConnect4 player(&board, NNs[n]);
					player.evaluate_for(time_per_move);
					HTTEntry* entry = player.get_entry(&board);
					TTEntry* exact_entry = player.get_exact_entry(&board);
					uint8_t exact_depth = 0;
					if (exact_entry)
						exact_depth = exact_entry->depth;

					if (!entry || entry->flag != ENTRY_FLAG_EXACT)
					{
						printf("Player %u did not manage to get a single tree done. Score -50.00\n", n);
						scores[n] += -50.0f / (2.f + sqrtf(board.moveCount));
					}
					else if (entry->eval == 1.f || entry->eval == -1.f || entry->bitDepth + entry->heuDepth + board.moveCount == 64)
					{
						printf("Player %u solved the board. Score +%.2f\n", n, 30.f * (1.f + sqrtf(exact_depth)));
						scores[n] += 30.f * (1.f + sqrtf(exact_depth)) / (2.f + sqrtf(board.moveCount));
					}
					else
					{
						float score = (1.f + sqrtf(entry->heuDepth)) * (1.f + sqrtf(entry->bitDepth)) * (1.f + sqrtf(exact_depth));
						printf("Player %u evaluated to depths (%hhu, %hhu, %hhu). Score +%.2f\n", n, entry->heuDepth, entry->bitDepth, exact_depth, score);

						scores[n] += score / (2.f + sqrtf(board.moveCount));
					}
				}
			}
			else Timer::sleep_for(1000UL);

			PositionEval eval = scheduler.get_evaluation();
			playMove(board, eval.column);
			if (scheduler.update_position(&board))
			{
				printf("\nPosition updated correctly.\n");
			}
			else
			{
				printf("Error updating the board on the scheduler.\n");
				return;
			}

			if (eval.eval == 1.f || eval.eval == -1.f)
			{
				printf("Board has been solved by the scheduler.\n");
				break;
			}

			if (is_win(board.playerBitboard[board.sideToPlay ^ 1]))
				break;
		}

		printf("Game %u has ended.\n", game);
	}
	printf("Tournament has ended.\n");
}

// Main function of the class, it takes the last session champions stored in last_session storage,
// multiplies them to fit the training size, adding radom noise to the weights. Performs the 
// specified tournament and wtores the survivors to the session storage folder.

void Trainer::training_session(const char* last_session_storage, const char* this_session_storage, unsigned last_session_survival, unsigned this_session_size, unsigned survival_ranking, float deviation, Training_Type type, bool default_heuristic)
{
	if (last_session_storage && this_session_size % last_session_survival)
	{
		printf("This session size must be divisible by last session survival");
		return;
	}
	if (this_session_size % survival_ranking)
	{
		printf("This session size must be divisible by survival ranking");
		return;
	}

	char filename[12] = "weights0000";

	NeuralNetwork** stored_nns = (NeuralNetwork**)calloc(this_session_size, sizeof(NeuralNetwork*));
	float* rewards = (float*)calloc(this_session_size, sizeof(float));

	if (last_session_storage)
	{
		char file_path[512] = {};
		int j = -1;
		while (this_session_storage[++j]) file_path[j] = this_session_storage[j];

		unsigned factor = this_session_size / last_session_survival;

		for (unsigned i = 0; i < last_session_survival; i++)
		{
			filename[10] = '0' + i % 10;
			filename[9] = '0' + (i / 10) % 10;
			filename[8] = '0' + (i / 100) % 10;
			filename[7] = '0' + (i / 1000) % 10;

			int k = -1;
			while (filename[++k]) file_path[j + k] = filename[k];
			file_path[j + k] = '\0';

			stored_nns[i * factor] = new NeuralNetwork(file_path);
		
			NeuralNetwork** sons = generate_inherited_NN(stored_nns[i * factor], factor - 1, deviation);

			for (unsigned n = 1; n < factor; n++)
				stored_nns[i * factor + n] = sons[n - 1];

			free(sons);
		}
	}
	else
	{
		free(stored_nns);
		stored_nns = generate_NN(this_session_size, DEFAULT_INIT_DEVIATION);
	}

	if (default_heuristic) for (unsigned i = 0; i < this_session_size; i++)
		set_default_heuristic_weights(stored_nns[i]);

	// Here starts the survival tournament

	switch (type)
	{
	case WIN_RATE:
		play_survival_tournament(stored_nns, rewards, this_session_size, DEFAULT_MATCH_ROUNDS, DEFAULT_GAMES_PER_MATCH);
		break;
	case DEEP_SEARCH:
		deeper_search_tournament(nullptr, stored_nns, rewards, this_session_size, 1);
		break;
	}

	// Here ends the survival epoch and NNs are ranked

	for (unsigned i = 0; i < survival_ranking; i++)
	{
		unsigned best_id = i;
		for (unsigned j = i + 1; j < this_session_size; j++)
			if (rewards[j] > rewards[best_id]) best_id = j;

		NeuralNetwork* aux_nn = stored_nns[i];
		float reward_aux = rewards[i];

		stored_nns[i] = stored_nns[best_id];
		rewards[i] = rewards[best_id];
		
		stored_nns[best_id] = aux_nn;
		rewards[best_id] = reward_aux;
	}

	// Here the final rankings are printed

	printf("Tournament Winners:\n");
	for (unsigned i = 0; i < survival_ranking; i++)
		printf("#%u with score %.2f\n", i, rewards[i]);

	// Here the best NNs are saved

	char file_path[512] = {};
	int j = -1;
	while (this_session_storage[++j]) file_path[j] = this_session_storage[j];

	for (unsigned i = 0; i < survival_ranking; i++)
	{
		filename[10] = '0' + i % 10;
		filename[9] = '0' + (i / 10) % 10;
		filename[8] = '0' + (i / 100) % 10;
		filename[7] = '0' + (i / 1000) % 10;

		int k = -1;
		while (filename[++k]) file_path[j + k] = filename[k];
		file_path[j + k] = '\0';

		stored_nns[i]->store_weights(file_path);
	}

	// Cleanup and return

	for (unsigned i = 0; i < this_session_size; i++)
		delete stored_nns[i];

	free(stored_nns);
	free(rewards);
}
