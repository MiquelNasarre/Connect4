#pragma once
#include "bitBoard.h"
#include "Engine_NN.h"

#ifdef _TRAINING
/* TRAINER HEADER FILE
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This header includes functions used for training the Neural Networks
used by the Connect4 engine via random search.
Different training methods are defined to train on different tasks of 
the scheduler, primarily tree generation and heuristic evaluation.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// Enumerates the current training methods supported.
enum Training_Type
{
	DEEP_SEARCH,	// Method for deeper tree generation
	WIN_RATE,		// Method for competition capabilities
};

enum Training_Weights
{
	HEURISTIC,		// Only the heuristic weights are trained
	DEPTHS,			// Only the depth related weights are trained
	ALL				// All the weights are trained
};

// Contains the set of static functions used for training NeuralNetwors.
// Most functions use console prints so the class is meant only for console applications.
class Trainer
{
private:
	// Generates a set of Neural Networks copied from the parent Neural Network.
	// For random search a random noise is applied to each weight with the specified deviation.
	static NeuralNetwork** generate_inherited_NN(NeuralNetwork* parent, unsigned amount, Training_Weights weights, float deviation);

	// Generates a set of Neural Networks with weights initialized by a
	// normally distributed random variable with the specified deviation.
	static NeuralNetwork** generate_NN(unsigned amount, float deviation);

	// Add a random noise to the Neural Network weights with the specified deviation.
	static void add_random_noise(NeuralNetwork* nn, Training_Weights weights, float deviation);

	// Degrades the weights by a factor to keep them from exploding.
	static void do_weight_decay(NeuralNetwork* nn, Training_Weights weights, float lambda);

	// Sets the heuristic weights output to the default defined values.
	// It does that by setting all the weights to the specific output 
	// nodes to 0 and the biases to the default values.
	static void set_default_heuristic_weights(NeuralNetwork* nn);

public:
	// Player versus player tournament that during multiple rounds randomly matches the players
	// against each other for a set of games, the score of each player is based on an ELO system 
	// updated through single game wins and losses.
	static void elo_games_tournament(NeuralNetwork** NNs, float* scores, unsigned n_players, unsigned match_rounds, unsigned games_per_match);

	// The specified amount of games are played by an engine using the default scheduler,
	// and during the games the different NNs are attached to an engine and asked to evaluate 
	// a the current position with a random amount of time. The scores ara based on the
	// heuristic depth, tail depth and exact depth of the transposition table entries generated.
	static void deeper_search_tournament(NeuralNetwork** NNs, float* scores, unsigned n_players, unsigned games);

public:
	// Game between two different engines using the specified schedulers, given a certain time per move 
	// and an initial board. It returns 0 if it is a draw, 1 if nn_1 wins and 2 if nn_2 wins.
	static unsigned char NNvsNN(float sec_per_move, bool see_time_left, Board initial_board, NeuralNetwork* nn_1, NeuralNetwork* nn_2, bool print_boards = false, bool clear_console = true);

	// Main function of the class, it takes the last session champions stored in last_session storage,
	// multiplies them to fit the training size, adding radom noise to the weights. Performs the 
	// specified tournament and wtores the survivors to the session storage folder.
	static void training_session(const char* last_session_storage, const char* this_session_storage, unsigned last_survival_size, unsigned this_session_size, unsigned survival_ranking, float deviation, Training_Type type, Training_Weights weights, bool default_heuristic);
};
#endif