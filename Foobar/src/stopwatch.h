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
		auto elapsed_nano = _elapsed_nano + running_nanoseconds();
		auto elapsed_milli = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_nano);
		return elapsed_milli.count();
	}

	bool running() const
	{
		return _start_time != not_running;
	}

private:
	void set_not_running()
	{
		_start_time = not_running;
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

	static constexpr std::chrono::high_resolution_clock::time_point not_running
		= std::chrono::high_resolution_clock::time_point::min();
};
