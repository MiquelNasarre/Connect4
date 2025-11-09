#pragma once

/* CONNECT4 THREADING ENGINE HEADER FILE
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This header contains the engine to evaluate Connect4 positions.
It encapsulates all the functions from the other files, and threads
the tree generation to allow for continous computation as well as 
multiple threads computing a position at the same time.

Its main tool is the heuristicTree which is used at increasing depths
to solve a given position. Currently the threads compute the same 
position at different depths. Ideally in the future every thread 
will compute a diferent part of the tree.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

#define UNLIMITED_DEPTH 255u

/*
-------------------------------------------------------------------------------------------------------
Other relevant structs to use the engine
-------------------------------------------------------------------------------------------------------
*/

// Struct that defines a Connect4 board. It stores the board configuration
// and the current side to play. 1 is player RED, 2 is player YELLOW.
// For indexs, rows go from bottom to top and columns from left to right.
struct Connect4
{
	unsigned char board[8][8] = { 0 };	// Current board configuration
	unsigned char player = 1u;			// Current side to play

	Connect4() = default;
	Connect4(const Connect4& other);
	Connect4& operator=(const Connect4& other);

	// This constructor copies a conventional 8x8 array as the board. It will
	// flip the array to put it in the struct prefered position, so enter an
	// array with normal visual arrangement, consult testBoards.h for examples.
	Connect4(const unsigned char position[8][8], unsigned char player);

#ifdef _CONSOLE
	// Print board function inherited from last Connect4
	// project I did. Do not know how it works.
	void console_fancy_print();
#endif
};

// Struct that will be return after calling for a board evaluation.
// Contains all the important data of the analized position.
struct PositionEval
{
	float eval;				// Value between -1 and 1, evaluation for the current player
	unsigned char column;	// Best column to throw at the given position
	unsigned char depth;	// Depth at which the position was evalued 
							// If the position is win/loss, depth is the distance with perfect play

	// Reflects the state of the positon, invalid board means the board data is not being stored.
	enum EvalFlag : char
	{
		CURRENT_PLAYER_WIN = 1,
		DRAW = 0,
		OTHER_PLAYER_WIN = -1,
		INVALID_BOARD = -2,
		CURRENT_PLAYER_BETTER = 2,
		OTHER_PLAYER_BETTER = 3,

	} flag = INVALID_BOARD;	
};

/*
-------------------------------------------------------------------------------------------------------
Engine class
-------------------------------------------------------------------------------------------------------
*/

// Core class of Engine.h. It continuously evaluates a Connect4 position.
// The evaluation is threaded, while it computes you can perform any other task.
// You can call for best moves, update position, suspend/resume the engine
// or update the engines max depth at any time.
class EngineConnect4
{
private:
	void* threadedData = nullptr;	// DATA* for threading masked as void* 

	// Before the creation of a tree a future Machine Learning algorithm
	// will analise which data it want the tree to work with. Including 
	// depth, exact tree depth, heuristic weights, number of workers, etc.
	void update_ML_DATA() const;

	// Helper static function used to send the main loop into a thread.
	static void thread_entry(EngineConnect4* this_)
	{
		this_->main_loop();
	}

	// This is the core of the engine, will be running from creation to
	// destruction and will be multithreading to evaluate the current 
	// position set by the constructor or updatePosition().
	void main_loop() const;

	// Function used to call terminate on the main loop and join it.
	void kill_main_loop() const;

	// No copies of an Engine are allowed.
	EngineConnect4(const EngineConnect4&) = delete;
	// No copies of an Engine are allowed.
	EngineConnect4& operator=(const EngineConnect4&) = delete;

/*
-------------------------------------------------------------------------------------------------------
Constructors/Destructors
-------------------------------------------------------------------------------------------------------
*/

public:
	// Constructor, it calls the main loop to start analizing the position.
	// If no position is provided it will default to initial position. If it
	// started suspended, resume needs to be called to start evaluating.
	EngineConnect4(const Connect4* Position = nullptr, bool start_suspended = false);

#ifdef BIT_BOARD
	// Constructor, it calls the main loop to start analizing the board.
	// If no board is provided it will default to initial position. If it
	// started suspended, resume needs to be called to start evaluating.
	EngineConnect4(const Board* board, bool start_suspended = false);
#endif

	// Destructor, calls kill_main_loop and frees the allocated data.
	~EngineConnect4();

/*
-------------------------------------------------------------------------------------------------------
User end functions
-------------------------------------------------------------------------------------------------------
*/

	// New evaluation trees will not be called in the main loop.
	// Current evaluation trees will be allowed to finish.
	void suspend() const;

	// Allows the continuation of the main loop.
	// If an engine has started suspended, resume has to be called
	// for it to begin evaluating the position.
	void resume() const;

	// Sends a new position to the engine that will be called for evaluation.
	// Returns true if updated correctly, returns false if invalid position.
	bool update_position(const Connect4* newPosition);

	// If the position is in memory it will return a PositionEval for that position.
	// If it is not in memory EvalFlag will be INVALID_BOARD, nullptr is current position.
	PositionEval get_evaluation(const Connect4* position = nullptr) const;

	// It returns a position evaluation after a certain time, you can either 
	// enter a new position or leave it at nullptr to mantain current position.
	// If it finds a forced win it will return immediately.
	PositionEval evaluate_for(float seconds_for_answer, const Connect4* position = nullptr);

	// The new position is introduced and will not return a PositionEval until the depth of the
	// evaluation is at least the specified or the position is solved, nullptr for current position.
	PositionEval evaluate_until_depth(unsigned char heuristicDept, const Connect4* position = nullptr);

	// Returns a Connect4 struct copy of the current position under evaluation.
	Connect4 get_current_position() const;

#ifdef BIT_BOARD
	// Sends a new position to the engine that will be called for evaluation.
	// Returns true if updated correctly, returns false if invalid position.
	bool update_position(const Board* board);

	// If the board is in memory it will return a PositionEval for that position.
	// If it is not in memory EvalFlag will be INVALID_BOARD.
	PositionEval get_evaluation(const Board* board) const;

	// It returns a position evaluation after a certain time, you can either 
	// enter a new board or leave it at nullptr to mantain current position.
	// If it finds a forced win it will return immediately.
	PositionEval evaluate_for(float seconds_for_answer, const Board* board);

	// The new board is introduced and will not return a PositionEval until the depth
	// of the evaluation is at least the specified or the board is solved.
	PositionEval evaluate_until_depth(unsigned char heuristicDept, const Board* board);

	// Returns a Board struct copy of the current position under evaluation.
	Board get_current_bitBoard() const;
#endif

	// Sets a depth limit for the board evaluation, by default it will take no limit and
	// evaluate until the position is solved, it is suspended or the engine is destroyed.
	void setMaxDepth(unsigned char heuristicDepth = UNLIMITED_DEPTH) const;

/*
-------------------------------------------------------------------------------------------------------
Static helpers
-------------------------------------------------------------------------------------------------------
*/

#ifdef BIT_BOARD
	// It takes a bitBoard and returns the equivalent Connect4 struct.
	static Connect4 decodeBoard(const Board& bitBoard);

	// It takes a Connect4 strunc and returns the equivalent bitBoard.
	// If the board position is invalid it returns nullptr.
	static Board* translateBoard(const Connect4& position);
#endif

};