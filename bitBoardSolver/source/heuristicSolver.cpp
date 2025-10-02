#include "heuristicSolver.h"
#include "heuristictt.h"
#include "bitSolver.h"
#include "Timer.h"

#include <stdio.h>

#include <bit>

// This is the depth value at which the heuristic tree
// stops calling the function orderMoves.

#define NO_ORDERING_DEPTH 2

// This value is the multiplier for the heuristic function value.
// It has to be small enough to mantain the result between +-1.

#define POINT_DISTANCE 0x1.0p-7f // 1/128 in hexadecimal floating-point

// Default move order used for low depth branches.

#define DEFAULT_MOVE_ORDER { 3, 4, 2, 5, 1, 6, 0, 7 }

// Moment at which the evaluate board function switches
// to fully solving the position without heuristic.

#define MOVE_COUNT_TRIGGER 30

// This function assigns a value to a given board position.
// It first generates a bitTree to look for wins and losses.
// If it doesn't find any it does the following computation.
//
// It computes how many connect4's each color could have assuming
// all the spaces in the board are occupied by that color, and it
// subtracts the two values, the difference is the board score.

static inline int _countConnect4s(const uint64_t board)
{
	uint64_t m;
	int count = 0;

	// Horizontal (east = -8)
	m = board & SHIFT_EAST(board);
	m &= SHIFT_2_EAST(m) & MASK_3E;
	count += (int)__popcnt64(m);

	// Vertical (south = +1)
	m = board & SHIFT_SOUTH(board);
	m &= SHIFT_2_SOUTH(m) & MASK_3S;
	count += (int)__popcnt64(m);

	// Diagonal SW (+9)
	m = board & SHIFT_SW(board);
	m &= SHIFT_2_SW(m) & MASK_3SW;
	count += (int)__popcnt64(m);

	// Diagonal SE (-7)
	m = board & SHIFT_SE(board);
	m &= SHIFT_2_SE(m) & MASK_3SE;
	count += (int)__popcnt64(m);

	return count;
}

static inline float heuristic0(const Board& board, unsigned char bitDepth)
{
	// Generates a tree under the position to check for wins or losses
	// This deepens the heuristic tree making the evaluation fail safe

	const SolveResult deepSolve = noChecksSolveBoard(board, bitDepth);
	if (deepSolve)
		return (float)deepSolve;

	// Here we flip the others board to represent all the possible 
	// spots where your pieces could go later in the game

	const uint64_t bitBoard0 = board.playerBitboard[board.sideToPlay];
	const uint64_t bitBoard1 = board.playerBitboard[board.sideToPlay ^ 1u];

	const uint64_t _bitBoard0 = ~bitBoard1;
	const uint64_t _bitBoard1 = ~bitBoard0;

	// Here, following the same method seen to check for a win, we compare how
	// many possible winning positions each player has, and return the difference

	return (_countConnect4s(_bitBoard0) - _countConnect4s(_bitBoard1)) * POINT_DISTANCE; // This maintains the value under +-1 (max is 130 but impossible)
}

