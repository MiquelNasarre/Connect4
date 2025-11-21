#include "heuristicSolver.h"
#include "Engine_NN.h"
#include "Engine.h"
#include "Thread.h"
#include "Timer.h"

#include <cmath>
#ifdef _CONSOLE
#include <cstdio>
#endif

#define CALL_MAINLOOP			0
#define CALL_SUSPENDERS			1

#define MAINLOOP_WAIT_TIME_MS	5UL
#define START_DEPTH_H			6
#define START_DEPTH_E			12

#define DEFAULT_DEADLINE		0.0f
#define DEFAULT_CONSERVATISM	1.00f
#define DEFAULT_NO_HTT_DEPTH	5

/*
-------------------------------------------------------------------------------------------------------
Connect4 struct functions
-------------------------------------------------------------------------------------------------------
*/

// Constructor, copies the board elements one by one, without sharing arrays.

inline Connect4::Connect4(const Connect4& other)
{
	for (unsigned char i = 0; i < 8; i++)
		for (unsigned char j = 0; j < 8; j++)
			this->board[i][j] = other.board[i][j];

	this->player = other.player;
}

// Equal operator, copies the board elements one by one, without sharing arrays.

inline Connect4& Connect4::operator=(const Connect4& other)
{
	for (unsigned char i = 0; i < 8; i++)
		for (unsigned char j = 0; j < 8; j++)
			this->board[i][j] = other.board[i][j];

	this->player = other.player;

	return *this;
}

// This constructor copies a conventional 8x8 array as the board. It will
// flip the array to put it in the struct preferred position.

Connect4::Connect4(const unsigned char position[8][8], unsigned char player)
{
	for (unsigned char i = 0; i < 8; i++)
		for (unsigned char j = 0; j < 8; j++)
			this->board[i][j] = position[7 - i][j];

	this->player = player;
}

#ifdef _CONSOLE
// Print board function inherited from last Connect4
// project I did. Do not know how it works.

void Connect4::console_fancy_print()
{
	constexpr int N = 8;
	char T[N][N] = {};
	for (unsigned char i = 0; i < N; i++)
		for (unsigned char j = 0; j < N; j++)
			T[i][j] = (char)board[7 - i][j];

	for (int f = 0; f < N - 3; f++) {
		for (int c = 0; c < N; c++) {
			if (T[f][c] != 0 && T[f][c] == T[f + 1][c] && T[f][c] == T[f + 2][c] && T[f][c] == T[f + 3][c]) { T[f][c] = T[f][c] + 10; T[f + 1][c] = T[f + 1][c] + 10; T[f + 2][c] = T[f + 2][c] + 10; T[f + 3][c] = T[f + 3][c] + 10; }
		}
	}
	for (int f = 0; f < N; f++) {
		for (int c = 0; c < N - 3; c++) {
			if (T[f][c] != 0 && T[f][c] == T[f][c + 1] && T[f][c] == T[f][c + 2] && T[f][c] == T[f][c + 3]) { T[f][c] = T[f][c] + 10; T[f][c + 1] = T[f][c + 1] + 10; T[f][c + 2] = T[f][c + 2] + 10; T[f][c + 3] = T[f][c + 3] + 10; }
		}
	}
	for (int f = 0; f < N - 3; f++) {
		for (int c = 0; c < N - 3; c++) {
			if (T[f][c] != 0 && T[f][c] == T[f + 1][c + 1] && T[f][c] == T[f + 2][c + 2] && T[f][c] == T[f + 3][c + 3]) { T[f][c] = T[f][c] + 10; T[f + 1][c + 1] = T[f + 1][c + 1] + 10; T[f + 2][c + 2] = T[f + 2][c + 2] + 10; T[f + 3][c + 3] = T[f + 3][c + 3] + 10; }
		}
	}
	for (int f = 3; f < N; f++) {
		for (int c = 0; c < N - 3; c++) {
			if (T[f][c] != 0 && T[f][c] == T[f - 1][c + 1] && T[f][c] == T[f - 2][c + 2] && T[f][c] == T[f - 3][c + 3]) { T[f][c] = T[f][c] + 10; T[f - 1][c + 1] = T[f - 1][c + 1] + 10; T[f - 2][c + 2] = T[f - 2][c + 2] + 10; T[f - 3][c + 3] = T[f - 3][c + 3] + 10; }
		}
	}
	system("color");
	printf("\033[0;34m");
	for (int i = 0; i < N; i++) {
		if (!i)printf("\n\n  %c", 201);
		printf("%c%c%c", 205, 205, 205);
		if (i != N - 1)printf("%c", 203);
		else printf("%c", 187);
	}

	for (int i = 0; i < N; i++) {
		printf("\n  ");
		printf("%c", 186);
		for (int j = 0; j < N; j++) {
			if (!T[i][j])printf("   ");
			else {
				if (T[i][j] > 10) {
					T[i][j] = T[i][j] % 10;
					if (T[i][j] == 1)printf("\033[1;31m");
					else if (T[i][j] == 2)printf("\033[1;33m");
					else if (T[i][j] == 3)printf("\033[1;36m");
					else if (T[i][j] == 4)printf("\033[1;32m");
					else if (T[i][j] == 5)printf("\033[1;35m");
					else if (T[i][j] == 6)printf("\033[1;37m");
				}
				else if (T[i][j] == 1)printf("\033[0;31m");
				else if (T[i][j] == 2)printf("\033[0;33m");
				else if (T[i][j] == 3)printf("\033[0;36m");
				else if (T[i][j] == 4)printf("\033[0;32m");
				else if (T[i][j] == 5)printf("\033[0;35m");
				else if (T[i][j] == 6)printf("\033[0;37m");
				printf("%c%c%c", 219, 219, 219);
				printf("\033[0;34m");
			}
			printf("%c", 186);
		}
		printf("\n  ");
		if (i != N - 1) {
			for (int k = 0; k < N; k++) {
				if (!k)printf("%c", 204);
				printf("%c%c%c", 205, 205, 205);
				if (k != N - 1)printf("%c", 206);
				else printf("%c", 185);
			}
		}
		else {
			for (int k = 0; k < N; k++) {
				if (!k)printf("%c", 200);
				printf("%c%c%c", 205, 205, 205);
				if (k != N - 1)printf("%c", 202);
				else printf("%c\n ", 188);
			}
		}
	}
	printf("\033[1;30m");
	for (int i = 0; i < N; i++)printf("   %i", i + 1);
	printf("\n\n");
	printf("\033[0m");
}
#endif

