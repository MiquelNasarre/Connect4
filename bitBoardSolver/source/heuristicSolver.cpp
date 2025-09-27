#include "heuristicSolver.h"
#include "heuristictt.h"
#include "bitSolver.h"
#include "Timer.h"

#include <bit>

inline constexpr unsigned char moveOrder[8] = {3, 4, 2, 5, 1, 6, 0, 7};

float heuristic(const Board& board, unsigned char bitDepth)
{
	// Generates a tree under the position to check for wins or losses
	// This deepens the heuristic tree making the evaluation fail safe

	const SolveResult deepSolve = noChecksSolveBoard(board, bitDepth);
	if (deepSolve)
		return (float)deepSolve;

	// Here we flip the others board to represent all the possible 
	// spots where your pieces could go later in the game

	const uint64_t bitBoard0 = ~board.playerBitboard[board.sideToPlay ^ 1u];
	const uint64_t bitBoard1 = ~board.playerBitboard[board.sideToPlay];

	// Here, following the same method seen to check for a win, we compare how
	// many possible winning positions each player has, and return the difference

	char count = 0;

	uint64_t m0;
	uint64_t m1;

	// Horizontal (east = -8)
	m0 = bitBoard0 & (bitBoard0 << 8);
	m1 = bitBoard1 & (bitBoard1 << 8);
	m0 &= (m0 << 16) & collapseE;
	m1 &= (m1 << 16) & collapseE;
	count += (char)__popcnt64(m0) - (char)__popcnt64(m1);

	// Vertical (south = +1)
	m0 = bitBoard0 & (bitBoard0 >> 1);
	m1 = bitBoard1 & (bitBoard1 >> 1);
	m0 &= (m0 >> 2) & collapseS;
	m1 &= (m1 >> 2) & collapseS;
	count += (char)__popcnt64(m0) - (char)__popcnt64(m1);

	// Diagonal SW (+9)
	m0 = bitBoard0 & (bitBoard0 >> 9);
	m1 = bitBoard1 & (bitBoard1 >> 9);
	m0 &= (m0 >> 18) & collapseSW;
	m1 &= (m1 >> 18) & collapseSW;
	count += (char)__popcnt64(m0) - (char)__popcnt64(m1);

	// Diagonal SE (-7)
	m0 = bitBoard0 & (bitBoard0 << 7);
	m1 = bitBoard1 & (bitBoard1 << 7);
	m0 &= (m0 << 14) & collapseSE;
	m1 &= (m1 << 14) & collapseSE;
	count += (char)__popcnt64(m0) - (char)__popcnt64(m1);

	return count / 128.f; // This maintains the value under +-1 (max is 130 but impossible)
}

