#include "Trainer.h"

#ifdef _TRAINING
#include "heuristicSolver.h"
#include "Engine.h"
#include "Thread.h"
#include "rng.h"

#include <stdio.h>
#include <stdlib.h>
#include <cmath>

/*
-------------------------------------------------------------------------------------------------------
Arbitratry Training Values
-------------------------------------------------------------------------------------------------------
*/

// Global
#define MOMENTUM								0.85f
#define DEFAULT_WEIGHT_DECAY					0.01f
#define ARBITRARY_MAX_RANDOM_MOVES				2
#define ARBITRARY_TIME_DEVIATION				0.50f // 1.50f
#define ARBITRARY_MIN_TIME_PER_MOVE				0.05f
#define DEFAULT_INIT_DEVIATION					sqrtf(2.f/INPUT_DIM)

// ELO tournament
#define ARBITRARY_INITIAL_ELO					1000.f
#define DEFAULT_K_FACTOR						16.f
#define DEFAULT_MATCH_ROUNDS					4
#define DEFAULT_GAMES_PER_MATCH					1
#define ARBITRARY_ELO_DIV						2.f

// Deep Tournament
#define BATCH_SIZE								4
#define DEFAULT_N_GAMES							1 //4
#define ARBITRARY_SCHEDULER_TIME				0.25f
#define ARBITRARY_SKIP_CHANCE					0.66f
#define ARBITRARY_NO_ENTRY_SCORE				-250.f
#define ARBITRARY_SOLVED_BOARD_SCORE			200.f
#define ARBITRARY_SCORE_DIVIDER					(2.f + sqrtf(board.moveCount))
#define ARBITRARY_CONSERVATISM_DEVIATION		0.20f

/*
-------------------------------------------------------------------------------------------------------
Neural Network Generators and Modifiers
-------------------------------------------------------------------------------------------------------
*/

// Generates a set of Neural Networks copied from the parent Neural Network.
// For random search a random noise is applied to each weight with the specified deviation.

NeuralNetwork** Trainer::generate_inherited_NN(NeuralNetwork* parent, unsigned amount, Training_Weights weights, float deviation)
{
	NeuralNetwork** sons = (NeuralNetwork**)calloc(amount, sizeof(NeuralNetwork*));
	for (unsigned i = 0; i < amount; i++)
	{
		sons[i] = new NeuralNetwork(*parent);
		do_weight_decay(sons[i], weights, DEFAULT_WEIGHT_DECAY);
		add_random_noise(sons[i], weights, deviation);
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
		add_random_noise(nns[i], ALL, deviation);
	}
	return nns;
}

// Add a random noise to the Neural Network weights with the specified deviation.

void Trainer::add_random_noise(NeuralNetwork* nn, Training_Weights weights, float deviation)
{
	switch (weights)
	{
	case HEURISTIC:
		for (unsigned out = 0; out < 4; out++)
			for (unsigned node = 0; node < HIDDEN_LAYER_DIM + 1; node++)
			{
				nn->_grad_hidden_layer_weights[out][node] = nn->_grad_hidden_layer_weights[out][node] * MOMENTUM + rng::random_norm(0.f, deviation) * (1.f - MOMENTUM);
				nn->hidden_layer_weights[out][node] += nn->_grad_hidden_layer_weights[out][node];
			}
		break;

	case DEPTHS:
		for (unsigned out = 4; out < 8; out++)
			for (unsigned node = 0; node < HIDDEN_LAYER_DIM + 1; node++)
			{
				nn->_grad_hidden_layer_weights[out][node] = nn->_grad_hidden_layer_weights[out][node] * MOMENTUM + rng::random_norm(0.f, deviation) * (1.f - MOMENTUM);
				nn->hidden_layer_weights[out][node] += nn->_grad_hidden_layer_weights[out][node];
			}
		break;

	case ALL:
	default:
		for (unsigned n = 0; n < HIDDEN_LAYER_DIM; n++)
			for (unsigned i = 0; i < INPUT_DIM + 1; i++)
			{
				nn->_grad_input_weights[n][i] = nn->_grad_input_weights[n][i] * MOMENTUM + rng::random_norm(0.f, deviation) * (1.f - MOMENTUM);
				nn->input_weights[n][i] += nn->_grad_input_weights[n][i];
			}
		for (unsigned n = 0; n < OUTPUT_DIM; n++)
			for (unsigned i = 0; i < HIDDEN_LAYER_DIM + 1; i++)
			{
				nn->_grad_hidden_layer_weights[n][i] = nn->_grad_hidden_layer_weights[n][i] * MOMENTUM + rng::random_norm(0.f, deviation) * (1.f - MOMENTUM);
				nn->hidden_layer_weights[n][i] += nn->_grad_hidden_layer_weights[n][i];
			}
		break;
	}
}