/*
-------------------------------------------------------------------------------------------------------
Struct of data shared between threads stored in *threadedData
-------------------------------------------------------------------------------------------------------
*/

// Struct use throughout the entire class to share information
// with the threads, it created upon construction and stored 
// as a void* threadedData.
//
// Contains simple bools for communication, the main loop thread
// some constants, the transposition tables and the board position.
struct DATA
{
	HeuristicData H_DATA = {};						// Constant variables for the heuristic tree
	TransTable* path_TT = nullptr;					// Transposition tables for the pathfinder

	NeuralNetwork* scheduler = nullptr;
	unsigned char HEURISTIC_DEPTH = START_DEPTH_H;	// Current depth to analize
	unsigned char EXACT_DEPTH = START_DEPTH_E;		// Start exact depth to analize

	// Arbitrary depth for the exact tree to
	// stop storing entries in the HTT
	unsigned char NO_HTT_DEPTH = DEFAULT_NO_HTT_DEPTH;
													
	Board currentBoard = {};						// Current board under evaluation
	unsigned char first_player = 0;					// Stores initial board parity

	Thread* main_loop_thread = nullptr;				// Main loop thread handle
	Timer timer;
	float deadline = 0.f;							// Time left before returning eval
	float conservatism = DEFAULT_CONSERVATISM;		// Divides the time left to hurry the scheduler

	bool sleeping_threads = false;					// Communicator bool to announce workers are sleeping
	bool suspended = false;							// Communicator bool to suspend main loop
	bool terminate = false;							// Communicator bool to terminate main loop
	bool finding_solution = false;					// Communicator bool to announce board is being solved
	bool solution_found = false;					// Communicator bool to announce solved board
	bool updatedBoard = false;						// Communicator bool to announce an new board

	unsigned long long cpu_affinity = 0ULL;			// CPU flag the threads will be set to when running.
};

/*
-------------------------------------------------------------------------------------------------------
Static helper functions
-------------------------------------------------------------------------------------------------------
*/

// It takes a bitBoard and returns the equivalent Connect4 struct.

inline Connect4 EngineConnect4::decodeBoard(const Board& bitBoard)
{
	Connect4 board;

	for (unsigned int col = 0; col < 8; col++)
		for (unsigned int row = 0; row < 8; row++)
		{
			if (bitBoard.playerBitboard[0] & bit_at(col, row))
				board.board[row][col] = 1;
			if (bitBoard.playerBitboard[1] & bit_at(col, row))
				board.board[row][col] = 2;
		}

	board.player = bitBoard.sideToPlay ? 2 : 1;

	return board;
}

// It takes a Connect4 struct and returns the equivalent bitBoard.
// If the board position is invalid it returns nullptr.

inline Board* EngineConnect4::translateBoard(const Connect4& position)
{
	init_zobrist();

	Board* board = new Board();

	for(unsigned char r = 0; r<8; r++)
		for (unsigned char c = 0; c < 8; c++)
		{
			unsigned char player = position.board[r][c];

			if (player == 0)
				continue;

			else if (player == 1)
				board->playerBitboard[0] |= bit_at(c, r);

			else if (player == 2)
				board->playerBitboard[1] |= bit_at(c, r);

			else
			{
				delete board;
				return nullptr;
			}
			board->moveCount++;
			board->heights[c]++;
		}

	board->hash = boardHash(board->playerBitboard);
	
	board->sideToPlay = position.player == 1 ? 0 : 1;

	if (invalidBoard(*board))
	{
		delete board;
		return nullptr;
	}
	return board;
}

