#pragma once
#include "Engine.h"

// Creates an engine object and starts a game played in the console.
// The engine will think the amount of time specified.
void playAgainstEngine(float engine_time_per_move, Connect4 position, unsigned char play_as = 1, bool clear_console = false);

// Creates an engine object and starts a game with itself played in the console.
// The engine will think the amount of time specified.
char engineAgainstEngine(float engine_time_per_move, Connect4 position, bool clear_console = false);