// Sets the heuristic weights output to the default defined values.
// It does that by setting all the weights to the specific output 
// nodes to 0 and the biases to the default values.

void Trainer::set_default_heuristic_weights(NeuralNetwork* nn)
{
	float* weights_heu_favorables			= nn->hidden_layer_weights[0];
	float* weights_heu_favorables_1move		= nn->hidden_layer_weights[1];
	float* weights_heu_possibles			= nn->hidden_layer_weights[2];
	float* weights_heu_possibles_1move		= nn->hidden_layer_weights[3];

	for (unsigned node = 0; node < HIDDEN_LAYER_DIM; node++)
	{
		weights_heu_favorables[node]		= 0.f;
		weights_heu_favorables_1move[node]	= 0.f;
		weights_heu_possibles[node]			= 0.f;
		weights_heu_possibles_1move[node]	= 0.f;
	}

	weights_heu_favorables[HIDDEN_LAYER_DIM]		= WEIGHT_FAVORABLES			/ POINT_DISTANCE;
	weights_heu_favorables_1move[HIDDEN_LAYER_DIM]	= WEIGHT_FAVORABLE_1MOVE	/ POINT_DISTANCE;
	weights_heu_possibles[HIDDEN_LAYER_DIM]			= WEIGHT_POSSIBLES			/ POINT_DISTANCE;
	weights_heu_possibles_1move[HIDDEN_LAYER_DIM]	= WEIGHT_POSSIBLES_1MOVE	/ POINT_DISTANCE;
}

// Degrades the weights by a factor to keep them from exploding.

void Trainer::do_weight_decay(NeuralNetwork* nn, Training_Weights weights, float lambda)
{
	switch (weights)
	{
	case HEURISTIC:
		for (unsigned out = 0; out < 4; out++)
			for (unsigned in = 0; in < HIDDEN_LAYER_DIM + 1; in++)
				nn->hidden_layer_weights[out][in] *= 1.f - lambda;
		break;
		
	case DEPTHS:
		for (unsigned out = 4; out < 8; out++)
			for (unsigned in = 0; in < HIDDEN_LAYER_DIM + 1; in++)
				nn->hidden_layer_weights[out][in] *= 1.f - lambda;
		break;

	case ALL:
	default:
		for (unsigned out = 0; out < HIDDEN_LAYER_DIM; out++)
			for (unsigned in = 0; in < INPUT_DIM + 1; in++)
				nn->input_weights[out][in] *= 1.f - lambda;

		for (unsigned out = 0; out < OUTPUT_DIM; out++)
			for (unsigned in = 0; in < HIDDEN_LAYER_DIM + 1; in++)
				nn->hidden_layer_weights[out][in] *= 1.f - lambda;
		break;
	}
}

/*
-------------------------------------------------------------------------------------------------------
Training Related Functions
-------------------------------------------------------------------------------------------------------
*/

// Helper to compute the ELO exhange between two players after a game. Returns the gain for player 1.

