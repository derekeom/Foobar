#pragma once

#include <chrono>

class stopwatch
{
public:
	stopwatch();

	void reset();
	void restart();
	void start();
	void stop();

	long long elapsed_milliseconds() const;
	bool running() const;

private:
	void set_not_running();
	void set_running();
	std::chrono::nanoseconds running_nanoseconds() const;

	std::chrono::high_resolution_clock::time_point _startTime;
	std::chrono::nanoseconds _elapsedNanoseconds;
};

inline stopwatch::stopwatch()
{
	reset();
}

inline void stopwatch::reset()
{
	_elapsedNanoseconds = std::chrono::nanoseconds::zero();
	set_not_running();
}

inline void stopwatch::restart()
{
	reset();
	start();
}

inline void stopwatch::start()
{
	if (!running())
	{
		set_running();
	}
}

inline void stopwatch::stop()
{
	_elapsedNanoseconds += running_nanoseconds();
	set_not_running();
}

inline long long stopwatch::elapsed_milliseconds() const
{
	auto elapsedNano = _elapsedNanoseconds + running_nanoseconds();
	auto elapsedMilli = std::chrono::duration_cast<std::chrono::milliseconds>(elapsedNano);
	return elapsedMilli.count();
}

inline bool stopwatch::running() const
{
	return _startTime != std::chrono::high_resolution_clock::time_point::min();
}

inline void stopwatch::set_not_running()
{
	_startTime = std::chrono::high_resolution_clock::time_point::min();
}

inline void stopwatch::set_running()
{
	_startTime = std::chrono::high_resolution_clock::now();
}

inline std::chrono::nanoseconds stopwatch::running_nanoseconds() const
{
	return running()
		? std::chrono::high_resolution_clock::now() - _startTime
		: std::chrono::nanoseconds::zero();
}

