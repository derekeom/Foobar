#include "Stopwatch.h"

long long Stopwatch::ElapsedMilliseconds()
{
	auto elapsedNano = _elapsedNanoseconds + NanosecondsFromStart();
	auto elapsedMilli = std::chrono::duration_cast<std::chrono::milliseconds>(elapsedNano);
	return elapsedMilli.count();
}

void Stopwatch::Reset()
{
	_elapsedNanoseconds = {};
	_started = false;
}

void Stopwatch::Restart()
{
	Reset();
	Start();
}

void Stopwatch::Start()
{
	if (!_started)
	{
		_startTime = std::chrono::high_resolution_clock::now();
		_started = true;
	}
}

void Stopwatch::Stop()
{
	_elapsedNanoseconds += NanosecondsFromStart();
	_started = false;
}

Stopwatch::Nanoseconds Stopwatch::NanosecondsFromStart()
{
	return _started
		? std::chrono::high_resolution_clock::now() - _startTime
		: Nanoseconds{};
}