static inline float compute_elo_diff(float player1_elo, float player2_elo, char game_result, float K_factor)
{
	float Expectation_player1 = 1.f / (1.f + powf(10.f, (player2_elo - player1_elo) / 400.f));

	float Score_player1;
	switch (game_result)
	{
	case 1:
		Score_player1 = 1.0f;
		break;
	case 2:
		Score_player1 = 0.0f;
		break;
	case 0:
	default:
		Score_player1 = 0.5f;
		break;
	}

	float elo_diff = K_factor * (Score_player1 - Expectation_player1);
	return elo_diff;
}

// Helper function to thread games in tournament.

static inline void threaded_match(float sec_per_move, bool see_time_left, Board board, NeuralNetwork* nn_1, NeuralNetwork* nn_2, unsigned char* result)
{
	unsigned char winner = Trainer::NNvsNN(sec_per_move, see_time_left, board, nn_1, nn_2, false, false);
	*result = winner;
}

// Player versus player tournament that during multiple rounds randomly matches the players
// against each other for a set of games, the score of each player is based on an ELO system 
// updated through single game wins and losses.

void Trainer::elo_games_tournament(NeuralNetwork** NNs, float* scores, unsigned n_players, unsigned match_rounds, unsigned games_per_match)
{
		printf("\nTOURNAMENT BEGINS\n");
		printf("PLAYERS %u, ROUNDS %u, MATCHES %u, GAMES %u\n", n_players, match_rounds, match_rounds * n_players / 2, match_rounds * n_players / 2 * games_per_match);
		printf("Average time per game %.2fs\n", ARBITRARY_TIME_DEVIATION * 32.f);
		printf("Expected Tournament Time %.2fs\n", ARBITRARY_TIME_DEVIATION * 32.f * match_rounds * n_players / 2 * games_per_match);

		Thread arena[2] = { Thread(), Thread() };

	for (unsigned round = 0; round < match_rounds; round++)
	{
		printf("\nROUND %u/%u\n", round + 1, match_rounds);
		unsigned* shuffled = rng::shuffled_integers(n_players);

		for (unsigned match = 0; match < n_players / 2; match++)
		{
			const unsigned& p1 = shuffled[2 * match];
			const unsigned& p2 = shuffled[2 * match + 1];
			NeuralNetwork* player1 = NNs[p1];
			NeuralNetwork* player2 = NNs[p2];
			float& score1 = scores[p1];
			float& score2 = scores[p2];

			printf("\nMatch %u/%u will begin:\n", match + 1, n_players / 2);
			printf("Player #%02u ELO %.3f\n", p1, score1);
			printf("Player #%02u ELO %.3f\n", p2, score2);


			for (unsigned game = 0; game < games_per_match; game++)
			{
				Board initial_board = {};
				unsigned n_random_moves = rng::random_unsigned(0, ARBITRARY_MAX_RANDOM_MOVES);
				for (unsigned move = 0; move < n_random_moves; move++)
					playMove(initial_board, rng::random_unsigned(0, 7));

				float time_per_move = fabsf(rng::random_norm(0.f, ARBITRARY_TIME_DEVIATION));
				if (time_per_move < ARBITRARY_MIN_TIME_PER_MOVE) time_per_move = ARBITRARY_MIN_TIME_PER_MOVE;
				bool see_time_left = bool(rng::random_unsigned(0, 2));

				printf("Game %u/%u starts: Time per move %.3f (%s)\n", game + 1, games_per_match, time_per_move, see_time_left ? "known" : "unknown");

				unsigned char game_result1;
				unsigned char game_result2;

				if (arena[0].start(&threaded_match, time_per_move, see_time_left, initial_board, player1, player2, &game_result1))
				{
					arena[0].set_priority(Thread::PRIORITY_ABOVE_NORMAL);
					arena[0].set_name(L"Game %u.1: Player #%02u vs Player #%02u", game + 1, p1, p2);
				} else throw("ERROR: Unable to start game thread");
				
				if (arena[1].start(&threaded_match, time_per_move, see_time_left, initial_board, player2, player1, &game_result2))
				{
					arena[1].set_priority(Thread::PRIORITY_ABOVE_NORMAL);
					arena[1].set_name(L"Game %02u.2: Player #%u vs Player #%02u", game + 1, p2, p1);
				} else throw("ERROR: Unable to start game thread");

				arena[0].join(), arena[1].join();

				float g1 = compute_elo_diff(score1, score2, game_result1, DEFAULT_K_FACTOR);
				float g2 = compute_elo_diff(score2, score1, game_result2, DEFAULT_K_FACTOR);

				score1 += g1 - g2;
				score2 += g2 - g1;
				
				if(game_result1)
					printf("Game %u.1 won by player #%02u, ELO exchanged +%.2f\n", game + 1, game_result1 == 1 ? p1 : p2, fabsf(g1));
				else
					printf("Game %u.1 ended in a draw, ELO exchanged +%.2f\n", game + 1, fabsf(g1));

				if (game_result2)
					printf("Game %u.2 won by player #%02u, ELO exchanged +%.2f\n", game + 1, game_result2 == 2 ? p1 : p2, fabsf(g2));
				else
					printf("Game %u.2 ended in a draw, ELO exchanged +%.2f\n", game + 1, fabsf(g2));
			}
		}
		free(shuffled);
		printf("\nROUND %u ENDED\nCurrent Standigs:\n", round + 1);
		for (unsigned p = 0; p < n_players; p++)
			printf("Player #%02u ELO %.3f\n", p, scores[p]);
	}
}

