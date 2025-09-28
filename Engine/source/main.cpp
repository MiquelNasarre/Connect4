#include "Timer.h"
#include "Thread.h"
#include <stdio.h>
#include <thread>
#include <chrono>

void threadedFunction(float timediference, int* n, bool* cutoff)
{
	Timer timer;
	timer.reset();
	while (!(*cutoff))
	{
		while (timer.check() < timediference)
			std::this_thread::sleep_for(std::chrono::milliseconds(5));

		timer.reset();
		(*n)++;
	}
}

int main()
{
	int n = 0;
	bool cutoff = false;
	Thread thread;
	thread.start_suspended(&threadedFunction, 1.f, &n, &cutoff);
	char c;
	while (true)
	{
		scanf("%c", &c);
		if (c == 'n')
			printf("%i", n);

		if (c == 's')
		{
			cutoff = true;
			thread.join();
			printf("%lu", thread.get_exit_code());
		}

		if (c == 'r')
			thread.resume();

		if (c == 'a')
			return 0;
	}
}