// Takes a transposition table entry and returns the corresponding
// position evaluation. If the entry data is not exact returns invalid.

static inline PositionEval obtainPosEvalFromEntry(HTTEntry* e)
{
	if (!e) return PositionEval();

	PositionEval::EvalFlag flag = PositionEval::EvalFlag::DRAW;
	switch (e->flag)
	{
	case ENTRY_FLAG_EXACT:

		if (e->eval == 1.f)
			flag = PositionEval::EvalFlag::CURRENT_PLAYER_WIN;
		else if (e->eval == -1.f)
			flag = PositionEval::EvalFlag::OTHER_PLAYER_WIN;
		else if (e->eval > 0.f)
			flag = PositionEval::EvalFlag::CURRENT_PLAYER_BETTER;
		else if (e->eval < 0.f)
			flag = PositionEval::EvalFlag::OTHER_PLAYER_BETTER;

		return { e->eval,e->order[0], unsigned char(e->heuDepth + e->bitDepth), flag };

	case ENTRY_FLAG_LOWER:
	case ENTRY_FLAG_UPPER:
	default:
		break;
	}

	return PositionEval();
}

/*
-------------------------------------------------------------------------------------------------------
Static main loop worker functions
-------------------------------------------------------------------------------------------------------
*/

// If we find moves that are loosing in the current board we want to make sure this moves are not 
// listed first on its entry.Since the exactTree overrides the HTT this information will go through, 
// but the delay might make the information not arrive on time, this function prevents that.
static inline void check_for_loosing_moves(Board board, TransTable* TT, HTTEntry* e)
{
	if (e->eval != -1.f && e->eval != 1.f)
	{
		e->lock();
		for (unsigned column = 0; column < 8; column++)
		{
			if (!canPlay(board, column))
				continue;

			playMove(board, column);
			TTEntry* sons_entry = TT[board.moveCount].storedBoard(board.hash);
			undoMove(board, column);

			if (sons_entry && sons_entry->score == CURRENT_PLAYER_WIN)
				for (unsigned idx = 0; idx < 7; idx++)
				{
					if (!canPlay(board, e->order[idx + 1]))
						break;

					if (e->order[idx] == column)
					{
						e->order[idx] = e->order[idx + 1];
						e->order[idx + 1] = column;
					}
				}
		}
		e->unlock();
	}
}

// Heuristic Tree threading helper: When called runs until it is called to stop or until it finds a
// solution for the board. Increasing the depth one step at a time from the specified starting depth.
static inline void main_loop_worker_heuristicSolver(Thread* self_thread, DATA* data, bool* busy, bool* kill_exact, bool* STOP)
{
	Board board = data->currentBoard;
	HeuristicData H_DATA = data->H_DATA;
	H_DATA.STOP = STOP;

	HTTEntry* stored_data = H_DATA.HTT[board.moveCount].storedBoard(board.hash);

	// If there is not exact data about your board, you need to be able to overwrite it.
	if (stored_data && stored_data->flag != ENTRY_FLAG_EXACT && stored_data->heuDepth >= data->HEURISTIC_DEPTH)
		stored_data->heuDepth = 0;

	// Loops generating heuristic-trees.
	for (;
		// It will run until the maximum depth or until it finds a winning sequence for either player.
		data->HEURISTIC_DEPTH + H_DATA.EXACT_TAIL + board.moveCount <= 64 &&
		(!stored_data || (stored_data->eval != 1.f && stored_data->eval != -1.f));
		// After every run it increases the depth one step further.
		data->HEURISTIC_DEPTH++
		)
	{
		// Sets the name to its own thread to the current depth for reference.
		self_thread->set_name(L"Worker HEURISTIC: Depths (%u, %u) H%llu", data->HEURISTIC_DEPTH, H_DATA.EXACT_TAIL, board.hash);

		// Computes the heuristic-tree as specified.
		heuristicTree(board, (float)OTHER_PLAYER_WIN, (float)CURRENT_PLAYER_WIN, data->HEURISTIC_DEPTH, H_DATA);

		// If the tree ended by force end the function.
		if (*STOP)
			goto end;

		// We check what data of the actual position we have on storage.
		stored_data = H_DATA.HTT[board.moveCount].storedBoard(board.hash);

		// Make sure loosing moves are not first
		check_for_loosing_moves(board, data->H_DATA.TT, stored_data);

	} --data->HEURISTIC_DEPTH; // To keep depth consistent, oops!

	if (*STOP)
		goto end;

	// If a solution was found in the current position the best path will be solved.
	if (stored_data->eval == 1.f || stored_data->eval == -1.f)
	{
		data->finding_solution = true;
		*kill_exact = true;

		// Sets the name to its own thread to the current state for reference.
		self_thread->set_name(L"Worker HEURISTIC: Pathfinding H%llu", board.hash);

		char* solution = findBestPath(board, (SolveResult)stored_data->eval, data->path_TT, STOP);
		if (*STOP)
			goto end;

		stored_data->heuDepth = 0;
		stored_data->bitDepth = solution[1];
		for (unsigned char c = 0; c < 8; c++)
			if (stored_data->order[c] == solution[0])
				stored_data->order[c] = stored_data->order[0];

		stored_data->order[0] = solution[0];
		free(solution);

		data->solution_found = true;
		data->finding_solution = false;
		goto end;
	}

	// If the final move was reached with no winner the position is solved, is a draw.
	if (stored_data->heuDepth + stored_data->bitDepth + board.moveCount == 64)
	{
		*kill_exact = true;
		data->solution_found = true;
		goto end;
	}

end:
	// cleanup
	*busy = false;
	Thread::wakeUpThreads(CALL_MAINLOOP);
	return;
}