// Game between two different engines using the specified schedulers, given a certain time per move 
// and an initial board. It returns 0 if it is a draw, 1 if nn_1 wins and 2 if nn_2 wins.

inline unsigned char Trainer::NNvsNN(float sec_per_move, bool see_time_left, Board board, NeuralNetwork* nn_1, NeuralNetwork* nn_2, bool print_boards, bool clear_console)
{
	EngineConnect4 player1(&board, nn_1);
	EngineConnect4 player2(&board, nn_2);

	while (board.moveCount < 64)
	{
		PositionEval eval = {};
		if (board.sideToPlay)
		{
			player1.update_position(&board);
			eval = player2.evaluate_for(sec_per_move, &board, see_time_left);
		}
		else
		{
			player2.update_position(&board);
			eval = player1.evaluate_for(sec_per_move, &board, see_time_left);
		}

		if (eval.flag == INVALID_BOARD)
		{
			printf("Invalid evaluation received. Instant loss\n");
			return board.sideToPlay ? 1 : 2;
		}

		if (!canPlay(board, eval.column))
		{
			printf("INVALID COLUMN!!! THERE IS A BUG!!!\n");
			return board.sideToPlay ? 1 : 2;
		}


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

// Helper function to thread engine evaluation for batching.

static inline void threaded_evaluate_for(EngineConnect4* player, Board* board, float time_per_move, bool see_time_left)
{
	player->resume();
	player->evaluate_for(time_per_move, board, see_time_left);
	player->suspend();
}

// The specified amount of games are played by an engine using the default scheduler,
// and during the games the different NNs are attached to an engine and asked to evaluate 
// a the current position with a random amount of time. The scores ara based on the
// heuristic depth, tail depth and exact depth of the transposition table entries generated.

void Trainer::deeper_search_tournament(NeuralNetwork** NNs, float* scores, unsigned n_players, unsigned games)
{
	Thread player_threads[BATCH_SIZE];

	for (unsigned game = 0; game < games; game++)
	{
		Board board = {};
		unsigned n_random_moves = rng::random_unsigned(0, ARBITRARY_MAX_RANDOM_MOVES);
		for (unsigned move = 0; move < n_random_moves; move++)
			playMove(board, rng::random_unsigned(0, 7));

		EngineConnect4** players = (EngineConnect4**)calloc(n_players + BATCH_SIZE, sizeof(EngineConnect4*));
		for (unsigned i = 0; i < n_players; i++)
			players[i] = new EngineConnect4(nullptr, NNs[i], true);

		EngineConnect4** schedulers = players + n_players;
		for (unsigned i = 0; i < BATCH_SIZE; i++)
		{
			schedulers[i] = new EngineConnect4(nullptr, (NeuralNetwork*)nullptr, true);
			((HeuristicData*)schedulers[i]->threadedData)->EXACT_TAIL = 10u - i;
		}

		PositionEval global_eval = {};
		while (board.moveCount < 64)
		{
			printf("Current position to evaluate:\n");
			EngineConnect4::decodeBoard(board).console_fancy_print();

			if (rng::random_0_1() > ARBITRARY_SKIP_CHANCE)
			{
				float conservatism = rng::random_norm(1.f, ARBITRARY_CONSERVATISM_DEVIATION);

				float time_per_move = fabsf(rng::random_norm(0.f, ARBITRARY_TIME_DEVIATION));
				if (time_per_move < ARBITRARY_MIN_TIME_PER_MOVE) time_per_move = ARBITRARY_MIN_TIME_PER_MOVE;
				bool see_time_left = bool(rng::random_unsigned(0, 2));

				printf("Time per eval: %.3fs (%s)\n", time_per_move, see_time_left ? "known" : "unknown");
				printf("Conservatism: %.2f\n", conservatism);

				for (unsigned i = 0; i < BATCH_SIZE; i++)
				{
					schedulers[i]->setConservatism(conservatism);
					if (player_threads[i].start(&threaded_evaluate_for, schedulers[i], &board, time_per_move, see_time_left))
					{
						player_threads[i].set_priority(Thread::PRIORITY_ABOVE_NORMAL);
						player_threads[i].set_name(L"Scheduler #%u Evaluation Thread", i);
					}
					else printf("ERROR LAUNCHING THREAD FOR SCHEDULER #%u", i);
				}
				for (unsigned i = 0; i < BATCH_SIZE; i++)
				{
					player_threads[i].join();
					HTTEntry* entry = schedulers[i]->get_entry(&board);
					TTEntry* exact_entry = schedulers[i]->get_exact_entry(&board);
					uint8_t exact_depth = 0;
					if (exact_entry)
						exact_depth = exact_entry->depth;

					if (!entry || entry->flag != ENTRY_FLAG_EXACT)
						printf("Scheduler #%u did not manage to get a single tree done.\n", i);
					else if (entry->eval == 1.f || entry->eval == -1.f)
						printf("Scheduler #%u solved the board.\n", i);
					else
						printf("Scheduler #%u evaluated to depths (%hhu, %hhu, %hhu)\n", i, entry->heuDepth, entry->bitDepth, exact_depth);
				}
				global_eval = schedulers[0]->get_evaluation(&board);
				printf("\n");

				for (unsigned n = 0; n < n_players; n += BATCH_SIZE)
				{
					for (unsigned i = 0; i < BATCH_SIZE && i + n < n_players; i++)
					{
						players[n + i]->setConservatism(conservatism);
						if (player_threads[i].start(&threaded_evaluate_for, players[n + i], &board, time_per_move, see_time_left))
						{
							player_threads[i].set_priority(Thread::PRIORITY_ABOVE_NORMAL);
							player_threads[i].set_name(L"Player #%02u Evaluation Thread", n + i);
						}
						else printf("ERROR LAUNCHING THREAD FOR PLAYER #%02u", n + i);
					}
					for (unsigned i = 0; i < BATCH_SIZE && i + n < n_players; i++)
					{
						player_threads[i].join();
						HTTEntry* entry = players[n + i]->get_entry(&board);
						TTEntry* exact_entry = players[n + i]->get_exact_entry(&board);
						uint8_t exact_depth = 0;
						if (exact_entry)
							exact_depth = exact_entry->depth;

						if (!entry || entry->flag != ENTRY_FLAG_EXACT)
						{
							printf("Player #%02u did not manage to get a single tree done. Score %.2f\n", n + i, ARBITRARY_NO_ENTRY_SCORE);
							scores[n + i] += ARBITRARY_NO_ENTRY_SCORE / ARBITRARY_SCORE_DIVIDER;
						}
						else if (entry->eval == 1.f || entry->eval == -1.f || entry->bitDepth + entry->heuDepth + board.moveCount == 64)
						{
							printf("Player #%02u solved the board. Score +%.2f\n", n + i, ARBITRARY_SOLVED_BOARD_SCORE);
							scores[n + i] += ARBITRARY_SOLVED_BOARD_SCORE / ARBITRARY_SCORE_DIVIDER;
						}
						else
						{
							float score = (1.f + sqrtf(entry->heuDepth * 1.2f)) * (1.f + sqrtf(entry->bitDepth)) * (1.f + sqrtf(exact_depth));
							printf("Player #%02u evaluated to depths (%hhu, %hhu, %hhu). Score +%.2f\n", n + i, entry->heuDepth, entry->bitDepth, exact_depth, score);
							scores[n + i] += score / ARBITRARY_SCORE_DIVIDER;
						}
					}
				}
			}
			else
			{
				printf("Position randomly skipped.\n");
				schedulers[0]->resume();
				global_eval = schedulers[0]->evaluate_for(ARBITRARY_SCHEDULER_TIME, &board);
				schedulers[0]->suspend();
			}

			playMove(board, global_eval.column);
			if (is_win(board.playerBitboard[board.sideToPlay ^ 1]))
				break;
		}

		printf("Game %u has ended.\n", game + 1u);
		EngineConnect4::decodeBoard(board).console_fancy_print();

		for (unsigned i = 0; i < n_players + BATCH_SIZE; i++)
			delete players[i];
		free(players);

	}
	printf("Tournament has ended.\n");
}

// Main function of the class, it takes the last session champions stored in last_session storage,
// multiplies them to fit the training size, adding radom noise to the weights. Performs the 
// specified tournament and wtores the survivors to the session storage folder.

void Trainer::training_session(const char* last_session_storage, const char* this_session_storage, unsigned last_survival_size, unsigned this_session_size, unsigned survival_ranking, float deviation, Training_Type type, Training_Weights weights, bool default_heuristic)
{
	if (last_session_storage && this_session_size % last_survival_size)
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
		while (last_session_storage[++j]) file_path[j] = last_session_storage[j];

		unsigned factor = this_session_size / last_survival_size;

		for (unsigned i = 0; i < last_survival_size; i++)
		{
			filename[10] = '0' + i % 10;
			filename[9] = '0' + (i / 10) % 10;
			filename[8] = '0' + (i / 100) % 10;
			filename[7] = '0' + (i / 1000) % 10;

			int k = -1;
			while (filename[++k]) file_path[j + k] = filename[k];
			file_path[j + k] = '\0';

			stored_nns[i * factor] = new NeuralNetwork(file_path);
		
			NeuralNetwork** sons = generate_inherited_NN(stored_nns[i * factor], factor - 1, weights, deviation);

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
		for (unsigned i = 0; i < this_session_size; i++)
			rewards[i] = ARBITRARY_INITIAL_ELO;
		elo_games_tournament(stored_nns, rewards, this_session_size, DEFAULT_MATCH_ROUNDS, DEFAULT_GAMES_PER_MATCH);
		break;
	case DEEP_SEARCH:
		deeper_search_tournament(stored_nns, rewards, this_session_size, DEFAULT_N_GAMES);
		break;
	}

	// Here ends the survival epoch and NNs are ranked

	printf("\nTOURNAMENT ENDED\nWINNERS:\n");
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

		printf("Player #%02u with score %.2f\n", best_id, rewards[i]);
	}

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
#endif