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
		for (unsigned i = 0; i < INPUT_DIM + 1; i++)
			input_weights[n][i] = other.input_weights[n][i];
		hidden_layer[n] = other.hidden_layer[n];
	}

	for (unsigned n = 0; n < OUTPUT_DIM; n++)
	{
		for (unsigned i = 0; i < HIDDEN_LAYER_DIM + 1; i++)
			hidden_layer_weights[n][i] = other.hidden_layer_weights[n][i];
		output_layer[n] = other.output_layer[n];
	}

#ifdef _TRAINING
	for (unsigned n = 0; n < HIDDEN_LAYER_DIM; n++)
		for (unsigned i = 0; i < INPUT_DIM + 1; i++)
			_grad_input_weights[n][i] = other._grad_input_weights[n][i];

	for (unsigned n = 0; n < OUTPUT_DIM; n++)
		for (unsigned i = 0; i < HIDDEN_LAYER_DIM + 1; i++)
			_grad_hidden_layer_weights[n][i] = other._grad_hidden_layer_weights[n][i];
#endif
	return *this;
}

/*
-------------------------------------------------------------------------------------------------------
Data Functions
-------------------------------------------------------------------------------------------------------
*/

#ifdef _TRAINING
constexpr uint32_t training_magic = 0xE210B0A1UL;
#endif

// Expected file head when loading weights form file
struct file_head
{
	uint32_t magic = 0xE210B0A1UL;
	uint32_t input_dim = INPUT_DIM;
	uint32_t hidden_dim = HIDDEN_LAYER_DIM;
	uint32_t output_dim = OUTPUT_DIM;
};

// Loads weights from a given file.

void NeuralNetwork::load_weights(const char* weights_file)
{
	file_head head = {};
	file_head expected = {};
	uint32_t magic = 0UL;

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

	for (unsigned n = 0; n < OUTPUT_DIM; n++)
		if (fread(hidden_layer_weights[n], sizeof(float), HIDDEN_LAYER_DIM + 1, file) != HIDDEN_LAYER_DIM + 1)
			goto error;

#ifdef _TRAINING
	if (fread(&magic, sizeof(uint32_t), 1ULL, file) == 1ULL && magic == training_magic)
	{
		for (unsigned n = 0; n < HIDDEN_LAYER_DIM; n++)
			if (fread(_grad_input_weights[n], sizeof(float), INPUT_DIM + 1, file) != INPUT_DIM + 1)
				goto error;

		for (unsigned n = 0; n < OUTPUT_DIM; n++)
			if (fread(_grad_hidden_layer_weights[n], sizeof(float), HIDDEN_LAYER_DIM + 1, file) != HIDDEN_LAYER_DIM + 1)
				goto error;
	}
#ifdef _CONSOLE
	else printf("Unable to load gradient from %s\n", weights_file);
#endif
#endif

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

	for (unsigned n = 0; n < OUTPUT_DIM; n++)
		if (fwrite(hidden_layer_weights[n], sizeof(float), HIDDEN_LAYER_DIM + 1, file) != HIDDEN_LAYER_DIM + 1)
			goto error;

#ifdef _TRAINING
	if (fwrite(&training_magic, sizeof(uint32_t), 1ULL, file) != 1ULL)
		goto error;
	for (unsigned n = 0; n < HIDDEN_LAYER_DIM; n++)
		if (fwrite(_grad_input_weights[n], sizeof(float), INPUT_DIM + 1, file) != INPUT_DIM + 1)
			goto error;

	for (unsigned n = 0; n < OUTPUT_DIM; n++)
		if (fwrite(_grad_hidden_layer_weights[n], sizeof(float), HIDDEN_LAYER_DIM + 1, file) != HIDDEN_LAYER_DIM + 1)
			goto error;
#endif

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
		float& node = hidden_layer[n];
		node = 0.f;
		for (unsigned i = 0; i < INPUT_DIM; i++)
			node += input_weights[n][i] * input[i];
		node += input_weights[n][INPUT_DIM];

		if (node < 0.f)
			node = 0.f;
	}

	for (unsigned n = 0; n < OUTPUT_DIM; n++)
	{
		float& node = output_layer[n];
		node = 0.f;
		for (unsigned i = 0; i < HIDDEN_LAYER_DIM; i++)
			node += hidden_layer_weights[n][i] * hidden_layer[i];
		node += hidden_layer_weights[n][HIDDEN_LAYER_DIM];
	}

	return output_layer;
}
