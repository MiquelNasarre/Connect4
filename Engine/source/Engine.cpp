#include "heuristicSolver.h"
#include "Engine_NN.h"
#include "Engine.h"
#include "Thread.h"
#include "Timer.h"

#ifdef _CONSOLE
#include <stdio.h>
#endif

#define CALL_MAINLOOP			0
#define CALL_WORKER(idx)		((idx) + 1)
#define MAX_WORKERS				1

#define MAINLOOP_WAIT_TIME_MS	5
#define START_DEPTH_H			3
#define START_DEPTH_E			13

#define DEFAULT_DEADLINE		1.f

#define CPU_FLAG(idx)			255

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
// flip the array to put it in the struct prefered position.

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
// Contains simple bools for comunication, the main loop thread
// some constants, the transposition tables and the board position.
struct DATA
{
	HeuristicData H_DATA = {};						// Constant variables for the heuristic tree

	NeuralNetwork* scheduler = nullptr;
	unsigned char HEURISTIC_DEPTH = START_DEPTH_H;	// Current depth to analize
	unsigned char EXACT_DEPTH = START_DEPTH_E;		// Start exact depth to analize

	Board currentBoard = {};						// Current board under evaluation
	unsigned char first_player = 0;					// Stores initial board parity

	Thread* main_loop_thread = nullptr;				// Main loop thread handle
	Timer timer;
	float deadline = 0.f;							// Time left before returning eval

	bool suspended = false;							// Comunicator bool to suspend main loop
	bool terminate = false;							// Comunicator bool to terminate main loop
	bool finding_solution = false;					// Comunicator bool to anounce board is being solved
	bool solution_found = false;					// Comunicator bool to anounce solved board
	bool updatedBoard = false;						// Comunicator bool to anounce an new board

