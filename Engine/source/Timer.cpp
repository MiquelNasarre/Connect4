#include <Windows.h>
#include <timeapi.h>
#include "Timer.h"
#include <chrono>
#include <queue>

using namespace std::chrono;

void Timer::push(void* last)
{
	if (Markers[MaxMarkers - 1u])
		delete (std::chrono::steady_clock::time_point*)Markers[MaxMarkers - 1u];

	for (unsigned int i = MaxMarkers - 1u; i > 0; i--)
		Markers[i] = Markers[i - 1];

	Markers[0] = new std::chrono::steady_clock::time_point(*(std::chrono::steady_clock::time_point*)last);
}

//	Public

Timer::Timer(bool precise) :last{ new std::chrono::steady_clock::time_point }
{
	Markers = (void**)calloc(MaxMarkers, sizeof(void*));
	*(std::chrono::steady_clock::time_point*)last = std::chrono::steady_clock::now();
	if (precise)
		timeBeginPeriod(1);
}

Timer::~Timer()
{
	timeEndPeriod(1);
}

void Timer::reset()
{
	*(std::chrono::steady_clock::time_point*)last = high_resolution_clock::now();
	for (unsigned int i = 0; i < MaxMarkers; i++)
	{
		if (!Markers[i])
			break;
		delete (std::chrono::steady_clock::time_point*)Markers[i];
		Markers[i] = nullptr;
	}
	push(last);
}

float Timer::mark()
{
	const auto old = *(std::chrono::steady_clock::time_point*)last;
	*(std::chrono::steady_clock::time_point*)last = steady_clock::now();
	push(last);
	return duration<float>(*(std::chrono::steady_clock::time_point*)last - old).count();
}

float Timer::check()
{
	return duration<float>(steady_clock::now() - *(std::chrono::steady_clock::time_point*)last).count();
}

float Timer::skip()
{
	unsigned int m = 0;
	for (unsigned int i = 0; i < MaxMarkers; i++)
	{
		if (Markers[i])
			m = i;
		else break;
	}

	const auto old = *(std::chrono::steady_clock::time_point*)last;
	*(std::chrono::steady_clock::time_point*)last = steady_clock::now();
	for (unsigned int i = 0; i < m; i++)
		*(std::chrono::steady_clock::time_point*)Markers[i] += *(std::chrono::steady_clock::time_point*)last - old;
	return duration<float>(*(std::chrono::steady_clock::time_point*)last - old).count();
}

float Timer::average()
{
	if(!Markers[1])
		return 0.0f;
	
	unsigned int m = 0;
	for (unsigned int i = 0; i < MaxMarkers; i++)
	{
		if (Markers[i])
			m = i;
		else break;
	}

	return duration<float>(*(std::chrono::steady_clock::time_point*)Markers[0] - *(std::chrono::steady_clock::time_point*)Markers[m]).count() / float(m);
}

int Timer::getSize()
{
	unsigned int m = 0;
	for (unsigned int i = 0; i < MaxMarkers; i++)
	{
		if (Markers[i])
			m = i;
		else return m;
	}
	return m;
}

void Timer::setMax(unsigned int max)
{
	MaxMarkers = max;
}
