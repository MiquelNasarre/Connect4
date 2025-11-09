#include "bitBoard.h"
#include "Interface.h"
#include "Timer.h"

#include <stdio.h>
#include <stdlib.h>

void playAgainstEngine(float engine_time_per_move, Connect4 position, unsigned char play_as, bool clear_console)
{
	Timer timer;
	
	char you = 1;
	if (play_as != 1)
		you = 2;

	printf("Initial Board:\n\n\n\n");

	position.console_fancy_print();
	Board bitBoard = *EngineConnect4::translateBoard(position);

	EngineConnect4 engine(&position);

	while (!is_win(bitBoard.playerBitboard[0]) && !is_win(bitBoard.playerBitboard[1]) && bitBoard.moveCount < 64)
	{
		if (you - 1 == bitBoard.sideToPlay)
		{
			unsigned char column;
			printf("\nChoose a column:  ");
			scanf("%hhu", &column);
			if (column == 9)
			{
				you = 3 - you;
				continue;
			}
			if (!column || column > 8 || !canPlay(bitBoard, column - 1))
			{
				printf("Invalid column!!");
				continue;
			}
			playMove(bitBoard, column - 1);

			if (clear_console)
				system("cls");

			printf("You have played at column %u\n\n\n\n", column);
		}
		else
		{
			timer.reset();
			PositionEval eval = engine.evaluate_for(engine_time_per_move);
			float time = timer.check();

			if(clear_console)
				system("cls");

			printf("The code has chosen column %u after thinking for %.3fs \nIt evaluates the position as ", eval.column + 1, time);

			switch (eval.flag)
			{
			case CURRENT_PLAYER_WIN:
				printf("winning for %s.", you == 1 ? "YELLOW" : "RED");
				break;
			case OTHER_PLAYER_WIN:
				printf("winning for %s.", you == 1 ? "RED" : "YELLOW");
				break;
			case CURRENT_PLAYER_BETTER:
				printf("better for %s.", you == 1 ? "YELLOW" : "RED");
				break;
			case OTHER_PLAYER_BETTER:
				printf("better for %s.", you == 1 ? "RED" : "YELLOW");
				break;
			case DRAW:
				printf("a draw.");
				break;
			}

			printf("\nValue: %.3f \n", eval.eval);

			if (eval.eval == 1.f || eval.eval == -1.f)
			{
				if (eval.depth != 255)
					printf("Distance to mate: %hhu\n", eval.depth - 1);
				else
					printf("Distance to mate: Unknown");
			}
			else if (eval.depth + bitBoard.moveCount >= 64 || eval.depth == 255)
				printf("Depth: Until the end of the game\n");
			else
				printf("Depth: %hhu\n", eval.depth);

			playMove(bitBoard, eval.column);

		}

		engine.update_position(&bitBoard);
		engine.get_current_position().console_fancy_print();
	}

	if (is_win(bitBoard.playerBitboard[0]))
		printf("\nPlayer RED has won!!\n");

	else if (is_win(bitBoard.playerBitboard[1]))
		printf("\nPlayer YELLOW has won!!\n");

	else
		printf("\nThe game has ended in a draw\n");
}

char engineAgainstEngine(float engine_time_per_move, Connect4 position, bool clear_console)
{
	Timer timer;

	printf("Initial Board:\n\n\n\n");

	position.console_fancy_print();

	Board bitBoard = *EngineConnect4::translateBoard(position);

	EngineConnect4 engine(&position);

	while (!is_win(bitBoard.playerBitboard[0]) && !is_win(bitBoard.playerBitboard[1]) && bitBoard.moveCount < 64)
	{
		timer.reset();
		PositionEval eval = engine.evaluate_for(engine_time_per_move);
		float time = timer.check();

		if (clear_console)
			system("cls");

		printf("The code has chosen column %u after thinking for %.3fs \nIt evaluates the position as ", eval.column + 1, time);

		switch (eval.flag)
		{
		case CURRENT_PLAYER_WIN:
			printf("winning for %s.", bitBoard.sideToPlay == 1 ? "YELLOW" : "RED");
			break;
		case OTHER_PLAYER_WIN:
			printf("winning for %s.", bitBoard.sideToPlay == 1 ? "RED" : "YELLOW");
			break;
		case CURRENT_PLAYER_BETTER:
			printf("better for %s.", bitBoard.sideToPlay == 1 ? "YELLOW" : "RED");
			break;
		case OTHER_PLAYER_BETTER:
			printf("better for %s.", bitBoard.sideToPlay == 1 ? "RED" : "YELLOW");
			break;
		case DRAW:
			printf("a draw.");
			break;
		}

		printf("\nValue: %.3f \n", eval.eval);

		if (eval.eval == 1.f || eval.eval == -1.f)
		{
			if (eval.depth != 255)
				printf("Distance to mate: %hhu\n", eval.depth - 1);
			else
				printf("Distance to mate: Unknown");
		}
		else if (eval.depth + bitBoard.moveCount >= 64 || eval.depth == 255)
			printf("Depth: Until the end of the game\n");
		else
			printf("Depth: %hhu\n", eval.depth);

		playMove(bitBoard, eval.column);

		engine.update_position(&bitBoard);
		engine.get_current_position().console_fancy_print();

	}

	if (is_win(bitBoard.playerBitboard[0]))
	{
		printf("\nPlayer RED has won!!\n");
		return 0;
	}


	else if (is_win(bitBoard.playerBitboard[1]))
	{
		printf("\nPlayer YELLOW has won!!\n");
		return 1;
	}


	else
	{
		printf("\nThe game has ended in a draw\n");
		return -1;
	}


}