// Exact Tree threading helper: When called runs until it is called to stop or until it finds a
// solution for the board. Increasing the depth one step at a time from the specified starting depth.
static inline void main_loop_worker_exactSolver(Thread* self_thread, DATA* data, bool* busy, bool* kill_heuristic, bool* STOP)
{
	Board board = data->currentBoard;
	
	TTEntry* stored_data = data->H_DATA.TT[board.moveCount].storedBoard(board.hash);

	// Loops generating exact-trees.
	for (;
		// It will run until the maximum depth or until it finds a winning sequence for either player.
		data->EXACT_DEPTH + board.moveCount <= 64 && (!stored_data || stored_data->score == DRAW);
		// After every run it increases the depth one step further.
		data->EXACT_DEPTH++
		)
	{
		// Sets the name to its own thread to the current depth for reference.
		self_thread->set_name(L"Worker EXACT: Depth %u H%llu", data->EXACT_DEPTH, board.hash);

		// Computes the exact-tree as specified.
		exactTree(board, OTHER_PLAYER_WIN, CURRENT_PLAYER_WIN, data->EXACT_DEPTH, data->H_DATA.TT, (void*)data->H_DATA.HTT, data->NO_HTT_DEPTH, STOP);

		// If the tree ended by force end the function.
		if (*STOP)
			goto end;

		// Make sure loosing moves are not listed first
		if (HTTEntry* e = data->H_DATA.HTT[board.moveCount].storedBoard(board.hash))
			check_for_loosing_moves(board, data->H_DATA.TT, e);

		// We check what data of the actual position we have on storage.
		stored_data = data->H_DATA.TT[board.moveCount].storedBoard(board.hash);

	} --data->EXACT_DEPTH; // To keep depth consistent, oops!

	if (*STOP)
		goto end;

	// If a solution was found in the current position the best path will be solved.
	if (stored_data->score != DRAW)
	{
		data->finding_solution = true;
		*kill_heuristic = true;

		HTTEntry* e = data->H_DATA.HTT[board.moveCount].probe(board.hash);
		e->key = board.hash;
		e->bitDepth = 255; // Temporary while it finds the actual solution depth.
		e->heuDepth = 0;
		e->flag = ENTRY_FLAG_EXACT;
		e->eval = (float)stored_data->score;
		e->order[0] = stored_data->bestCol;

		// Sets the name to its own thread to the current state for reference.
		self_thread->set_name(L"Worker EXACT: Pathfinding H%llu", board.hash);

		char* solution = findBestPath(board, (SolveResult)stored_data->score, data->path_TT, STOP);
		if (*STOP)
			goto end;

		e->bitDepth = solution[1];
		for (unsigned char c = 0; c < 8; c++)
			if (e->order[c] == solution[0])
				e->order[c] = e->order[0];

		e->order[0] = solution[0];
		free(solution);

		data->solution_found = true;
		data->finding_solution = false;
		goto end;
	}

	// If the final move was reached with no winner the position is solved, is a draw.
	if (stored_data->depth + board.moveCount == 64)
	{
		data->finding_solution = true;
		*kill_heuristic = true;

		HTTEntry* e = data->H_DATA.HTT[board.moveCount].probe(board.hash);

		e->key = board.hash;
		e->bitDepth = stored_data->depth;
		e->heuDepth = 0;
		e->flag = ENTRY_FLAG_EXACT;
		e->eval = (float)stored_data->score;

		for (unsigned char c = 0; c < 8; c++)
			if (e->order[c] == stored_data->bestCol)
				e->order[c] = e->order[0];

		e->order[0] = stored_data->bestCol;

		data->solution_found = true;
		data->finding_solution = false;
		goto end;
	}

end:
	// cleanup
	*busy = false;
	Thread::wakeUpThreads(CALL_MAINLOOP);
	return;
}

/*
-------------------------------------------------------------------------------------------------------
Main Loop functions
-------------------------------------------------------------------------------------------------------
*/

// Before the creation of a tree a simple MLP gets fed the updated position
// and outputs the details about the following tree generation. Including 
// different depths for the trees and dynamic heuristic weights.