static inline float heuristic1(const Board& board, const unsigned char bitDepth)
{
	// Generates a tree under the position to check for wins or losses
	// This deepens the heuristic tree making the evaluation fail safe

	const SolveResult deepSolve = noChecksSolveBoard(board, bitDepth);
	if (deepSolve)
		return (float)deepSolve;

	// Here we flip the others board to represent all the possible 
	// spots where your pieces could go later in the game

	const uint64_t bitBoard0 = board.playerBitboard[board.sideToPlay];
	const uint64_t bitBoard1 = board.playerBitboard[board.sideToPlay ^ 1u];

	const uint64_t _bitBoard0 = ~bitBoard1;
	const uint64_t _bitBoard1 = ~bitBoard0;

	const uint64_t bmask = mask(board);
	const uint64_t holes = ~bmask;

	const uint64_t up1_bmask = SHIFT_NORTH(bmask) | Row0;
	const uint64_t up1_holes = ~up1_bmask;

	// This are all the 1-move wins (3 in a row) that each player has.
	// Those at floor level are discarded because they will either be an immediate win
	// or immediately covered, and since deepSolve did not find a win, they are covered.

	const uint64_t winningHoles0 = COLLAPSE_ALL_WINNING_MOVES(bitBoard0) & up1_holes;
	const uint64_t winningHoles1 = COLLAPSE_ALL_WINNING_MOVES(bitBoard1) & up1_holes;

	// This are holes which by their nature it is impossible for something to be built on top.
	// Shared 1 move win, and two vertical one movers in a row.

	const uint64_t barrierHoles = (winningHoles0 & winningHoles1) | (winningHoles0 & SHIFT_NORTH(winningHoles0)) | (winningHoles0 & SHIFT_NORTH(winningHoles1));

	// Now we want to mark the region above all the 1 move holes.

	const uint64_t favorableArea0 = ~FILL_UP_SPACES(winningHoles1);
	const uint64_t favorableArea1 = ~FILL_UP_SPACES(winningHoles0);

	const uint64_t possibleArea = ~FILL_UP_SPACES(barrierHoles);

	// Now we run all the connect 4 we want to compute.

	const uint64_t favorable_bitBoard0 = _bitBoard0 & favorableArea0 & possibleArea;
	const uint64_t favorable_bitBoard1 = _bitBoard1 & favorableArea1 & possibleArea;

	const uint64_t possible_bitBoard0 = _bitBoard0 & possibleArea;
	const uint64_t possible_bitBoard1 = _bitBoard1 & possibleArea;

	const uint64_t favorable_1move_bitBoard0 = winningHoles0 & favorableArea0 & possibleArea;
	const uint64_t favorable_1move_bitBoard1 = winningHoles1 & favorableArea1 & possibleArea;

	const uint64_t possible_1move_bitBoard0 = winningHoles0 & possibleArea;
	const uint64_t possible_1move_bitBoard1 = winningHoles1 & possibleArea;

	// Here, following the same method seen to check for a win, we compare how
	// many possible winning positions each player has, and return the difference

	const char count_Favorables = _countConnect4s(favorable_bitBoard0) - _countConnect4s(favorable_bitBoard1);
	const char count_Possibles = _countConnect4s(possible_bitBoard0) - _countConnect4s(possible_bitBoard0);
	const char count_Favorable_1move = (char)__popcnt64(favorable_1move_bitBoard0) - (char)__popcnt64(favorable_1move_bitBoard1);
	const char count_Possible_1move = (char)__popcnt64(possible_1move_bitBoard0) - (char)__popcnt64(possible_1move_bitBoard1);

#define WEIGHT_FAVORABLES		(1.f * POINT_DISTANCE)
#define WEIGHT_POSSIBLES		(2.f * POINT_DISTANCE)
#define WEIGHT_FAVORABLE_1MOVE	(1.f * POINT_DISTANCE)
#define WEIGHT_POSSIBLES_1MOVE	(2.f * POINT_DISTANCE)

	return WEIGHT_FAVORABLES * count_Favorables + WEIGHT_POSSIBLES * count_Possibles + WEIGHT_FAVORABLE_1MOVE * count_Favorable_1move + WEIGHT_POSSIBLES_1MOVE * count_Possible_1move;
}

// This function orders the moves by height from highest to lowest.
// Also putting the invalid moves at the end.
// This helps with pruning because most of the time the moves at 
// the low positions are not good because they lose or they invalidate
// a winning opportunity.

static inline void orderByHeight(const unsigned char boardHeights[8], unsigned char order[8])
{
    unsigned char heights[8];
    for (unsigned char i = 0; i < 8; ++i)
        heights[i] = boardHeights[order[i]];

	unsigned char lastCol = 7;
	unsigned char tempCol, tempHeight;

	for (unsigned char i = 0; i <= lastCol; i++)
	{
		// If max height reached it sends it to the end.

		while (heights[i] == 8u)
		{
			tempCol = order[i];

			order[i] = order[lastCol];
			order[lastCol] = tempCol;

			heights[i] = heights[lastCol--];
		}

		// While height bigger than previous it swaps

		unsigned char j = i;
		while (j > 0 && heights[j] > heights[j - 1])
		{
			tempHeight = heights[j];
			tempCol = order[j];

			heights[j] = heights[j - 1];
			heights[j - 1] = tempHeight;

			order[j] = order[j - 1];
			order[j-- - 1] = tempCol;
		}
	}
}

// This function is called for trees bigger than a certain depth.
// It generates a heuristic tree of depth 1 and orders the moves
// according to their score, greatly improving pruning.

