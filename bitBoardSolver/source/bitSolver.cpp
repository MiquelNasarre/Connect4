#include "bitSolver.h"
#include "tt.h"

// Preferred move order for better pruning depending on the first column

constexpr unsigned char moveOrder[8][8] =
{
	{ 0,3,4,2,5,1,6,7 },
	{ 1,3,4,2,5,6,0,7 },
	{ 2,3,4,5,1,6,0,7 },
	{ 3,4,2,5,1,6,0,7 },
	{ 4,3,2,5,1,6,0,7 },
	{ 5,3,4,2,1,6,0,7 },
	{ 6,3,4,2,5,1,0,7 },
	{ 7,3,4,2,5,1,6,0 },
};

// The transposition table used by solve board is defined as a global 
// so that the function retrieve column can also access it.

static inline TransTable* TT = nullptr;

// ----------------------------------------------------------------------------------------------------------
// 
// The next two funtions solve the board using alpha-beta pruning
// The idea behind alpha-beta pruning is to eliminate nodes that are not worth checking.
// A simple example would be:
// Imagine I have a move that allows me to draw, and when I am analyzing my next move I see 
// that you can draw that move to. Then since the outcome of that other move is not any better,
// there is no point in analyzing it any further. Cutting off most of the board evaluation at first move.
//
// "Beta" is the best score my opponent has against me from the previous positions,
// if I can beat that score there is no point in analyzing any further since 
// he will not play this move allowing me a better outcome than a previous move would.
// 
// "Alpha" is my highest score from previous positions, and will get uptaded while analizing 
// the current avaliable moves, if my oponent beats it further down the line, there is no need
// to keep evaluating that line since I will not choose it, I have a better option.
//
// That way you keep the scores you are actually evaluating in between your best score and your 
// opponents best score, since surpassing your opponents best score or viceversa, are lines 
// that will not occurr since either of you can choose the better one.
//
// If you find a score more favorable for you than "beta", your opponent will favor the line that 
// gves him "beta" score. And if your oponent can find a score more favorable for him than "alpha",
// you will favor the line that gives you score "alpha", therefore making those cases irrelevant.
//
// ----------------------------------------------------------------------------------------------------------

// This is just a tiny helper funtion very useful for reconaissance if you are doing a very big tree
// it can cut a lot of branches off, but too expensive for any small tree.

static inline SolveResult tinyAlphaBetaTree(Board& board, SolveResult alpha, SolveResult beta, unsigned char depth)
{
	if (!depth)
		return DRAW;

	SolveResult best = OTHER_PLAYER_WIN;
	for (unsigned char column : moveOrder[3])
	{
		if (!canPlay(board, column))
			continue;

		if (is_win(board.playerBitboard[board.sideToPlay] | bit_at(column, board.heights[column])))
			return CURRENT_PLAYER_WIN;

		playMove(board, column);
		const SolveResult score = -tinyAlphaBetaTree(board, -beta, -alpha, depth - 1);
		undoMove(board, column);

		if (score > best)
		{
			best = score;

			if (best > alpha)
			{
				alpha = best;
				if (alpha >= beta)
					return best;
			}
		}
	}
	return best;
}

// This is the main function of this file.
// It solves a given position up to a certain depth, returns win, loss or draw.
// Uses transposition tables, with best move ordering and alpha beta pruning

static inline SolveResult alphaBetaTreeSolve(Board& board, SolveResult alpha, SolveResult beta, unsigned char depth, TransTable* TT)
{
	// First using the transposition tables we want to check if
	// we have already encountered this position before.
	// 
	// If we computed the entire position we return the value we obtained
	// 
	// If we know that the lower bound is higher or equal than beta we return the value
	// Else if it is bigger than our current alpha we adjust alpha
	// 
	// If we know that our upper bound is lower than our alpha we return the value
	// Else if it is smaller than our current beta we adjust beta

	unsigned char colTT = 3u;

	TTEntry* storedData = TT[board.moveCount].storedBoard(board.hash);

	if (storedData)
	{
		SolveResult score = (SolveResult)storedData->score;
#ifdef DEPTH_TESTING
		if (score)
			return score;

		if (storedData->depth >= depth) 
#endif
			switch (storedData->flag)
			{
			case ENTRY_FLAG_EXACT: // Exact value known
				return score;

			case ENTRY_FLAG_LOWER: // Lower bound (at least)
				if (score >= beta) return score;
				if (score > alpha) alpha = score;
				break;

			case ENTRY_FLAG_UPPER: // Upper bound (up most)
				if (score <= alpha) return score;
				if (score < beta) beta = score;
				break;
			}

		// Tree cutoff

		if (depth == 1u || board.moveCount == 63u)
			return DRAW;

		if (storedData->bestCol != 255)
			colTT = storedData->bestCol;

	}
	else
	{
		// Checks if a move is winning for the current player or the other player
		// Returns wins from our player and counts possible wins of the other player
		// Wins are checked here so there is no need to check them anywhere else

		for (unsigned char column : moveOrder[colTT])
		{
			if (!canPlay(board, column))
				continue;

			const uint64_t s = bit_at(column, board.heights[column]);

			if (is_win(board.playerBitboard[board.sideToPlay] | s))
			{
				TT[board.moveCount].store(board.hash, depth, CURRENT_PLAYER_WIN, ENTRY_FLAG_EXACT, column);
				return CURRENT_PLAYER_WIN;
			}
		}

		// Tree cutoff

		if (depth == 1u || board.moveCount == 63u)
		{
			TT[board.moveCount].store(board.hash, depth, DRAW, ENTRY_FLAG_EXACT);
			return DRAW;
		}
	}

	// Usual alphe-beta pruning recursive algorithm
	// Moves are ordered starting from the column suggested by the TT

	unsigned char bestCol = colTT;
	SolveResult best = INVALID_BOARD;
	const SolveResult alpha0 = alpha;

	for (unsigned char column : moveOrder[colTT])
	{
		if (!canPlay(board, column))
			continue;

		playMove(board, column);
		const SolveResult score = -alphaBetaTreeSolve(board, -beta, -alpha, depth - 1, TT);
		undoMove(board, column);

		if (score > best)
		{
			best = score;
			bestCol = column;

			if (best > alpha)
			{
				alpha = best;
				if (alpha >= beta) 
					break;
			}
		}
	}

	// Before returning it always saves the position in the TT

	uint8_t ttFlag = ENTRY_FLAG_EXACT;
	if (best >= beta) ttFlag = ENTRY_FLAG_LOWER;
	else if (best <= alpha0) ttFlag = ENTRY_FLAG_UPPER;
	TT[board.moveCount].store(board.hash, depth, best, ttFlag, bestCol);

	return best;
}