inline void EngineConnect4::update_scheduler() const
{
	DATA* data = (DATA*)threadedData;
	data->updatedBoard = false;

	if (!data->scheduler)
	{
		data->EXACT_DEPTH = START_DEPTH_E;
		data->HEURISTIC_DEPTH = START_DEPTH_H;
	}
	else
	{
		// NN inputs
		Board& board = data->currentBoard;

		float time_left = data->deadline ? data->deadline - data->timer.check() : DEFAULT_DEADLINE;
		if (time_left < 0.f) time_left = DEFAULT_DEADLINE;

		float NN_inputs[INPUT_DIM] = {
			sqrtf(time_left / data->conservatism),
			float(board.moveCount) / 32.f - 1.f,
			float(board.heights[0]) / 4.f - 1.f,
			float(board.heights[1]) / 4.f - 1.f,
			float(board.heights[2]) / 4.f - 1.f,
			float(board.heights[3]) / 4.f - 1.f,
			float(board.heights[4]) / 4.f - 1.f,
			float(board.heights[5]) / 4.f - 1.f,
			float(board.heights[6]) / 4.f - 1.f,
			float(board.heights[7]) / 4.f - 1.f,
		};
		if (NN_inputs[0] > 2.f) NN_inputs[0] = 2.f; // To avoid weird behavior in untrained ranges.

		// Here you feed the data to the NN and expect a return

		float* NN_outputs = data->scheduler->forward_pass(NN_inputs);

		// Assign outputs
		// Smoothen weights
		float total_weight =
			(NN_outputs[0] > 0.f ? NN_outputs[0] : -NN_outputs[0]) +
			(NN_outputs[1] > 0.f ? NN_outputs[1] : -NN_outputs[1]) +
			(NN_outputs[2] > 0.f ? NN_outputs[2] : -NN_outputs[2]) +
			(NN_outputs[3] > 0.f ? NN_outputs[3] : -NN_outputs[3]);
		if (total_weight < 0.01f) total_weight = 0.01f;

		// Assign heuristic weights
		data->H_DATA.FAVORABLES			= NN_outputs[0] * POINT_DISTANCE / total_weight;
		data->H_DATA.FAVORABLE_1MOVE	= NN_outputs[1] * POINT_DISTANCE / total_weight;
		data->H_DATA.POSSIBLES			= NN_outputs[2] * POINT_DISTANCE / total_weight;
		data->H_DATA.POSSIBLES_1MOVE	= NN_outputs[3] * POINT_DISTANCE / total_weight;

		// Convert to integers discrete outputs (+1 to avoid zeros)
		data->H_DATA.ORDERING_DEPTH		= uint8_t(NN_outputs[4] * 8.f) + 1u;
		data->H_DATA.EXACT_TAIL			= uint8_t(NN_outputs[5] * 8.f) + 1u;
		data->HEURISTIC_DEPTH			= uint8_t(NN_outputs[6] * 8.f) + 1u;
		data->EXACT_DEPTH				= uint8_t(NN_outputs[7] * 8.f) + 1u;
	}
	
	// To feed the depths into the workers these need to be at reasonable 
	// values and not exceed the end of the game
	
	// We reference the depths for easier manipulation
	uint8_t& exact_tail = data->H_DATA.EXACT_TAIL;
	uint8_t& heu_depth	= data->HEURISTIC_DEPTH;
	uint8_t& bit_depth	= data->EXACT_DEPTH;
	
	// Keep values under end of game.
	uint8_t max_depth = 64 - data->currentBoard.moveCount;

	if (bit_depth > max_depth) 
		bit_depth = max_depth;
	
	if (exact_tail > max_depth)
	{
		heu_depth = 0;
		exact_tail = max_depth;
	}
	else if (exact_tail + heu_depth > max_depth)
		heu_depth = max_depth - exact_tail;
}

// This is the core of the engine, will be running from creation to
// destruction and will be multithreading to evaluate the current 
// position set by the constructor or updatePosition().

