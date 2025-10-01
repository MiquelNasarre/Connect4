#pragma once
#include "Engine.h"

void playAgainstEngine(float engine_time_per_move, Connect4 position, unsigned char play_as = 1, bool clear_console = false);

void engineAgainstEngine(float engine_time_per_move, Connect4 position, bool clear_console = false);
