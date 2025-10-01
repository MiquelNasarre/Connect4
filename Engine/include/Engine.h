#pragma once

#define UNLIMITED_DEPTH 255u

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

	enum EvalFlag : char
	{
		CURRENT_PLAYER_WIN = 1,
		DRAW = 0,
		OTHER_PLAYER_WIN = -1,
		INVALID_BOARD = -2,
		CURRENT_PLAYER_BETTER = 2,
		OTHER_PLAYER_BETTER = 3,

	} flag = INVALID_BOARD;	// Reflects the state of the positon, invalid board means the board data is not being stored.
};

// Core class of Engine.h. It continuously evaluates a Connect4 position.
// The evaluation is threaded, while it computes you can perform any other task.
// You can call for best moves, update position, suspend/resume the engine
// or update the engines max depth at any time.
class EngineConnect4
{
private:
	void* threadedData = nullptr;	// DATA* for threading masked as void* 

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

public:
	// Constructor, it calls the main loop to start analizing the position.
	// If no position is provided it will default to initial position. If it
	// started suspended, resume needs to be called to start evaluating.
	EngineConnect4(const Connect4* Position = nullptr, bool start_suspended = false);

#ifdef BIT_BOARD
	// Constructor, it calls the main loop to start analizing the board.
	// If no board is provided it will default to initial position. If it
	// started suspended, resume needs to be called to start evaluating.
	EngineConnect4(const Board* board = nullptr, bool start_suspended = false);
#endif

	// Destructor, calls kill_main_loop and frees the allocated data.
	~EngineConnect4();

#ifdef BIT_BOARD
	// It takes a bitBoard and returns the equivalent Connect4 struct.
	static Connect4 decodeBoard(const Board& bitBoard);

	// It takes a Connect4 strunc and returns the equivalent bitBoard.
	// If the board position is invalid it returns nullptr.
	static Board* translateBoard(const Connect4& position);
#endif

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

#ifdef BIT_BOARD
	// Sends a new position to the engine that will be called for evaluation.
	// Returns true if updated correctly, returns false if invalid position.
	bool update_position(const Board* board);
#endif

	// If the position is in memory it will return a PositionEval for that position.
	// If it is not in memory EvalFlag will be INVALID_BOARD.
	PositionEval getPositionEval(const Connect4* position = nullptr) const; // nullptr is current position

	// The introduces position is introduced and will not return a PositionEval until the 
	// depth of the evaluation is at least the specified or the position is solved.
	PositionEval EvalAtDepth(unsigned char heuristicDept, const Connect4* position = nullptr);

	// Sets a depth limit for the board evaluation, by default it will take no limit and
	// evaluate until the position is solved, it is suspended or the engine is destroyed.
	void setMaxDepth(unsigned char heuristicDepth = UNLIMITED_DEPTH) const;

	// Returns a Connect4 struct copy of the current position under evaluation.
	Connect4 getCurrentPosition() const;

#ifdef BIT_BOARD
	// Returns a Board struct copy of the current position under evaluation.
	Board getCurrentBitBoard() const;
#endif

};