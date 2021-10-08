#pragma once

#include <chrono>
#include <cstdint>

class stopwatch
{
public:
	stopwatch()
	{
		reset();
	}

	void reset()
	{
		_elapsed_nano = std::chrono::nanoseconds::zero();
		set_not_running();
	}

	void restart()
	{
		reset();
		start();
	}

	void start()
	{
		if (!running())
		{
			set_running();
		}
	}

	void stop()
	{
		_elapsed_nano += running_nanoseconds();
		set_not_running();
	}

	int64_t elapsed_milliseconds() const
	{
		const auto elapsed_nano = _elapsed_nano + running_nanoseconds();
		const auto elapsed_milli = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_nano);
		return elapsed_milli.count();
	}

	int64_t elapsed_nanoseconds() const
	{
		const auto elapsed_nano = _elapsed_nano + running_nanoseconds();
		return elapsed_nano.count();
	}

	bool running() const
	{
		return _start_time != NOT_RUNNING;
	}

private:
	void set_not_running()
	{
		_start_time = NOT_RUNNING;
	}

	void set_running()
	{
		_start_time = std::chrono::high_resolution_clock::now();
	}

	std::chrono::nanoseconds running_nanoseconds() const
	{
		return running()
			? std::chrono::high_resolution_clock::now() - _start_time
			: std::chrono::nanoseconds::zero();
	}

	std::chrono::high_resolution_clock::time_point _start_time;
	std::chrono::nanoseconds _elapsed_nano;

	static constexpr std::chrono::high_resolution_clock::time_point NOT_RUNNING
		= std::chrono::high_resolution_clock::time_point::min();
};
