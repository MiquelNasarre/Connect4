#include "heuristicSolver.h"
#include "Timer.h"

#include <bit>	// For __popcnt64()

#define YOU_WIN 1.f
#define OTHER_WIN -1.f

#define KILL_TEST 	if (DATA.STOP && *DATA.STOP)return alpha

// Default move order used for low depth branches.

#define DEFAULT_MOVE_ORDER { 3, 4, 2, 5, 1, 6, 0, 7 }

// Moment at which the evaluate board function switches
// to fully solving the position without heuristic.

#define MOVE_COUNT_TRIGGER 30

// Helper function for the heuristic. Following the same method seen to check 
// for a win, generates the connect4 masks for a give board and pop-counts 
// how many bits are in total, counting the amount of connect4s in a position.

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

// This function assigns a value to a given board position.
// It first generates a bitTree to look for wins and losses.
// If it doesn't find any it does the following computation.
//
// It computes how many connect4's each color could have assuming
// all the spaces in the board are occupied by that color, and it
// subtracts the two values, the difference is the board score.

static inline float heuristic(Board board, unsigned char depth, const HeuristicData& DATA)
{

	// Generates a tree under the position to check for wins or losses
	// This deepens the heuristic tree making the evaluation fail safe

	if (SolveResult deepSolve = exactTree(board, OTHER_PLAYER_WIN, CURRENT_PLAYER_WIN, depth, DATA.TT, DATA.STOP))
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

	return DATA.FAVORABLES * count_Favorables + DATA.POSSIBLES * count_Possibles + DATA.FAVORABLE_1MOVE * count_Favorable_1move + DATA.POSSIBLES_1MOVE * count_Possible_1move;
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

		while (heights[i] == 8u && lastCol > i)
		{
			tempCol = order[i];

			order[i] = order[lastCol];
			order[lastCol] = tempCol;

			heights[i] = heights[lastCol--];
		}

		// While height bigger than previous it swaps

		unsigned char j = i;
		while (j > 0 && heights[j] > heights[j - 1] && heights[j] != 8u)
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

static inline float orderMoves(Board& board, unsigned char order[8], unsigned char depth, const HeuristicData& DATA)
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
			return YOU_WIN;
		}

		playMove(board, column);
		scores[i] = -heuristic(board, depth, DATA);
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