float heuristicTree(Board& board, float alpha, float beta, unsigned char depth, unsigned char bitDepth, HeuristicTransTable* HTT)
{
	if (board.moveCount == 64u)
		return DRAW;

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

	unsigned char order[8] = { 3, 4, 2, 5, 1, 6, 0, 7 };

	HTTEntry* storedData = HTT[board.moveCount].storedBoard(board.hash);

	if (storedData)
	{
		float score = storedData->eval;
#ifdef DEPTH_TESTING
		if (score == 1.f || score == -1.f)
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

		// Tree cutoff, returns heuristic

		if (!depth)
			return heuristic(board, bitDepth);

		order[0] = storedData->order[0];
		order[1] = storedData->order[1];
		order[2] = storedData->order[2];
		order[3] = storedData->order[3];
		order[4] = storedData->order[4];
		order[5] = storedData->order[5];
		order[6] = storedData->order[6];
		order[7] = storedData->order[7];
	}
	else
	{
		// Tree cutoff, returns heuristic

		if (!depth)
		{
			float score = heuristic(board, bitDepth);
			HTT[board.moveCount].store(board.hash, order, depth, score, ENTRY_FLAG_EXACT);
			return score;
		}

		// Checks if a move is winning for the current player or the other player
		// Returns wins from our player and counts possible wins of the other player
		// Wins are checked here so there is no need to check them anywhere else

		for (unsigned char i = 0; i < 8; i++)
		{
			unsigned char column = order[i];

			if (!canPlay(board, column))
				continue;

			if (is_win(board.playerBitboard[board.sideToPlay] | bit_at(column, board.heights[column])))
			{
				order[i] = order[0];
				order[0] = column;
				HTT[board.moveCount].store(board.hash, order, depth, CURRENT_PLAYER_WIN, ENTRY_FLAG_EXACT);
				return 1.f;
			}
		}
	}

	float best = OTHER_PLAYER_WIN;
	const float alpha0 = alpha;

	for (unsigned char i = 0; i < 8; i++)
	{
		unsigned char column = order[i];

		if (!canPlay(board, column))
			continue;

		playMove(board, column);
		const float score = -heuristicTree(board, -beta, -alpha, depth - 1, bitDepth, HTT);
		undoMove(board, column);

		if (score > best)
		{
			best = score;

			if (best > alpha)
			{
				for (unsigned char j = i; j > 0; j--)
					order[j] = order[j - 1];

				order[0] = column;

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
	HTT[board.moveCount].store(board.hash, order, depth, best, ttFlag);
	return best;
}

// Evaluates the given board position up to a certain depth
// It checks the validity of the board and then initializes the heuristic tree
// If it finds a winning move it reanalises the position to find the fastest path
// If if finds a losing move it reanalises the position to find the longest path

SolveEval evaluateBoard(const Board& initialBoard, unsigned char depth, unsigned char bitDepth)
{
	if (!Z_PIECE[0][0])
		init_zobrist();

	if (invalidBoard(initialBoard))
		return SolveEval(0.f,0,0,INVALID_BOARD);

	if (is_win(initialBoard.playerBitboard[initialBoard.sideToPlay]))
		return SolveEval(1.f, 8,0, CURRENT_PLAYER_WIN);

	if (is_win(initialBoard.playerBitboard[initialBoard.sideToPlay ^ 1]))
		return SolveEval(-1.f, 8,0, OTHER_PLAYER_WIN);

	Board board = initialBoard;
	
	// If the depth demanded is higher than the remaining move count
	// or the remaining move count is below a certain threshold
	// it will compute the entirity of the remaining board

	if (depth + bitDepth > 64 - board.moveCount || board.moveCount > MOVE_COUNT_TRIGGER)
	{
		float eval = (float)solveBoard(board, 64 - board.moveCount);
		unsigned char column = retrieveColumn(board);

		if (eval == 1.f)
		{
			unsigned char* quikest = findBestPath(board);
			column = quikest[0];
			depth = quikest[1];
			free(quikest);
			return SolveEval(eval, column, depth, CURRENT_PLAYER_WIN);
		}

		else if (eval == -1.f)
		{
			unsigned char* quikest = findBestPath(board);
			column = quikest[0];
			depth = quikest[1];
			free(quikest);
			return SolveEval(eval, column, depth, OTHER_PLAYER_WIN);
		}

		return SolveEval(eval, column, 255, DRAW);
	}

	// Here it initialises transposition tables if necessary and calls the tree creation
	// after the tree creation the best column is retrieved from the transposition table

	static HeuristicTransTable* HTT = (HeuristicTransTable*)calloc(64 + 1, sizeof(HeuristicTransTable));

	for (int d = board.moveCount; d < board.moveCount + depth + 1; d++)
		if (!HTT[d].entries)
			HTT[d].init();

	float eval = heuristicTree(board, OTHER_PLAYER_WIN, CURRENT_PLAYER_WIN, depth, bitDepth, HTT);

	unsigned char column = HTT[board.moveCount].probe(board.hash)->order[0];

	// Depending on the obtained evaluationg will return the value with a different flag
	// also if it is a Mate situation will find the best path for either player.

	if (eval == 1.f)
	{
		unsigned char* quikest = findBestPath(board);
		column = quikest[0];
		depth = quikest[1];
		free(quikest);
		return SolveEval(eval, column, depth, CURRENT_PLAYER_WIN);
	}
	else if (eval == -1.f)
	{
		unsigned char* quikest = findBestPath(board);
		column = quikest[0];
		depth = quikest[1];
		free(quikest);
		return SolveEval(eval, column, depth, OTHER_PLAYER_WIN);
	}
	else if (eval > 0.f)
		return SolveEval(eval, column, depth, CURRENT_PLAYER_BETTER);
	else if (eval < 0.f)
		return SolveEval(eval, column, depth, OTHER_PLAYER_BETTER);
	else
		return SolveEval(eval, column, depth, DRAW);
}

SolveEval evaluateBoardTime(const Board& initialBoard, float Max_time, unsigned char bitDepth)
{
	Timer timer;

	timer.reset();
	unsigned char depth = 3u;
	SolveEval eval = evaluateBoard(initialBoard, depth, bitDepth);
	while (
		initialBoard.moveCount < MOVE_COUNT_TRIGGER
		&& timer.check() < Max_time
		&& depth + bitDepth < 64 - initialBoard.moveCount
		&& eval.eval != 1.f
		&& eval.eval != -1.f
		)
		eval = evaluateBoard(initialBoard, ++depth, bitDepth);

	return eval;

}