void EngineConnect4::main_loop() const
{
	DATA* data = (DATA*)threadedData;

	// Worker struct to run heuristic and exact trees for the engine.
	struct Worker
	{
	private:
		DATA*	data = nullptr;
		Thread	thread;
		bool	request = false;
		bool	busy = false;
		void(*_proc)(Thread*,DATA*,bool*,bool*,bool*) = nullptr;

	public:
		Worker(DATA* data, void(*proc)(Thread*, DATA*, bool*, bool*, bool*)) :data{ data }, _proc{ proc } {}

		bool* get_request_pointer()
		{
			return &request;
		}

		inline bool is_sleeping() const
		{
			return !busy;
		}

		inline void sleep()
		{
			request = true;
			thread.join();
			request = false;
		}

		inline void run_tree(bool* _killer)
		{
			sleep();
			if (thread.start(_proc, &thread, data, &busy, _killer, &request))
			{
				busy = true;
				thread.set_affinity(data->cpu_affinity);
				thread.set_priority(Thread::PRIORITY_HIGHEST);
			}
			else throw("Error: Unable to launch Worker thread");
		}
	} 
	h_worker(data, &main_loop_worker_heuristicSolver), 
	e_worker(data, &main_loop_worker_exactSolver);

	// Here Starts the main loop itself and will be running until terminate is called.
	// Terminate is called only at class destruction.
	while (!data->terminate)
	{
		// The loop will be waiting in a suspended state if it is suspended or solved.
		if (data->suspended || data->solution_found)
		{
			e_worker.sleep(), h_worker.sleep();

			// Lets everyone know that the workers are not working anymore.
			data->sleeping_threads = true;
			Thread::wakeUpThreads(CALL_SUSPENDERS);
		}
		// If the board has been updated every worker goes to work from scratch.
		// The TTs are the same, if it is a continuation it will catch up quickly.
		else 
		{
			// Let's everyone know that it is not suspended anymore.
			data->sleeping_threads = false;
			Thread::wakeUpThreads(CALL_SUSPENDERS);

			// If the board was updated the sheduler needs to act.
			if (data->updatedBoard)
				update_scheduler();

			// It checks wether the workers were suspended and puts them to work.
			if (h_worker.is_sleeping() && !data->finding_solution) h_worker.run_tree(e_worker.get_request_pointer());
			if (e_worker.is_sleeping() && !data->finding_solution) e_worker.run_tree(h_worker.get_request_pointer());
		}

		// It will wait a little bit after every loop unless it is awakened.
		Thread::waitForWakeUp(CALL_MAINLOOP, MAINLOOP_WAIT_TIME_MS);
	}
	// If engine terminated stop the workers.
	h_worker.sleep(), e_worker.sleep();
}

// Function used to call terminate on the main loop and join it.

void EngineConnect4::kill_main_loop() const
{
	DATA* data = (DATA*)threadedData;

	data->terminate = true;
	Thread::wakeUpThreads(CALL_MAINLOOP);

	data->main_loop_thread->join();
	delete data->main_loop_thread;

	return;
}

/*
-------------------------------------------------------------------------------------------------------
Constructor/Destructor functions
-------------------------------------------------------------------------------------------------------
*/

// Constructor, it calls the main loop to start analyzing the position.
// If no position is provided it will default to initial position. If it
// started suspended, resume needs to be called to start evaluating.

EngineConnect4::EngineConnect4(const Connect4* position, const char* nn_weights_file, bool start_suspended)
{
	init_zobrist();

	threadedData = (void*)new DATA;

	DATA* data = (DATA*)threadedData;
	Thread* thread = data->main_loop_thread = new Thread;

	setSchedulerWeights(nn_weights_file);

	if (start_suspended)
		data->suspended = true;

	Board* board = nullptr;
	if (position)
		board = translateBoard(*position);

	if (board)
	{
		data->first_player = (board->moveCount + board->sideToPlay) % 2;
		data->currentBoard = *board;
		delete board;
	}
	else
	{
		data->first_player = 0;
		data->currentBoard = Board();
	}
	data->updatedBoard = true;

	data->H_DATA.TT = new TransTable[65];
	data->H_DATA.HTT = new HeuristicTransTable[65];
	data->path_TT = new TransTable[65];

	thread->start(&EngineConnect4::thread_entry, this);
	thread->set_priority(Thread::PRIORITY_NORMAL);
	thread->set_name(L"Engine Main Loop");
}

// Constructor, it calls the main loop to start analyzing the board.
// If no board is provided it will default to initial position. If it
// started suspended, resume needs to be called to start evaluating.

EngineConnect4::EngineConnect4(const Board* board, const char* nn_weights_file, bool start_suspended)
{
	init_zobrist();

	threadedData = (void*)new DATA;

	DATA* data = (DATA*)threadedData;
	Thread* thread = data->main_loop_thread = new Thread;

	setSchedulerWeights(nn_weights_file);

	if (start_suspended)
		data->suspended = true;

	if (board && !invalidBoard(*board))
	{
		data->first_player = (board->moveCount + board->sideToPlay) % 2;
		data->currentBoard = *board;
	}
	else
	{
		data->first_player = 0;
		data->currentBoard = Board();
	}
	data->updatedBoard = true;

	data->H_DATA.TT = new TransTable[65];
	data->H_DATA.HTT = new HeuristicTransTable[65];
	data->path_TT = new TransTable[65];

	thread->start(&EngineConnect4::thread_entry, this);
	thread->set_priority(Thread::PRIORITY_NORMAL);
	thread->set_name(L"Engine Main Loop");
}

// Constructor, it calls the main loop to start analyzing the board.
// If no board is provided it will default to initial position. If it
// started suspended, resume needs to be called to start evaluating.

EngineConnect4::EngineConnect4(const Board* board, NeuralNetwork* nn_scheduler, bool start_suspended)
{
	init_zobrist();

	threadedData = (void*)new DATA;

	DATA* data = (DATA*)threadedData;
	Thread* thread = data->main_loop_thread = new Thread;

	setScheduler(nn_scheduler);

	if (start_suspended)
		data->suspended = true;

	if (board && !invalidBoard(*board))
	{
		data->first_player = (board->moveCount + board->sideToPlay) % 2;
		data->currentBoard = *board;
	}
	else
	{
		data->first_player = 0;
		data->currentBoard = Board();
	}
	data->updatedBoard = true;

	data->H_DATA.TT = new TransTable[65];
	data->H_DATA.HTT = new HeuristicTransTable[65];
	data->path_TT = new TransTable[65];

	thread->start(&EngineConnect4::thread_entry, this);
	thread->set_priority(Thread::PRIORITY_NORMAL);
	thread->set_name(L"Engine Main Loop");
}

