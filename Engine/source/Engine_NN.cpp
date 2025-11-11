#include "Engine_NN.h"

#include <cstdio>
#include <cstdint>

/*
-------------------------------------------------------------------------------------------------------
Constructors / Copiess
-------------------------------------------------------------------------------------------------------
*/

// Constructs the Neural Network and assigns it the weights from a given file.

NeuralNetwork::NeuralNetwork(const char* weights_file)
{
	load_weights(weights_file);
}

// Hard copies

NeuralNetwork::NeuralNetwork(const NeuralNetwork& other)
{
	*this = other;
}

NeuralNetwork& NeuralNetwork::operator=(const NeuralNetwork& other)
{
	for (unsigned n = 0; n < HIDDEN_LAYER_DIM; n++)
	{
		for (unsigned i = 0; i < INPUT_DIM; i++)
			input_weights[n][i] = other.input_weights[n][i];
		hidden_layer_1[n] = other.hidden_layer_1[n];
	}

	for (unsigned n = 0; n < HIDDEN_LAYER_DIM; n++)
	{
		for (unsigned i = 0; i < HIDDEN_LAYER_DIM; i++)
			hidden_layer_1_weights[n][i] = other.hidden_layer_1_weights[n][i];
		hidden_layer_2[n] = other.hidden_layer_2[n];
	}

	for (unsigned n = 0; n < OUTPUT_DIM; n++)
	{
		for (unsigned i = 0; i < HIDDEN_LAYER_DIM; i++)
			hidden_layer_2_weights[n][i] = other.hidden_layer_2_weights[n][i];
		output_layer[n] = other.output_layer[n];
	}

	return *this;
}

/*
-------------------------------------------------------------------------------------------------------
Data Functions
-------------------------------------------------------------------------------------------------------
*/

// Expected file head when loading weights form file
struct file_head
{
	uint32_t magic = 0xE200B001U;
	uint32_t input_dim = INPUT_DIM;
	uint32_t hidden_dim = HIDDEN_LAYER_DIM;
	uint32_t output_dim = OUTPUT_DIM;
};

// Loads weights from a given file.

void NeuralNetwork::load_weights(const char* weights_file)
{
	file_head head = {};
	file_head expected = {};

	FILE* file = fopen(weights_file, "rb");
	if (!file)
		goto error;

	if (fread(&head, sizeof(file_head), 1ULL, file) != 1ULL ||
		head.magic != expected.magic ||
		head.input_dim != expected.input_dim ||
		head.hidden_dim != expected.hidden_dim ||
		head.output_dim != expected.output_dim)
		goto error;

	for (unsigned n = 0; n < HIDDEN_LAYER_DIM; n++)
		if (fread(input_weights[n], sizeof(float), INPUT_DIM + 1, file) != INPUT_DIM + 1)
			goto error;

	for (unsigned n = 0; n < HIDDEN_LAYER_DIM; n++)
		if (fread(hidden_layer_1_weights[n], sizeof(float), HIDDEN_LAYER_DIM + 1, file) != HIDDEN_LAYER_DIM + 1)
			goto error;

	for (unsigned n = 0; n < OUTPUT_DIM; n++)
		if (fread(hidden_layer_2_weights[n], sizeof(float), HIDDEN_LAYER_DIM + 1, file) != HIDDEN_LAYER_DIM + 1)
			goto error;

	fclose(file);
	return;
error:
#ifdef _CONSOLE
	printf("Unexpected error when loading weights from file");
#endif
	if (file)
		fclose(file);
	return;
}

// Stores weights to a given file.

void NeuralNetwork::store_weights(const char* weights_file)
{
	file_head head = {};

	FILE* file = fopen(weights_file, "wb");
	if (!file)
		goto error;

	if(fwrite(&head, sizeof(file_head), 1ULL, file) != 1ULL)
		goto error;

	for (unsigned n = 0; n < HIDDEN_LAYER_DIM; n++)
		if (fwrite(input_weights[n], sizeof(float), INPUT_DIM + 1, file) != INPUT_DIM + 1)
			goto error;

	for (unsigned n = 0; n < HIDDEN_LAYER_DIM; n++)
		if (fwrite(hidden_layer_1_weights[n], sizeof(float), HIDDEN_LAYER_DIM + 1, file) != HIDDEN_LAYER_DIM + 1)
			goto error;

	for (unsigned n = 0; n < OUTPUT_DIM; n++)
		if (fwrite(hidden_layer_2_weights[n], sizeof(float), HIDDEN_LAYER_DIM + 1, file) != HIDDEN_LAYER_DIM + 1)
			goto error;

	fclose(file);
	return;
error:
#ifdef _CONSOLE
	printf("Unexpected error when saving weights to file");
#endif
	if (file)
		fclose(file);
	return;
}

/*
-------------------------------------------------------------------------------------------------------
User end Functions
-------------------------------------------------------------------------------------------------------
*/

// Performs the forward pass with the given input
// and the current set of weights. Returns output layer.

float* NeuralNetwork::forward_pass(const float* input)
{
	for (unsigned n = 0; n < HIDDEN_LAYER_DIM; n++)
	{
		float& node = hidden_layer_1[n];
		node = 0.f;
		for (unsigned i = 0; i < INPUT_DIM; i++)
			node += input_weights[n][i] * input[i];
		node += input_weights[n][INPUT_DIM];

		if (node < 0.f)
			node = 0.f;
	}

	for (unsigned n = 0; n < HIDDEN_LAYER_DIM; n++)
	{
		float& node = hidden_layer_2[n];
		node = 0.f;
		for (unsigned i = 0; i < HIDDEN_LAYER_DIM; i++)
			node += hidden_layer_1_weights[n][i] * hidden_layer_1[i];
		node += hidden_layer_1_weights[n][HIDDEN_LAYER_DIM];

		if (node < 0.f)
			node = 0.f;
	}

	for (unsigned n = 0; n < OUTPUT_DIM; n++)
	{
		float& node = output_layer[n];
		node = 0.f;
		for (unsigned i = 0; i < HIDDEN_LAYER_DIM; i++)
			node += hidden_layer_2_weights[n][i] * hidden_layer_2[i];
		node += hidden_layer_2_weights[n][HIDDEN_LAYER_DIM];
	}

	return output_layer;
}