	unsigned char maxDepth = UNLIMITED_DEPTH;		// Stores the max depth allowed to compute
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

// It takes a Connect4 strunc and returns the equivalent bitBoard.
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

static inline void main_loop_worker_heuristicSolver(Board board, unsigned char depth, HeuristicData* H_DATA, bool* busy, bool* solving, bool* solved, bool* kill_exact)
{
	// If there is not exact data about your board, you need to be able to overwrite it.
	if (HTTEntry* storedData = H_DATA->HTT[board.moveCount].storedBoard(board.hash))
		if (storedData->flag != ENTRY_FLAG_EXACT && storedData->heuDepth > depth)
			storedData->heuDepth = depth;

	heuristicTree(board, OTHER_PLAYER_WIN, CURRENT_PLAYER_WIN, depth, *H_DATA);

	HTTEntry* storedData = H_DATA->HTT[board.moveCount].storedBoard(board.hash);

	if (!storedData || *H_DATA->STOP)
		goto end;

	if (storedData->eval == 1.f || storedData->eval == -1.f)
	{
		*solving = true;
		*kill_exact = true;

		char* solution = findBestPath(board, (SolveResult)storedData->eval, nullptr, H_DATA->STOP);
		if (*H_DATA->STOP)
			goto end;

		storedData->heuDepth = 0;
		storedData->bitDepth = solution[1];
		for (unsigned char c = 0; c < 8; c++)
			if (storedData->order[c] == solution[0])
				storedData->order[c] = storedData->order[0];

		storedData->order[0] = solution[0];

		free(solution);
		*solved = true;
		*solving = false;
		goto end;
	}

	if (storedData->heuDepth + storedData->bitDepth + board.moveCount == 64)
	{
		*kill_exact = true;
		*solved = true;
		goto end;
	}

end:
	*busy = false;
	Thread::wakeUpThreads(CALL_MAINLOOP);
	delete H_DATA;
	return;
}

static inline void main_loop_worker_exactSolver(Board board, unsigned char depth, HeuristicData* H_DATA, bool* busy, bool* solving, bool* solved, bool* kill_heuristic)
{
	if(depth > 64 - board.moveCount) 
		depth = 64 - board.moveCount;
	
	
	exactTree(board, OTHER_PLAYER_WIN, CURRENT_PLAYER_WIN, depth, H_DATA->TT, H_DATA->STOP);
	TTEntry* storedData = H_DATA->TT[board.moveCount].storedBoard(board.hash);

	if (!storedData || *H_DATA->STOP)
		goto end;

	if (storedData->score != DRAW)
	{
		*solving = true;
		*kill_heuristic = true;
		HTTEntry* e = H_DATA->HTT[board.moveCount].probe(board.hash);

		e->key = board.hash;
		e->bitDepth = 255;
		e->heuDepth = 0;
		e->flag = ENTRY_FLAG_EXACT;
		e->eval = (float)storedData->score;
		e->order[0] = storedData->bestCol;


		char* solution = findBestPath(board, (SolveResult)storedData->score, nullptr, H_DATA->STOP);
		if (*H_DATA->STOP)
			goto end;

		e->bitDepth = solution[1];
		for (unsigned char c = 0; c < 8; c++)
			if (e->order[c] == solution[0])
				e->order[c] = e->order[0];

		e->order[0] = solution[0];

		free(solution);
		*solved = true;
		*solving = false;
		goto end;
	}

	if (depth + board.moveCount == 64)
	{
		*solving = true;
		*kill_heuristic = true;

		HTTEntry* e = H_DATA->HTT[board.moveCount].probe(board.hash);

		e->key = board.hash;
		e->bitDepth = storedData->depth;
		e->heuDepth = 0;
		e->flag = ENTRY_FLAG_EXACT;
		e->eval = (float)storedData->score;

		for (unsigned char c = 0; c < 8; c++)
			if (e->order[c] == storedData->bestCol)
				e->order[c] = e->order[0];

		e->order[0] = storedData->bestCol;

		*solved = true;
		*solving = false;
		goto end;
	}

end:
	*busy = false;
	Thread::wakeUpThreads(CALL_MAINLOOP);
	delete H_DATA;
	return;
}

/*
-------------------------------------------------------------------------------------------------------
Main Loop functions
-------------------------------------------------------------------------------------------------------
*/

// Before the creation of a tree a future Machine Learning algorithm
// will analyze which data it want the tree to work with. Including 
// depth, exact tree depth, heuristic weights, number of workers, etc.

inline void EngineConnect4::update_ML_DATA() const
{
	DATA* data = (DATA*)threadedData;

	if (data->updatedBoard)
	{
		data->EXACT_DEPTH = START_DEPTH_E;
		data->HEURISTIC_DEPTH = START_DEPTH_H;
		data->H_DATA.EXACT_TAIL = DEFAULT_TAIL_DEPTH;
	}

	if (!data->scheduler)
	{
		data->updatedBoard = false;
		return;
	}


	// NN inputs
	bool& updated_board		= data->updatedBoard;
	uint8_t& moveCount		= data->currentBoard.moveCount;
	uint8_t& col_height0	= data->currentBoard.heights[0];
	uint8_t& col_height1	= data->currentBoard.heights[1];
	uint8_t& col_height2	= data->currentBoard.heights[2];
	uint8_t& col_height3	= data->currentBoard.heights[3];
	uint8_t& col_height4	= data->currentBoard.heights[4];
	uint8_t& col_height5	= data->currentBoard.heights[5];
	uint8_t& col_height6	= data->currentBoard.heights[6];
	uint8_t& col_height7	= data->currentBoard.heights[7];

	HTTEntry* h_entry = data->H_DATA.HTT[data->currentBoard.moveCount].storedBoard(data->currentBoard.hash);
	TTEntry* e_entry = data->H_DATA.TT[data->currentBoard.moveCount].storedBoard(data->currentBoard.hash);

	uint8_t heu_depth_in	= h_entry ? h_entry->heuDepth : 0u;
	uint8_t exact_tail_in	= h_entry ? h_entry->bitDepth : 0u;
	uint8_t bit_depth_in	= e_entry ? e_entry->depth : 0u;
	uint8_t eval_flag		= h_entry ? h_entry->flag : ENTRY_FLAG_UPPER;
	float evaluation		= h_entry ? h_entry->eval : 0.0f;
	float time_left			= data->deadline ? data->deadline - data->timer.check() : DEFAULT_DEADLINE;
	if (time_left < 0.f) time_left = 1.f;

	// NN outputs
	float& heu_weight0		= data->H_DATA.FAVORABLES;
	float& heu_weight1		= data->H_DATA.FAVORABLE_1MOVE;
	float& heu_weight2		= data->H_DATA.POSSIBLES;
	float& heu_weight3		= data->H_DATA.POSSIBLES_1MOVE;
	uint8_t& order_depth	= data->H_DATA.ORDERING_DEPTH;
	uint8_t& exact_tail_out	= data->H_DATA.EXACT_TAIL;
	uint8_t& heu_depth_out	= data->HEURISTIC_DEPTH;
	uint8_t& bit_depth_out	= data->EXACT_DEPTH;

	float NN_inputs[INPUT_DIM] = {
		float(updated_board),
		moveCount / 64.f,
		col_height0 / 8.f,
		col_height1 / 8.f,
		col_height2 / 8.f,
		col_height3 / 8.f,
		col_height4 / 8.f,
		col_height5 / 8.f,
		col_height6 / 8.f,
		col_height7 / 8.f,
		float(heu_depth_in),
		float(exact_tail_in),
		float(bit_depth_in),
		float(eval_flag),
		evaluation,
		time_left
	};

	// Here you feed the data to the NN and expect a return

	float* NN_outputs = data->scheduler->forward_pass(NN_inputs);

	// Here you assign the outputs

	// Smoothen weights
	float total_weight = 
		(NN_outputs[0] > 0.f ? NN_outputs[0] : -NN_outputs[0]) + 
		(NN_outputs[1] > 0.f ? NN_outputs[1] : -NN_outputs[1]) +
		(NN_outputs[2] > 0.f ? NN_outputs[2] : -NN_outputs[2]) + 
		(NN_outputs[3] > 0.f ? NN_outputs[3] : -NN_outputs[3]);

	if (total_weight < 0.01f) total_weight = 0.01f;
	heu_weight0 = NN_outputs[0] * POINT_DISTANCE / total_weight;
	heu_weight1 = NN_outputs[1] * POINT_DISTANCE / total_weight;
	heu_weight2 = NN_outputs[2] * POINT_DISTANCE / total_weight;
	heu_weight3 = NN_outputs[3] * POINT_DISTANCE / total_weight;

	// Convert to integers discrete outputs
	order_depth = uint8_t(NN_outputs[4]);
	exact_tail_out = uint8_t(NN_outputs[5]);
	heu_depth_out = uint8_t(NN_outputs[6]);
	bit_depth_out = uint8_t(NN_outputs[7]);
	
	// Keep values under end of game
	if (exact_tail_out + heu_depth_out + data->currentBoard.moveCount > 64)
	{
		if (exact_tail_out + data->currentBoard.moveCount > 64)
		{
			heu_depth_out = 0;
			exact_tail_out = 64 - data->currentBoard.moveCount;
		}
		else
			heu_depth_out = 64 - exact_tail_out - data->currentBoard.moveCount;
	}
	if (bit_depth_out + data->currentBoard.moveCount > 64)
		bit_depth_out = 64 - data->currentBoard.moveCount;

	if (heu_depth_out > data->maxDepth) heu_depth_out = data->maxDepth;
	if (bit_depth_out > data->maxDepth) bit_depth_out = data->maxDepth;
	if (bit_depth_out == 0) bit_depth_out = 1;
	if (exact_tail_out == 0) exact_tail_out = 1;

	data->updatedBoard = false;
}

// This is the core of the engine, will be running from creation to
// destruction and will be multithreading to evaluate the current 
// position set by the constructor or updatePosition().

void EngineConnect4::main_loop() const
{
	DATA* data = (DATA*)threadedData;

	// Worker struct that envelops worker threads for the main loop.
	struct HWorker
	{
	private:
		DATA* data = nullptr;
		unsigned int idx = 0;
		Thread		thread;
		bool		request = false;
		bool		busy = false;

	public:
		bool* get_request_pointer()
		{
			return &request;
		}

		inline void initialize(unsigned int idx, DATA* data)
		{
			this->idx = idx;
			this->data = data;
		}

		inline bool is_sleeping() const
		{
			return !busy;
		}

		inline bool is_busy() const
		{
			return busy;
		}

		inline void sleep()
		{
			request = true;
			thread.join();
			request = false;

			busy = false;
		}

		inline bool run_heuristic_tree(bool* exact_killer)
		{
			sleep();
			busy = true;

			HeuristicData* H_DATA = new HeuristicData;
			*H_DATA = data->H_DATA;
			H_DATA->STOP = &request;

			if (thread.start(&main_loop_worker_heuristicSolver, data->currentBoard, data->HEURISTIC_DEPTH, H_DATA, &busy, &data->finding_solution, &data->solution_found, exact_killer))
			{
				thread.set_name(L"Worker %u: Depth %u HEURISTIC H%llu", idx, data->HEURISTIC_DEPTH, data->currentBoard.hash);
				thread.set_affinity(CPU_FLAG(idx));
				thread.set_priority(Thread::PRIORITY_HIGHEST);
				return true;
			}
			else sleep();
			return false;
		}

	} h_worker;
	h_worker.initialize(0, data);

	struct EWorker
	{
	private:
		DATA* data = nullptr;
		unsigned int idx = 0;
		Thread		thread;
		bool		request = false;
		bool		busy = false;

	public:
		bool* get_request_pointer()
		{
			return &request;
		}

		inline void initialize(unsigned int idx, DATA* data)
		{
			this->idx = idx;
			this->data = data;
		}

		inline bool is_sleeping() const
		{
			return !busy;
		}

		inline bool is_busy() const
		{
			return busy;
		}

		inline void sleep()
		{
			request = true;
			thread.join();
			request = false;

			busy = false;
		}

		inline bool run_exact_tree(bool* heuristic_killer)
		{
			sleep();
			busy = true;

			HeuristicData* H_DATA = new HeuristicData;
			*H_DATA = data->H_DATA;
			H_DATA->STOP = &request;

			if (thread.start(&main_loop_worker_exactSolver, data->currentBoard, data->EXACT_DEPTH, H_DATA, &busy, &data->finding_solution, &data->solution_found, heuristic_killer))
			{
				thread.set_name(L"Worker %u: Depth %u EXACT H%llu", idx, data->EXACT_DEPTH, data->currentBoard.hash);
				thread.set_affinity(CPU_FLAG(idx));
				thread.set_priority(Thread::PRIORITY_HIGHEST);
				return true;
			}
			else sleep();
			return false;
		}
	} e_worker;
	e_worker.initialize(0, data);

	// Here Starts the main loop itself and will be running until terminate is called.
	// Terminate is called only at class destruction.

	while (!data->terminate)
	{
		// The loop will be waiting in a suspended state if it is suspended or solved.

		if (data->suspended || data->solution_found)
			e_worker.sleep(), h_worker.sleep();

		// Intentionally idle while stitching solution.

		else if (data->finding_solution);

		// If the board has been updated every worker goes to work from scratch.
		// The TTs are the same, if it is a continuation it will catch up quickly.

		else if (data->updatedBoard)
		{
			update_ML_DATA();
			h_worker.run_heuristic_tree(e_worker.get_request_pointer());
			e_worker.run_exact_tree(h_worker.get_request_pointer());
		}

		// It checks wether the heuristic worker has finished and can do more trees.

		else if (h_worker.is_sleeping() && data->HEURISTIC_DEPTH++ < data->maxDepth)
		{
			update_ML_DATA();
			h_worker.run_heuristic_tree(e_worker.get_request_pointer());
		}

		// It checks whether the exact worker has finished and can do more trees.

		else if (e_worker.is_sleeping() && data->EXACT_DEPTH++ < data->maxDepth)
		{
			update_ML_DATA();
			e_worker.run_exact_tree(h_worker.get_request_pointer());
		}

		// It will wait a little bit after every loop unless it is awakened.

		Thread::waitForWakeUp(CALL_MAINLOOP, MAINLOOP_WAIT_TIME_MS);
	}

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

// Constructor, it calls the main loop to start analizing the position.
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

	thread->start(&EngineConnect4::thread_entry, this);
	thread->set_priority(Thread::PRIORITY_NORMAL);
	thread->set_name(L"Engine Main Loop");
}

// Constructor, it calls the main loop to start analizing the board.
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
}