// Destructor, calls kill_main_loop and frees the allocated data.

EngineConnect4::~EngineConnect4()
{
	kill_main_loop();

	DATA* data = (DATA*)threadedData;

	if (data->scheduler)
		delete data->scheduler;
	delete[] data->H_DATA.TT;
	delete[] data->H_DATA.HTT;
	delete[] data->path_TT;

	delete data;
}

/*
-------------------------------------------------------------------------------------------------------
User functions for using the engine
-------------------------------------------------------------------------------------------------------
*/

// New evaluation trees will not be called in the main loop.
// Current evaluation trees will be allowed to finish.

void EngineConnect4::suspend() const
{
	DATA* data = (DATA*)threadedData;

	data->suspended = true;
	Thread::wakeUpThreads(CALL_MAINLOOP);

	// Wait until main-loop stops working.
	while(!data->sleeping_threads)
		Thread::waitForWakeUp(CALL_SUSPENDERS, 1UL);
}

// Allows the continuation of the main loop.
// If an engine has started suspended, resume has to be called
// for it to begin evaluating the position.

void EngineConnect4::resume() const
{
	DATA* data = (DATA*)threadedData;

	data->suspended = false;
	Thread::wakeUpThreads(CALL_MAINLOOP);

	// Wait until main-loop starts working.
	while (data->sleeping_threads && !data->solution_found)
		Thread::waitForWakeUp(CALL_SUSPENDERS, 1UL);
}

// Sends a new position to the engine that will be called for evaluation.
// Returns true if updated correctly, returns false if invalid position.

bool EngineConnect4::update_position(const Connect4* newPosition)
{
	if (!newPosition)
		return false;

	Board* board = translateBoard(*newPosition);
	if (!board)
		return false;

	bool success = update_position(board);
	delete board;
	return success;
}

// Sends a new position to the engine that will be called for evaluation.
// Returns true if updated correctly, returns false if invalid position.

bool EngineConnect4::update_position(const Board* board)
{
	DATA* data = (DATA*)threadedData;

	if (!board ||
		invalidBoard(*board) ||
		// Makes sure the side to play at a given turn is the same as the first board provided
		board->moveCount % 2 != (board->sideToPlay + data->first_player) % 2
		) return false;

	if (board->hash == data->currentBoard.hash)
		return true;

	bool was_suspended = data->suspended;
	suspend();

	data->currentBoard = *board;
	data->updatedBoard = true;
	data->finding_solution = false;

	bool game_ended =   is_win(data->currentBoard.playerBitboard[0]) ||
						is_win(data->currentBoard.playerBitboard[1]) ||
						data->currentBoard.moveCount == 64;

	data->solution_found = game_ended;

	if (!was_suspended)
		resume();

	return true;
}

// If the position is in memory it will return a PositionEval for that position.
// If it is not in memory EvalFlag will be INVALID_BOARD, nullptr is current position.

PositionEval EngineConnect4::get_evaluation(const Connect4* position) const
{
	DATA* data = (DATA*)threadedData;

	Board* board = &data->currentBoard;
	if (position)
		board = translateBoard(*position);

	if (board)
	{
		PositionEval eval = obtainPosEvalFromEntry(data->H_DATA.HTT[board->moveCount].storedBoard(board->hash));
		if (position)
			delete board;
		return eval;
	}

	return PositionEval();
}

// If the board is in memory it will return a PositionEval for that position.
// If it is not in memory EvalFlag will be INVALID_BOARD, nullptr is current position.

PositionEval EngineConnect4::get_evaluation(const Board* board) const
{
	DATA* data = (DATA*)threadedData;

	if (board)
		return obtainPosEvalFromEntry(data->H_DATA.HTT[board->moveCount].storedBoard(board->hash));

	return obtainPosEvalFromEntry(data->H_DATA.HTT[data->currentBoard.moveCount].storedBoard(data->currentBoard.hash));
}

// It returns a position evaluation after a specified time, you can either 
// enter a new position or leave it at nullptr to maintain current position.
// If it finds a forced win it will return immediately.

PositionEval EngineConnect4::evaluate_for(float seconds, const Connect4* position)
{
	DATA* data = (DATA*)threadedData;

	data->deadline = data->timer.check() + seconds;

	if (position && !update_position(position))
		return PositionEval(); // invalid

	while (data->deadline - data->timer.check() > 0.f && !data->solution_found)
		Thread::waitForWakeUp(CALL_MAINLOOP, 1UL);

	return get_evaluation();
}

// It updates the current board, and it returns a board evaluation after
// a specified time. If it finds a forced win it will return immediately.

