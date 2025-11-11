#pragma once

/* SCHEDULER NEURAL NETWORK HEADER FILE
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This header contains the Neural Network class that will be used
as a scheduler for tree generation by the Connect4Engine.

Since it will only have one purpose generalizations have been kept 
to the minimum with, the NN consists of a dense block with two hidden 
layers using ReLU as the activation function. 

Weights can be stored and loaded using raw byte files.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

#define NEURAL_NETWORK
#define INPUT_DIM			16
#define HIDDEN_LAYER_DIM	64
#define OUTPUT_DIM			8

/*
-------------------------------------------------------------------------------------------------------
Neural Network Class
-------------------------------------------------------------------------------------------------------
*/

// Simple Neural Network class containing a forward pass and some basic 
// loading and storing weights capabilities. Consists of a dense block
// with two hidden layers of same dimension with ReLU and a linear output.
class NeuralNetwork
{
	// The trainer has access to its private variables and functions.
	friend class Trainer;
private:
	float input_weights[HIDDEN_LAYER_DIM][INPUT_DIM + 1] = {};					// Weights for the input
	float hidden_layer_1_weights[HIDDEN_LAYER_DIM][HIDDEN_LAYER_DIM + 1] = {};	// Weights for the first layer
	float hidden_layer_2_weights[OUTPUT_DIM][HIDDEN_LAYER_DIM + 1] = {};		// Weights for the second layer

	float hidden_layer_1[HIDDEN_LAYER_DIM] = {};	// Stores the first layer nodes
	float hidden_layer_2[HIDDEN_LAYER_DIM] = {};	// Stores the second layer nodes
	float output_layer[OUTPUT_DIM] = {};			// Stores the output layer nodes

	// File management functions

	// Loads weights from a given file.
	void load_weights(const char* weights_file);
	// Stores weights to a given file.
	void store_weights(const char* weights_file);
public:
	// Performs the forward pass with the given input
	// and the current set of weights. Returns output layer.
	float* forward_pass(const float* input);

	// Constructors/Destructors

	// Constructs the Neural Network and assigns it the weights from a given file.
	NeuralNetwork(const char* weights_file);

	NeuralNetwork() = default;	// Default constructor, weights are zero.
	~NeuralNetwork() = default;	// Default destructor.

	// Hard copies

	NeuralNetwork(const NeuralNetwork& other);
	NeuralNetwork& operator=(const NeuralNetwork& other);
};