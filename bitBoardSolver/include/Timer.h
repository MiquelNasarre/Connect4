#pragma once

// Class taken from my 3D renderer program.
// Very useful for timing solves.

class Timer {
private:
	unsigned int MaxMarkers = 60u;
	void* last;
	void** Markers;

	void push(void* last);

public:
	Timer(bool precise = true);
	~Timer();
	void reset();
	float mark();
	float check();
	float skip();
	float average();
	int getSize();
	void setMax(unsigned int max);
};