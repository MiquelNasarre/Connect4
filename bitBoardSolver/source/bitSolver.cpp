#include "bitSolver.h"
#include <bit>

// Macro to get the column from the column mask

#define _ctz(mask) (((mask >> 1) & 1) | ((mask >> 2) & 1) * 2 | ((mask >> 3) & 1) * 3 | ((mask >> 4) & 1) * 4 | ((mask >> 5) & 1) * 5 | ((mask >> 6) & 1) * 6 | ((mask >> 7) & 1) * 7) 

// Preferred move order for better pruning
// If using transposition table, prefered move order depending on best column

#ifdef usingTT

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

#else

constexpr unsigned char moveOrder[8] = { 3,4,2,5,1,6,0,7 };

#endif

// Checks if the board is valid
// By checking if there are any floating pieces
// and if the move count matches the pieces on the board

static inline bool invalidBoard(const Board& board)
{
	unsigned char moveCount = 0;

	if(board.playerBitboard[0] & board.playerBitboard[1])
		return true;

#ifdef usingTT
	if (board.hash != boardHash(board.playerBitboard))
		return true;
#endif

	uint64_t boardMask = mask(board);
	for (unsigned char column = 0; column < 8; column++)
	{
		const uint8_t height = board.heights[column];

		if(height > 8)
			return true;

		moveCount += height;

		uint64_t expected = ((1ULL << height) - 1) << (8 * column);
		if ((boardMask & COL_MASK(column)) != expected)
			return true;
	}

	if (moveCount != board.moveCount)
		return true;

	return false;
}

// Solves the board using alpha-beta pruning
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

static inline SolveResult alphaBetaTreeSolve(Board& board, SolveResult alpha, SolveResult beta, unsigned char depth)
{

	// First if we are using transposition tables we want to check if
	// we have already encountered this position before.
	// 
	// If we computed the entire position we return the value we obtained
	// 
	// If we know that the lower bound is higher or equal than beta we return the value
	// Else if it is bigger than our current alpha we adjust alpha
	// 
	// If we know that our upper bound is lower than our alpha we return the value
	// Else if it is smaller than our current beta we adjust beta

#ifdef usingTT
	unsigned char colTT = 3u;
	TTEntry* storedData = TT.storedBoard(board.hash);
	if (storedData)
	{
		SolveResult score = (SolveResult)storedData->score;

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
		if (storedData->bestCol != 255)
			colTT = storedData->bestCol;
	}
#endif

	// Checks if a move is winning for the current player or the other player
	// Returns wins from our player and counts possible wins of the other player
	// Wins are checked here so there is no need to check them anywhere else

	uint16_t oppWinsMask = 0u;
#ifdef usingTT
	for (unsigned char column : moveOrder[colTT])
#else
	for (unsigned char column : moveOrder)
#endif
	{
		if (!canPlay(board, column))
			continue;

		const uint64_t s = bit_at(column, board.heights[column]);

		if (is_win(board.playerBitboard[board.sideToPlay] | s))
		{
#ifdef usingTT
			TT.store(board.hash, depth, CURRENT_PLAYER_WIN, ENTRY_FLAG_EXACT);
#endif
			return CURRENT_PLAYER_WIN;
		}

		if (is_win(board.playerBitboard[board.sideToPlay ^ 1] | s))
			oppWinsMask |= 1u << column;
	}

	// Checks if the other player has winning moves:
	// If it has more than one the other player wins
	// If it has one our player is forced to block it

	if (oppWinsMask)
	{
		const uint16_t oppWins = __popcnt16(oppWinsMask);
		if (oppWins >= 2u)
		{
#ifdef usingTT
			TT.store(board.hash, depth, OTHER_PLAYER_WIN, ENTRY_FLAG_EXACT);
#endif
			return OTHER_PLAYER_WIN;
		}


		// Tree cutoff
		if (depth - 1 == 0 || board.moveCount == 63)
			return DRAW;

		const unsigned char column = _ctz(oppWinsMask);
		playMove(board, column);
		const SolveResult score = (SolveResult)-alphaBetaTreeSolve(board, (SolveResult)-beta, (SolveResult)-alpha, depth - 1);
		undoMove(board, column);

#ifdef usingTT
		uint8_t ttFlag = ENTRY_FLAG_EXACT;
		if (score >= beta) ttFlag = ENTRY_FLAG_LOWER;
		else if (score <= alpha) ttFlag = ENTRY_FLAG_UPPER;
		TT.store(board.hash, depth, score, ttFlag);
#endif
		return score;
	}

	// Tree cutoff

	if (depth - 1 == 0 || board.moveCount == 63)
		return DRAW;

	// Usual alphe-beta pruning recursive algorithm

#ifdef usingTT
	unsigned char bestCol = 255u;
#endif
	SolveResult best = OTHER_PLAYER_WIN;
	const SolveResult alpha0 = alpha;

#ifdef usingTT
	for (unsigned char column : moveOrder[colTT])
#else
	for (unsigned char column : moveOrder)
#endif
	{
		if(!canPlay(board, column))
			continue;

		playMove(board, column);
		const SolveResult score = -alphaBetaTreeSolve(board, -beta, -alpha, depth - 1);
		undoMove(board, column);

		if (score > best)
		{
			best = score;
#ifdef usingTT
			bestCol = column;
#endif
		}

		if (best > alpha) 
			alpha = best;

		if (alpha >= beta) 
			break;
	}

#ifdef usingTT
	uint8_t ttFlag = ENTRY_FLAG_EXACT;
	if (best >= beta) ttFlag = ENTRY_FLAG_LOWER;
	else if (best <= alpha0) ttFlag = ENTRY_FLAG_UPPER;
	TT.store(board.hash, depth, best, ttFlag, bestCol);
#endif
	return best;
}

// Solves the given board position up to a certain depth
// It checks the validity of the board and then initializes the alpha-beta pruning tree

SolveResult solveBoard(const Board& initialBoard, unsigned char depth)
{
#ifdef usingTT
	if (!Z_PIECE[0][0])
		init_zobrist();
#endif

	if (invalidBoard(initialBoard))
		return SolveResult::INVALID_BOARD;

	if (is_win(initialBoard.playerBitboard[initialBoard.sideToPlay]))
		return CURRENT_PLAYER_WIN;

	if (is_win(initialBoard.playerBitboard[initialBoard.sideToPlay ^ 1]))
		return OTHER_PLAYER_WIN;

#ifdef usingTT
	if (!TT.entries)
		TT.init();
#endif

	Board board = initialBoard;

	if (depth > 64 - board.moveCount)
		depth = 64 - board.moveCount;

	SolveResult score = alphaBetaTreeSolve(board, SolveResult::OTHER_PLAYER_WIN, SolveResult::CURRENT_PLAYER_WIN, depth);

#ifdef clearTT
	TT.clear();
#endif

	return score;
}
