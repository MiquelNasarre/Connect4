#pragma once
#include "Timer.h"

#include <cstdint>
#include <cmath>

/*
-------------------------------------------------------------------------------------------------------
Random Number Generator Class
-------------------------------------------------------------------------------------------------------
*/

class rng
{
private:
	// Seed that is modified to produce random numbers.
	static uint64_t _seed;

public:
	// Sets the seed used for random numbers.
	static inline void set_seed(uint64_t seed)
	{
		_seed = seed;
	}

	// Mixes up the randomizer seed.
	static inline uint64_t& splitmix(uint64_t& seed)
	{
		seed += 0x9E3779B97F4A7C15ULL;
		seed = (seed ^ (seed >> 30)) * 0xBF58476D1CE4E5B9ULL;
		seed = (seed ^ (seed >> 27)) * 0x94D049BB133111EBULL;
		seed ^= (seed >> 31);
		return seed;
	}

	// Generates random values with the specified normal distribution.
	static inline float random_norm(float mean, float deviation)
	{
		float rand_1 = (splitmix(_seed) >> 8) * (1.0f / 72057594037927936.0f); // Random value between 0 and 1
		float rand_2 = (splitmix(_seed) >> 8) * (1.0f / 72057594037927936.0f); // Random value between 0 and 1

		float z = sqrtf(-2.0f * logf(rand_1)) * cosf(2.0f * 3.1415926535f * rand_2); // Normal distribution N(0,1) via Box-Muller

		return z * deviation + mean; // Normal distribution N(mean,std)
	}

	// Uses a randomizer to generate a shuffled set of integers from 0 to size.
	static inline unsigned* shuffled_integers(unsigned size)
	{
		unsigned* out = (unsigned*)calloc(size, sizeof(unsigned));
		for (unsigned i = 0; i < size && out; ++i) out[i] = i;

		// Fisher–Yates
		for (unsigned i = size; i > 1 && out; --i) {
			unsigned j = (unsigned)(splitmix(_seed) % i); // 0..i-1
			unsigned tmp = out[i - 1];
			out[i - 1] = out[j];
			out[j] = tmp;
		}
		return out;
	}

	// Generates a random natural number between begin and end (included).
	static inline unsigned random_unsigned(unsigned begin, unsigned end)
	{
		return splitmix(_seed) % (end + 1 - begin) + begin;
	}

	// Generates a random number between 0 and 1.
	static inline float random_0_1()
	{
		return (splitmix(_seed) >> 8) * (1.0f / 72057594037927936.0f); // Random value between 0 and 1
	}
};

inline uint64_t rng::_seed = Timer::get_system_time_ns() ^ (uint64_t)&_seed;