// Allows the continuation of the main loop.
// If an engine has started suspended, resume has to be called
// for it to begin evaluating the position.

void EngineConnect4::resume() const
{
	DATA* data = (DATA*)threadedData;

	if(is_win(data->currentBoard.playerBitboard[0]) || is_win(data->currentBoard.playerBitboard[1]))
		return;

	data->suspended = false;
	Thread::wakeUpThreads(CALL_MAINLOOP);
}

// Sends a new position to the engine that will be called for evaluation.
// Returns true if updated correctly, returns false if invalid position.

bool EngineConnect4::update_position(const Connect4* newPosition)
{
	DATA* data = (DATA*)threadedData;

	if (newPosition)
	{
		Board* board = translateBoard(*newPosition);

		if (!board)
			return false;

		if (board->hash == data->currentBoard.hash ||
			// Makes sure the side to play at a given turn is the same as the first board provided
			board->moveCount % 2 != (board->sideToPlay + data->first_player) % 2)
		{
			delete board;
			return false;
		}

		data->currentBoard = *board;
		delete board;
	}
	else
	{
		if (data->currentBoard.hash == INITIAL_HASH)
			return false;

		data->currentBoard = Board();
	}

	data->updatedBoard = true;
	data->finding_solution = false;
	data->suspended = data->solution_found = is_win(data->currentBoard.playerBitboard[0]) || 
											 is_win(data->currentBoard.playerBitboard[1]) || 
												    data->currentBoard.moveCount == 64;
	data->deadline = 0.f;

	Thread::wakeUpThreads(CALL_MAINLOOP);

	return true;
}