inline float heuristicTree(Board& board, float alpha, float beta, unsigned char depth, const HeuristicData& DATA)
{
	if (board.moveCount == 64u)
		return DRAW;

	KILL_TEST;

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

	HTTEntry* storedData = DATA.HTT[board.moveCount].storedBoard(board.hash);
	if (storedData)
	{
		float score = storedData->eval;

		if (score == 1.f || score == -1.f)
			return score;

		else if (storedData->heuDepth >= depth)
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
			return heuristic(board, DATA.EXACT_TAIL, DATA);


        // Copies move order stored

		if (storedData->heuDepth)
			for (int i = 0; i < 8; ++i)
				order[i] = storedData->order[i];
	}
	
	// If is does not have stored data of the position it means it is a first encounter
	// therefore ordering the nodes and checking for immediate wins can save a lot of 
	// unnecessary branching.
	// It is also important to order the nodes if the first encounter was of a diffrent
	// node ordering standard than now
	
	if (!storedData || !storedData->heuDepth || (storedData->heuDepth <= DATA.ORDERING_DEPTH && depth > DATA.ORDERING_DEPTH))
	{
		// If the depth is high enough it will make a small tree to order moves 
		// It also looks for forced wins or lossses, if it finds one it returns it.

		if (depth > DATA.ORDERING_DEPTH)
		{
			const float surfaceCheck = orderMoves(board, order, depth, DATA);
			KILL_TEST;

			if (surfaceCheck == 1.f || surfaceCheck == -1.f)
				return DATA.HTT[board.moveCount].store(board.hash, order, surfaceCheck, depth, DATA.EXACT_TAIL, ENTRY_FLAG_EXACT);
		}

		// Otherwise it orders the nodes by height, it performs a simple 
		// one move win check and it checks for a tree depth cutoff.

		else
		{
			// Tree cutoff, returns heuristic.

			if (!depth)
			{
				float eval = heuristic(board, DATA.EXACT_TAIL, DATA);
				KILL_TEST;
				return DATA.HTT[board.moveCount].store(board.hash, order, eval, depth, DATA.EXACT_TAIL, ENTRY_FLAG_EXACT);;
			}

			// It orders the moves from highest column to lowest column.
			// Leaving invalid moves at the end.

			orderByHeight(board.heights, order);

			// Checks if a move is winning for the current player.
			// Wins are checked here so there is no need to check them anywhere else.

			for (unsigned char i = 0; i < 8; i++)
			{
				unsigned char column = order[i];

				// If a move is invalid it has reached the end due to ordering.

				if (!canPlay(board, column))
					break;

				// Checks if positioning a piece in this column results in an instant win, returns if thats the case

				if (is_win(board.playerBitboard[board.sideToPlay] | bit_at(column, board.heights[column])))
				{
					order[i] = order[0];
					order[0] = column;
					return DATA.HTT[board.moveCount].store(board.hash, order, YOU_WIN, depth, DATA.EXACT_TAIL, ENTRY_FLAG_EXACT);
				}

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
	float score = -heuristicTree(board, -beta, -alpha, depth - 1, DATA);  // full window
	KILL_TEST;
	undoMove(board, order[0]);

	float best = score;
	if (best > alpha)
	{
		alpha = best;
		if (alpha >= beta)
			return DATA.HTT[board.moveCount].store(board.hash, order, best, depth, DATA.EXACT_TAIL, (alpha == YOU_WIN) ? ENTRY_FLAG_EXACT : ENTRY_FLAG_LOWER);
	}

	for (unsigned char i = 1; i < 8; i++)
	{
		unsigned char column = order[i];

		if (!canPlay(board, column))
			break;

		playMove(board, column);
		float score = -heuristicTree(board, -alpha - POINT_DISTANCE, -alpha, depth - 1, DATA); // probe
		KILL_TEST;
		if (score > alpha)
			score = -heuristicTree(board, -beta, -alpha, depth - 1, DATA);  // re-search at full window
		KILL_TEST;
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
					return DATA.HTT[board.moveCount].store(board.hash, order, best, depth, DATA.EXACT_TAIL, (alpha == YOU_WIN) ? ENTRY_FLAG_EXACT : ENTRY_FLAG_LOWER);
			}
		}
	}

	// Before returning it always saves the position in the TT

	return DATA.HTT[board.moveCount].store(board.hash, order, best, depth, DATA.EXACT_TAIL, (best <= alpha0 && best != OTHER_WIN) ? ENTRY_FLAG_UPPER : ENTRY_FLAG_EXACT);
}

// Evaluates the given board position up to a certain depth.
// It checks the validity of the board and then initializes the heuristic tree.
// If it finds a winning move it reanalises the position to find the fastest path.
// If if finds a losing move it reanalises the position to find the longest path.

SolveEval evaluateBoard(const Board& initialBoard, unsigned char depth, HeuristicData const* DATA)
{
	init_zobrist();

	HeuristicData USING_DATA = {};
	if (DATA)
		USING_DATA = *DATA;

	if (invalidBoard(initialBoard))
		return SolveEval(0.f, 0, 0, INVALID_BOARD);

	if (is_win(initialBoard.playerBitboard[initialBoard.sideToPlay]))
		return SolveEval(1.f, 8,0, CURRENT_PLAYER_WIN);

	if (is_win(initialBoard.playerBitboard[initialBoard.sideToPlay ^ 1]))
		return SolveEval(-1.f, 8,0, OTHER_PLAYER_WIN);

	Board board = initialBoard;
	
	// If the depth demanded is higher than the remaining move count
	// or the remaining move count is below a certain threshold
	// it will compute the entirity of the remaining board

	if (depth + USING_DATA.EXACT_TAIL > 64 - board.moveCount || board.moveCount >= MOVE_COUNT_TRIGGER)
	{
		float eval = (float)solveBoard(board, 64 - board.moveCount, USING_DATA.TT);
		unsigned char column = retrieveColumn(board, USING_DATA.TT);

		if (eval == YOU_WIN)
		{
			char* quickest = findBestPath(board, CURRENT_PLAYER_WIN);
			column = quickest[0];
			depth = quickest[1];
			free(quickest);
			return SolveEval(YOU_WIN, column, depth, CURRENT_PLAYER_WIN);
		}

		else if (eval == OTHER_WIN)
		{
			char* quickest = findBestPath(board, OTHER_PLAYER_WIN);
			column = quickest[0];
			depth = quickest[1];
			free(quickest);
			return SolveEval(OTHER_WIN, column, depth, OTHER_PLAYER_WIN);
		}

		return SolveEval(eval, column, 255, DRAW);
	}

	// Here it initialises transposition tables if necessary and calls the tree creation
	// after the tree creation the best column is retrieved from the transposition table

	if (!USING_DATA.HTT)
	{
		static HeuristicTransTable* HTT = (HeuristicTransTable*)calloc(64 + 1, sizeof(HeuristicTransTable));
		USING_DATA.HTT = HTT;
	}

	for (int d = board.moveCount; d < board.moveCount + depth + 1; d++)
		if (!USING_DATA.HTT[d].is_init())
			USING_DATA.HTT[d].init();

	float eval = heuristicTree(board, OTHER_PLAYER_WIN, CURRENT_PLAYER_WIN, depth, USING_DATA);

	unsigned char column = USING_DATA.HTT[board.moveCount].storedBoard(board.hash)->order[0];

	// Depending on the obtained evaluationg will return the value with a different flag
	// also if it is a Mate situation will find the best path for either player.

	if (eval == YOU_WIN)
	{
		char* quickest = findBestPath(board, CURRENT_PLAYER_WIN);
		column = quickest[0];
		depth = quickest[1];
		free(quickest);
		return SolveEval(YOU_WIN, column, depth, CURRENT_PLAYER_WIN);
	}
	else if (eval == OTHER_WIN)
	{
		char* quikest = findBestPath(board, OTHER_PLAYER_WIN);
		column = quikest[0];
		depth = quikest[1];
		free(quikest);
		return SolveEval(OTHER_WIN, column, depth, OTHER_PLAYER_WIN);
	}
	else if (eval > 0.f)
		return SolveEval(eval, column, depth, CURRENT_PLAYER_BETTER);
	else if (eval < 0.f)
		return SolveEval(eval, column, depth, OTHER_PLAYER_BETTER);
	else
		return SolveEval(eval, column, depth, DRAW);
}