static inline float orderMoves(Board& board, unsigned char order[8], unsigned char depth, HeuristicTransTable* HTT, unsigned char heuristic)
{
	float scores[8] = { INVALID_BOARD, INVALID_BOARD, INVALID_BOARD, INVALID_BOARD, INVALID_BOARD, INVALID_BOARD, INVALID_BOARD, INVALID_BOARD };

	for (unsigned char i = 0; i < 8; i++)
	{
		unsigned char column = order[i];

		if (!canPlay(board, column))
			continue;

		if (is_win(board.playerBitboard[board.sideToPlay] | bit_at(column, board.heights[column])))
		{
			order[i] = order[0];
			order[0] = column;
			return HTT[board.moveCount].store(board.hash, order, depth, CURRENT_PLAYER_WIN, ENTRY_FLAG_EXACT);
		}

		playMove(board, column);
		if(heuristic == 0)
			scores[i] = -heuristic0(board, depth);
		else 
			scores[i] = -heuristic1(board, depth);
		undoMove(board, column);

		unsigned char j = i;
		float tempScore;
		unsigned char tempColumn;

		while (j > 0 && scores[j] > scores[j - 1])
		{
			tempScore = scores[j];
			tempColumn = order[j];

			scores[j] = scores[j - 1];
			scores[j - 1] = tempScore;

			order[j] = order[j - 1];
			order[j - 1] = tempColumn;

			j--;
		}
	}
	return scores[0];
}

// This is the main function of the file, computing the tree to solve the position
// to a certain depth, and telling the heuristic to search further with the bitTree
// to a certain bitDepth.
// The tree for better performance uses transposition tables, that check wheter the 
// position has already been examined and its indormation is stored in the database.
// 
// It also uses alpha-beta pruning, this time paired with node ordering and PVS,
// which added to the transposition tables makes the pruning a lot more efficient.

static inline float heuristicTree(Board& board, float alpha, float beta, unsigned char depth, unsigned char bitDepth, HeuristicTransTable* HTT, unsigned char heuristic)
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

	unsigned char order[8] = DEFAULT_MOVE_ORDER;

	HTTEntry* storedData = HTT[board.moveCount].storedBoard(board.hash);

	if (storedData)
	{
		float score = storedData->eval;

		if (score == 1.f || score == -1.f)
			return score;

		if (storedData->depth >= depth)
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
		{
			if (heuristic == 0)
				return heuristic0(board, bitDepth);
			else
				return heuristic1(board, bitDepth);
		}


        // Copies move order stored

		if (storedData->depth)
			for (int i = 0; i < 8; ++i)
				order[i] = storedData->order[i];
	}
	
	// If is does not have stored data of the position it means it is a first encounter
	// therefore ordering the nodes and checking for immediate wins can save a lot of 
	// unnecessary branching.
	// It is also important to order the nodes if the first encounter was of a diffrent
	// node ordering standard than now
	
	if (!storedData || !storedData->depth || (storedData->depth <= NO_ORDERING_DEPTH && depth > NO_ORDERING_DEPTH))
	{
		// If the depth is high enough it will make a small tree to order moves 
		// It also looks for forced wins or lossses, if it finds one it returns it.

		if (depth > NO_ORDERING_DEPTH)
		{
			const float surfaceCheck = orderMoves(board, order, depth, HTT,heuristic);

			if (surfaceCheck == 1.f || surfaceCheck == -1.f)
				return HTT[board.moveCount].store(board.hash, order, depth, surfaceCheck, ENTRY_FLAG_EXACT);
		}

		// Otherwise it orders the nodes by height, it performs a simple 
		// one move win check and it checks for a tree depth cutoff.

		else
		{
			// Tree cutoff, returns heuristic.

			if (!depth)
				return HTT[board.moveCount].store(board.hash, order, depth, heuristic ? heuristic1(board, bitDepth) : heuristic0(board, bitDepth), ENTRY_FLAG_EXACT);

			// It orders the moves from highest column to lowest column.
			// Leaving invalid moves at the end.

			orderByHeight(board.heights, order);

			// Checks if a move is winning for the current player.
			// Wins are checked here so there is no need to check them anywhere else.

			for (unsigned char column : order)
			{
				// If a move is invalid it has reached the end due to ordering.

				if (!canPlay(board, column))
					break;

				// Checks if positioning a piece in this column results in an instant win, returns if thats the case

				if (is_win(board.playerBitboard[board.sideToPlay] | bit_at(column, board.heights[column])))
					return HTT[board.moveCount].store(board.hash, order, depth, CURRENT_PLAYER_WIN, ENTRY_FLAG_EXACT);
			}
		}

	}

	// Once the TT has been checked, the moves are ordered and no wins or losses 
	// have been found, the alpha beta pruning tree algorithm is used.
	// 
	// In this case we also implement PVS (Principal Variation Search), this only allows the 
	// first move to check on a full scope. Only allowing the rest of moves if they can surpass 
	// a certain threshold. Basically it closing the window for the moves to make sure they can 
	// actually do anything good and if they can it allows them to search the full scope.

	float alpha0 = alpha;
	playMove(board, order[0]);
	float score = -heuristicTree(board, -beta, -alpha, depth - 1, bitDepth, HTT, heuristic);  // full window
	undoMove(board, order[0]);

	float best = score;
	if (best > alpha)
	{
		alpha = best;
		if (alpha >= beta)
			return HTT[board.moveCount].store(board.hash, order, depth, best, ENTRY_FLAG_LOWER);
	}

	for (unsigned char i = 1; i < 8; i++)
	{
		unsigned char column = order[i];

		if (!canPlay(board, column))
			break;

		playMove(board, column);
		float score = -heuristicTree(board, -alpha - POINT_DISTANCE, -alpha, depth - 1, bitDepth, HTT, heuristic); // probe
		if (score > alpha)
			score = -heuristicTree(board, -beta, -alpha, depth - 1, bitDepth, HTT, heuristic);  // re-search at full window
		undoMove(board, column);

		if (score > best)
		{
			best = score;

			for (unsigned char j = i; j > 0; j--)
				order[j] = order[j - 1];
			order[0] = column;

			if (best > alpha)
			{
				alpha = best;
				if (alpha >= beta)
					return HTT[board.moveCount].store(board.hash, order, depth, best, ENTRY_FLAG_LOWER);
			}
		}
	}

	// Before returning it always saves the position in the TT

	return HTT[board.moveCount].store(board.hash, order, depth, best, (best <= alpha0) ? ENTRY_FLAG_UPPER : ENTRY_FLAG_EXACT);
}

