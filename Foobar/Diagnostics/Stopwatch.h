#pragma once

#include <chrono>

class Stopwatch
{
	using TimePoint = std::chrono::high_resolution_clock::time_point;
	using Nanoseconds = std::chrono::duration<long long, std::nano>;

public:
	long long ElapsedMilliseconds();
	void Reset();
	void Restart();
	void Start();
	void Stop();

private:
	Nanoseconds NanosecondsFromStart();

	TimePoint _startTime;
	Nanoseconds _elapsedNanoseconds = {};
	bool _started = false;
};