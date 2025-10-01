#include "heuristicSolver.h"
#include "Engine.h"
#include "Thread.h"
#include "Timer.h"

#ifdef _CONSOLE
#include <stdio.h>
#endif

#define CALL_MAINLOOP			0
#define MAINLOOP_WAIT_TIME_MS	5
#define DEFAULT_EXACT_DEPTH		10
#define START_MAINLOOP_DEPTH	4

#define N_WORKERS				1
#define CPU_FLAG(idx)			15

/*
-------------------------------------------------------------------------------------------------------
Connect4 struct functions
-------------------------------------------------------------------------------------------------------
*/

inline Connect4::Connect4(const Connect4& other)
{
	for (unsigned char i = 0; i < 8; i++)
		for (unsigned char j = 0; j < 8; j++)
			this->board[i][j] = other.board[i][j];

	this->player = other.player;
}

inline Connect4& Connect4::operator=(const Connect4& other)
{
	for (unsigned char i = 0; i < 8; i++)
		for (unsigned char j = 0; j < 8; j++)
			this->board[i][j] = other.board[i][j];

	this->player = other.player;

	return *this;
}

#ifdef _CONSOLE
// Print board function inherited from last Connect4
// project I did. Do not know how it works.

void Connect4::console_fancy_print()
{
	constexpr int N = 8;
	char** T = (char**)calloc(N, sizeof(void*));
	for (unsigned char i = 0; i < N; i++)
	{
		T[i] = (char*)calloc(N, sizeof(char));
		for (unsigned char j = 0; j < N; j++)
			T[i][j] = (char)board[7 - i][j];
	}

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

struct DATA
{
	Board currentBoard = {};

	Thread* main_loop_thread = nullptr;

	TransTable* TT = nullptr;
	HeuristicTransTable* HTT = nullptr;

	bool suspended = false;
	bool terminate = false;

	bool updatedBoard = false;

	unsigned char first_player = 0;

	bool solution_found = false;

	unsigned char maxDepth = UNLIMITED_DEPTH;

	bool updated_n_workers = false;
	unsigned char n_workers = N_WORKERS;
};

/*
-------------------------------------------------------------------------------------------------------
Static helper functions
-------------------------------------------------------------------------------------------------------
*/

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

inline Board* EngineConnect4::translateBoard(const Connect4& position)
{
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

static inline PositionEval obtainPosEvalFromEntry(HTTEntry* e)
{
	if (!e) return PositionEval();

	PositionEval::EvalFlag flag = PositionEval::EvalFlag::DRAW;
	switch (e->flag)
	{
	case ENTRY_FLAG_EXACT:

		if (e->eval == 1.f)
			flag = PositionEval::EvalFlag::CURRENT_PLAYER_WIN;
		else if (e->eval > 0.f)
			flag = PositionEval::EvalFlag::CURRENT_PLAYER_BETTER;
		else if (e->eval < 0.f)
			flag = PositionEval::EvalFlag::OTHER_PLAYER_BETTER;
		else if (e->eval == -1.f)
			flag = PositionEval::EvalFlag::OTHER_PLAYER_WIN;

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

static inline void main_loop_worker_heuristicSolver(Board board, unsigned char depth, unsigned char bitDepth, HeuristicTransTable* HTT, TransTable* TT, bool* busy, bool* solved, bool* kill)
{
	// If there is not exact data about your board, you need to be able to overwrite it.
	if (HTTEntry* storedData = HTT[board.moveCount].storedBoard(board.hash))
		if (storedData->flag != ENTRY_FLAG_EXACT)
			*storedData = HTTEntry();

	float eval = heuristicTree(board, OTHER_PLAYER_WIN, CURRENT_PLAYER_WIN, depth, bitDepth, HTT, TT, kill);
	
	if (*kill)
		return;

	if (eval == 1.f || eval == -1.f)
	{
		unsigned char* solution = findBestPath(board, eval == 1.f ? CURRENT_PLAYER_WIN : OTHER_PLAYER_WIN, kill);
		if (*kill)
			return;

		HTTEntry* e = HTT[board.moveCount].probe(board.hash);

		e->bitDepth = solution[1];
		e->heuDepth = 0;

		for (unsigned char c = 0; c < 8; c++)
		{
			if (e->order[c] == solution[0])
			{
				e->order[c] = e->order[0];
				e->order[0] = solution[0];
				break;
			}
		}
		free(solution);
		*solved = true;
	}

	*busy = false;

	Thread::wakeUpThreads(CALL_MAINLOOP);
}

/*
-------------------------------------------------------------------------------------------------------
Main Loop functions
-------------------------------------------------------------------------------------------------------
*/

void EngineConnect4::main_loop() const
{
	init_zobrist();
	DATA* data = (DATA*)threadedData;

	struct Worker
	{
	private:
		unsigned int idx = 0;
		Thread		thread;
		bool		request = false;
		bool		busy = false;

	public:
		inline void set_idx(unsigned int idx)
		{
			this->idx = idx;
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

		inline bool do_a_tree(DATA* data, unsigned char depth)
		{
			sleep();

			if (thread.start(&main_loop_worker_heuristicSolver, data->currentBoard, depth, DEFAULT_EXACT_DEPTH, data->HTT, data->TT, &busy, &data->solution_found, &request))
			{
				thread.set_name(L"Worker %u: Depth %u hTree H%llu", idx, depth, data->currentBoard.hash);
				thread.set_affinity(CPU_FLAG(idx));
				thread.set_priority(Thread::PRIORITY_HIGHEST);
				busy = true;
				return true;
			}
			else sleep();
			return false;
		}

	}workers[N_WORKERS];
	for (unsigned int i = 0; i < N_WORKERS; i++)
		workers[i].set_idx(i);

	// Here Starts the main loop itself and will be running until terminate is called.
	// Terminate is called only at class destruction.

	unsigned char depth_to_do = START_MAINLOOP_DEPTH;
	while (!data->terminate)
	{
		// The loop will be waiting in a suspended state if it is suspended.

		if (data->suspended);

		// If the board has been updated every worker goes to work from scratch.
		// The TTs are the same, if it is a continuation it will catch up quickly.

		else if (data->updatedBoard)
		{
			data->updatedBoard = false;
			depth_to_do = START_MAINLOOP_DEPTH;

			for (Worker& w : workers)
				w.do_a_tree(data, depth_to_do++);
		}

		else for (Worker& w : workers)
		{
			if (data->solution_found)
				w.sleep();

			else if (w.is_sleeping() && depth_to_do <= data->maxDepth)
				if (w.do_a_tree(data, depth_to_do))
					depth_to_do++;
		}

		Thread::waitForWakeUp(CALL_MAINLOOP, MAINLOOP_WAIT_TIME_MS);
	}

	for (Worker& w : workers)
		w.sleep();
}

void EngineConnect4::kill_main_loop() const
{
	DATA* data = (DATA*)threadedData;

	data->terminate = true;
	Thread::wakeUpThreads(CALL_MAINLOOP);

	data->main_loop_thread->join();
	delete data->main_loop_thread;

	return;
}

EngineConnect4::EngineConnect4(const Connect4* position, bool start_suspended)
{
	threadedData = (void*)new DATA;

	DATA* data = (DATA*)threadedData;
	Thread* thread = data->main_loop_thread = new Thread;

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

	data->TT = new TransTable[64];
	data->HTT = new HeuristicTransTable[64];

	thread->start(&EngineConnect4::thread_entry, this);
	thread->set_priority(Thread::PRIORITY_NORMAL);
	thread->set_name(L"Engine Main Loop");
}

EngineConnect4::EngineConnect4(const Board* board, bool start_suspended)
{
	threadedData = (void*)new DATA;

	DATA* data = (DATA*)threadedData;
	Thread* thread = data->main_loop_thread = new Thread;

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

	data->TT = new TransTable[64];
	data->HTT = new HeuristicTransTable[64];

	thread->start(&EngineConnect4::thread_entry, this);
	thread->set_priority(Thread::PRIORITY_NORMAL);
	thread->set_name(L"Engine Main Loop");
}

EngineConnect4::~EngineConnect4()
{
	kill_main_loop();

	DATA* data = (DATA*)threadedData;

	delete[] data->TT;
	delete[] data->HTT;

	delete data;
}

void EngineConnect4::suspend() const
{
	DATA* data = (DATA*)threadedData;
	data->suspended = true;
}

void EngineConnect4::resume() const
{
	DATA* data = (DATA*)threadedData;

	if(is_win(data->currentBoard.playerBitboard[0]) || is_win(data->currentBoard.playerBitboard[1]))
		return;

	data->suspended = false;
	Thread::wakeUpThreads(CALL_MAINLOOP);
}

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
	data->suspended = data->solution_found = is_win(data->currentBoard.playerBitboard[0]) || is_win(data->currentBoard.playerBitboard[1]);

	Thread::wakeUpThreads(CALL_MAINLOOP);

	return true;
}

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
	data->suspended = data->solution_found = is_win(data->currentBoard.playerBitboard[0]) || is_win(data->currentBoard.playerBitboard[1]);

	Thread::wakeUpThreads(CALL_MAINLOOP);

	return true;
}

PositionEval EngineConnect4::getPositionEval(const Connect4* position) const
{
	DATA* data = (DATA*)threadedData;

	Board* board = &data->currentBoard;
	if (position)
		board = translateBoard(*position);

	if (board)
	{
		PositionEval eval = obtainPosEvalFromEntry(data->HTT[board->moveCount].storedBoard(board->hash));
		if (position)
			delete board;
		return eval;
	}

	return PositionEval();
}

PositionEval EngineConnect4::EvalAtDepth(unsigned char heuristicDept, const Connect4* position)
{
	DATA* data = (DATA*)threadedData;

	update_position(position);

	HTTEntry* e = data->HTT[data->currentBoard.moveCount].storedBoard(data->currentBoard.hash);

	while ((!e || e->heuDepth < heuristicDept) && !data->solution_found)
	{
		e = data->HTT[data->currentBoard.moveCount].storedBoard(data->currentBoard.hash);
		Thread::waitForWakeUp(CALL_MAINLOOP, MAINLOOP_WAIT_TIME_MS);
	}

	return obtainPosEvalFromEntry(e);
}

void EngineConnect4::setMaxDepth(unsigned char heuristicDepth) const
{
	DATA* data = (DATA*)threadedData;
	if (heuristicDepth == UNLIMITED_DEPTH || heuristicDepth > START_MAINLOOP_DEPTH)
	{
		data->maxDepth = heuristicDepth;

		Thread::wakeUpThreads(CALL_MAINLOOP);
	}

}

Connect4 EngineConnect4::getCurrentPosition() const
{
	return decodeBoard(((DATA*)threadedData)->currentBoard);
}

Board EngineConnect4::getCurrentBitBoard() const
{
	return ((DATA*)threadedData)->currentBoard;
}