PositionEval EngineConnect4::evaluate_for(float seconds, const Board* board, bool update_scheduler)
{
	DATA* data = (DATA*)threadedData;

	float deadline = data->timer.check() + seconds;
	if (update_scheduler)
		data->deadline = deadline;

	if (board && !update_position(board))
		return PositionEval(); // invalid


	while (deadline - data->timer.check() > 0.f && !data->solution_found)
		Thread::waitForWakeUp(CALL_MAINLOOP, 1UL);

	return get_evaluation();
}

// The new position is introduced and will not return a PositionEval until the depth of the
// evaluation is at least the specified or the position is solved, nullptr for current position.

PositionEval EngineConnect4::evaluate_until_depth(unsigned char total_depth, const Connect4* position)
{
	DATA* data = (DATA*)threadedData;

	if (position && !update_position(position))
		return PositionEval(); // invalid

	HTTEntry* e = data->H_DATA.HTT[data->currentBoard.moveCount].storedBoard(data->currentBoard.hash);

	while ((!e || e->heuDepth + e->bitDepth < total_depth) && !data->solution_found)
	{
		e = data->H_DATA.HTT[data->currentBoard.moveCount].storedBoard(data->currentBoard.hash);
		Thread::waitForWakeUp(CALL_MAINLOOP, MAINLOOP_WAIT_TIME_MS);
	}

	return obtainPosEvalFromEntry(e);
}

// The new board is introduced and will not return a PositionEval until the depth
// of the evaluation is at least the specified or the board is solved.

PositionEval EngineConnect4::evaluate_until_depth(unsigned char total_depth, const Board* board)
{
	DATA* data = (DATA*)threadedData;

	if (board && !update_position(board))
		return PositionEval(); // invalid

	HTTEntry* e = data->H_DATA.HTT[data->currentBoard.moveCount].storedBoard(data->currentBoard.hash);

	while ((!e || e->heuDepth + e->bitDepth < total_depth) && !data->solution_found)
	{
		e = data->H_DATA.HTT[data->currentBoard.moveCount].storedBoard(data->currentBoard.hash);
		Thread::waitForWakeUp(CALL_MAINLOOP, MAINLOOP_WAIT_TIME_MS);
	}

	return obtainPosEvalFromEntry(e);
}

// Returns the HTTEntry* to the transposition table for the specified board.

HTTEntry* EngineConnect4::get_entry(const Board* board) const
{
	DATA* data = (DATA*)threadedData;

	return data->H_DATA.HTT[board->moveCount].storedBoard(board->hash);
}

// Returns the TTEntry* to the transposition table for the specified board.

TTEntry* EngineConnect4::get_exact_entry(const Board* board) const
{
	DATA* data = (DATA*)threadedData;

	return data->H_DATA.TT[board->moveCount].storedBoard(board->hash);
}

// Sets the weights for the Neural Network that schedules tree calls to the ones 
// specified in the weights file. If nullptr or file not found no NN scheduler is used.

void EngineConnect4::setSchedulerWeights(const char* nn_weights_file) const
{
	DATA* data = (DATA*)threadedData;

	if (data->scheduler)
	{
		delete data->scheduler;
		data->scheduler = nullptr;
	}

	if (nn_weights_file)
	{
		if (FILE* file = fopen(nn_weights_file, "rb"))
		{
			fclose(file);
			data->scheduler = new NeuralNetwork(nn_weights_file);
		}
	}
}

// Sets the scheduler to be used by the engine during tree calls.

void EngineConnect4::setScheduler(NeuralNetwork* nn_scheduler, bool copy) const
{
	DATA* data = (DATA*)threadedData;

	if (data->scheduler)
		delete data->scheduler;

	if (copy && nn_scheduler)
		data->scheduler = new NeuralNetwork(*nn_scheduler);
	else
		data->scheduler = nn_scheduler;
}

// Sets the conservatism value for the scheduler. Depending on the computer speed a 
// higher conservatism might be needed to ensure tree completion or a lower to get 
// better depth readings. It divides the time left to hurry the scheduler.

void EngineConnect4::setConservatism(float conservatism) const
{
	DATA* data = (DATA*)threadedData;

	if (conservatism > 0.f)
		data->conservatism = conservatism;
}

// Sets the CPU affinity of the threads run by the engine, set to all cpu cores by 
// default. For immediate enforcement call suspend/resume. Returns true uppon success, 
// false otherwise. For more information on how to select the CPU check Thread.h.

bool EngineConnect4::set_affinity(unsigned long long cpu_flag) const
{
	DATA* data = (DATA*)threadedData;

	if (!data->main_loop_thread->set_affinity(cpu_flag))
		return false;

	data->cpu_affinity = cpu_flag;
	return true;
}

// Returns a Connect4 struct copy of the current position under evaluation.

Connect4 EngineConnect4::get_current_position() const
{
	return decodeBoard(((DATA*)threadedData)->currentBoard);
}

// Returns a Board struct copy of the current position under evaluation.

Board EngineConnect4::get_current_bitBoard() const
{
	return ((DATA*)threadedData)->currentBoard;
}