// Evaluates the given board position up to a certain depth.
// It checks the validity of the board and then initializes the heuristic tree.
// If it finds a winning move it reanalises the position to find the fastest path.
// If if finds a losing move it reanalises the position to find the longest path.

SolveEval evaluateBoard(const Board& initialBoard, unsigned char depth, unsigned char bitDepth, unsigned char heuristic, HeuristicTransTable* HTT)
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

	if (depth + bitDepth > 64 - board.moveCount || board.moveCount >= MOVE_COUNT_TRIGGER)
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

	if (!HTT)
	{
		static HeuristicTransTable* backupHTT = (HeuristicTransTable*)calloc(64 + 1, sizeof(HeuristicTransTable));
		HTT = backupHTT;
	}


	for (int d = board.moveCount; d < 65; d++)
		if (!HTT[d].is_init())
			HTT[d].init();


	float eval = heuristicTree(board, OTHER_PLAYER_WIN, CURRENT_PLAYER_WIN, depth, bitDepth, HTT, heuristic);

	unsigned char column = HTT[board.moveCount].storedBoard(board.hash)->order[0];

	// Depending on the obtained evaluationg will return the value with a different flag
	// also if it is a Mate situation will find the best path for either player.

	if (eval == 1.f)
	{
		unsigned char* quikest = findWinningPath(board);
		column = quikest[0];
		depth = quikest[1];
		free(quikest);
		return SolveEval(eval, column, depth, CURRENT_PLAYER_WIN);
	}
	else if (eval == -1.f)
	{
		unsigned char* quikest = findLosingPath(board);
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

// Evaluates the given board with Iterative Deepening using the evaluateBoard funtion
// until a certain time threshold is reached or it finds a win.
// Then it returns the deepest evaluation computed.

SolveEval evaluateBoardTime(const Board& initialBoard, float Max_time, unsigned char bitDepth, unsigned char heuristic)
{
	static HeuristicTransTable* HTT = (HeuristicTransTable*)calloc(64 + 1, sizeof(HeuristicTransTable));

	Timer timer;

	timer.reset();
	unsigned char depth = 3u;
	SolveEval eval = evaluateBoard(initialBoard, depth, bitDepth, heuristic, HTT);
	while (
		initialBoard.moveCount < MOVE_COUNT_TRIGGER
		&& timer.check() < Max_time
		&& depth + bitDepth < 64 - initialBoard.moveCount
		&& eval.eval != 1.f
		&& eval.eval != -1.f
		)
		eval = evaluateBoard(initialBoard, ++depth, bitDepth, heuristic, HTT);

	for (int d = initialBoard.moveCount; d < 65; d++)
		HTT[d].clear();

	return eval;

}