// Solves the given board position up to a certain depth.
// It checks the validity of the board and then initializes the alpha-beta pruning tree.

SolveResult solveBoard(const Board& initialBoard, unsigned char depth, bool clearTT)
{
	if (!Z_PIECE[0][0])
		init_zobrist();

	if (invalidBoard(initialBoard))
		return INVALID_BOARD;

	if (is_win(initialBoard.playerBitboard[initialBoard.sideToPlay]))
		return CURRENT_PLAYER_WIN;

	if (is_win(initialBoard.playerBitboard[initialBoard.sideToPlay ^ 1]))
		return OTHER_PLAYER_WIN;

	Board board = initialBoard;

	if (depth > 64 - board.moveCount)
		depth = 64 - board.moveCount;

	if(!TT)
		TT = (TransTable*)calloc(64, sizeof(TransTable));

	for (int d = board.moveCount; d < board.moveCount + depth; d++)
		if (!TT[d].is_init())
			TT[d].init();
	
	if (clearTT)
		for (unsigned char d = 0; d < 64; d++)
			TT[d].clear();

	return alphaBetaTreeSolve(board, OTHER_PLAYER_WIN, CURRENT_PLAYER_WIN, depth, TT);
}

// Same as the previous one but assumes validity checks have been done.
// Used for win checks on bigger heuristic trees.

SolveResult noChecksSolveBoard(const Board& initialBoard, unsigned char depth)
{
	Board board = initialBoard;

	if (depth > 64 - board.moveCount)
		depth = 64 - board.moveCount;

	if(!TT)
		TT = (TransTable*)calloc(64, sizeof(TransTable));

	for (int d = board.moveCount; d < board.moveCount + depth; d++)
		if (!TT[d].is_init())
			TT[d].init();

	return alphaBetaTreeSolve(board, OTHER_PLAYER_WIN, CURRENT_PLAYER_WIN, depth, TT);
}

// If the position is stored on the transposition table it retrieves the best column.
// if the position is a mate situation it does not guarantee best path.

unsigned char retrieveColumn(const Board& board)
{
	if (!TT || !TT[board.moveCount].is_init())
		return 255;

	TTEntry* entry = TT[board.moveCount].storedBoard(board.hash);

	if (!entry)
		return 255;

	return entry->bestCol;
}

// If there is a forced win by any of the players it will find the best move.
// For the winning player is the one that wins the fastest.
// For the losing player is the one that delays the loss the longest.
// First value is the column second value is the distance.

unsigned char* findBestPath(const Board& board)
{
	Board b = board;

	TransTable* TTtemp = (TransTable*)calloc(64, sizeof(TransTable));

	SolveResult result = DRAW;
	unsigned char depth = 0u;
	while (!result)
	{
		depth++;
		for (int d = board.moveCount; d < board.moveCount + depth; d++)
			if (!TTtemp[d].is_init())
				TTtemp[d].init();

		result = alphaBetaTreeSolve(b, OTHER_PLAYER_WIN, CURRENT_PLAYER_WIN, depth, TTtemp);
	}
	unsigned char* solution = (unsigned char*)calloc(2, sizeof(char));
	solution[0] = TTtemp[board.moveCount].storedBoard(board.hash)->bestCol;
	solution[1] = depth;

	for (int d = board.moveCount; d < board.moveCount + depth; d++)
		if (TTtemp[d].is_init())
			TTtemp[d].erase();
	free(TTtemp);

	return solution;
}
