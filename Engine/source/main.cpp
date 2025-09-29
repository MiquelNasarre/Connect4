#include "Timer.h"
#include "Thread.h"
#include <stdio.h>

void threadedFunction(float timediference, int* n, bool* cutoff)
{
	Timer timer;
	timer.reset();
	while (!(*cutoff))
	{
		while (timer.check() < timediference)
			Timer::sleep_for_us(50);

		timer.mark();
		(*n)++;
	}
	Thread::waitForWakeUp();
}

int main()
{
	int n = 0;
	bool cutoff = false;
	Thread thread;
	thread.start_suspended(&threadedFunction, 0.2f, &n, &cutoff);
	thread.set_name(L"PatataThread");
	char c;
	while (!thread.has_finished())
	{
		scanf("%c", &c);
		if (c == 'n')
			printf("%i", n);

		if (c == 's')
		{
			cutoff = true;
			thread.join(2000);
			printf("%lu", thread.get_exit_code());
		}

		if (c == 'r')
			thread.resume();

		if (c == 'b')
		{
			Thread::wakeUpThreads();
			thread.join(2000);
			printf("%lu", thread.get_exit_code());
		}

		if (c == 'v')
		{
			Thread::wakeUpThreads(1);
			thread.join(2000);
			printf("%lu", thread.get_exit_code());
		}
	}
}