// Sends a new position to the engine that will be called for evaluation.
// Returns true if updated correctly, returns false if invalid position.

bool EngineConnect4::update_position(const Board* board)
{
	DATA* data = (DATA*)threadedData;

	if (board)
	{
		if (invalidBoard(*board))
			return false;

		if (board->hash == data->currentBoard.hash ||
			// Makes sure the side to play at a given turn is the same as the first board provided
			board->moveCount % 2 != (board->sideToPlay + data->first_player) % 2)
			return false;

		data->currentBoard = *board;
	}
	else
	{
		if (data->currentBoard.hash == INITIAL_HASH)
			return false;

		data->currentBoard = Board();
	}

	data->updatedBoard = true;
	data->finding_solution = false;
	data->suspended = data->solution_found = is_win(data->currentBoard.playerBitboard[0]) || 
											 is_win(data->currentBoard.playerBitboard[1]) || 
												    data->currentBoard.moveCount == 64;
	data->deadline = 0.f;

	Thread::wakeUpThreads(CALL_MAINLOOP);

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
// enter a new position or leave it at nullptr to mantain current position.
// If it finds a forced win it will return immediately.

PositionEval EngineConnect4::evaluate_for(float seconds, const Connect4* position)
{
	DATA* data = (DATA*)threadedData;

	if(position)
		update_position(position);

	data->deadline = data->timer.check() + seconds;

	while (data->deadline - data->timer.check() > 0.f && !data->solution_found)
		Thread::waitForWakeUp(CALL_MAINLOOP, 1UL);

	return get_evaluation();
}

// It updates the current board, and it returns a board evaluation after
// a specified time. If it finds a forced win it will return immediately.

PositionEval EngineConnect4::evaluate_for(float seconds, const Board* board)
{
	DATA* data = (DATA*)threadedData;

	if(board)
		update_position(board);

	data->deadline = data->timer.check() + seconds;

	while (data->deadline - data->timer.check() > 0.f && !data->solution_found)
		Thread::waitForWakeUp(CALL_MAINLOOP, 1UL);

	return get_evaluation();
}

// The new position is introduced and will not return a PositionEval until the depth of the
// evaluation is at least the specified or the position is solved, nullptr for current position.

PositionEval EngineConnect4::evaluate_until_depth(unsigned char heuristicDept, const Connect4* position)
{
	DATA* data = (DATA*)threadedData;

	if (position)
		update_position(position);

	HTTEntry* e = data->H_DATA.HTT[data->currentBoard.moveCount].storedBoard(data->currentBoard.hash);

	while ((!e || e->heuDepth < heuristicDept) && !data->solution_found)
	{
		e = data->H_DATA.HTT[data->currentBoard.moveCount].storedBoard(data->currentBoard.hash);
		Thread::waitForWakeUp(CALL_MAINLOOP, MAINLOOP_WAIT_TIME_MS);
	}

	return obtainPosEvalFromEntry(e);
}

// The new board is introduced and will not return a PositionEval until the depth
// of the evaluation is at least the specified or the board is solved.

PositionEval EngineConnect4::evaluate_until_depth(unsigned char heuristicDept, const Board* board)
{
	DATA* data = (DATA*)threadedData;

	update_position(board);

	HTTEntry* e = data->H_DATA.HTT[data->currentBoard.moveCount].storedBoard(data->currentBoard.hash);

	while ((!e || e->heuDepth < heuristicDept) && !data->solution_found)
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

// Sets a depth limit for the board evaluation, by default it will take no limit and
// evaluate until the position is solved, it is suspended or the engine is destroyed.

void EngineConnect4::setMaxDepth(unsigned char heuristicDepth) const
{
	DATA* data = (DATA*)threadedData;
	if (heuristicDepth == UNLIMITED_DEPTH || heuristicDepth > START_DEPTH_H)
	{
		data->maxDepth = heuristicDepth;

		Thread::wakeUpThreads(CALL_MAINLOOP);
	}

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
