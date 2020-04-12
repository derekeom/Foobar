#include "pch.h"
#include "CppUnitTest.h"
#include "../Foobar/src/Stopwatch.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

TEST_CLASS(StopwatchTest)
{
	const std::chrono::milliseconds default_sleep_time = std::chrono::milliseconds(10);
	const std::chrono::milliseconds time_epsilon = std::chrono::milliseconds(5);

public:
	TEST_METHOD(test_default_constructor)
	{
		stopwatch sw;

		Assert::IsFalse(sw.running());
		Assert::AreEqual(0LL, sw.elapsed_milliseconds());
	}

	TEST_METHOD(test_start)
	{
		stopwatch sw;
		sw.start();
		std::this_thread::sleep_for(default_sleep_time);

		Assert::IsTrue(sw.running());
		Assert::IsTrue((default_sleep_time - time_epsilon).count() < sw.elapsed_milliseconds());
		Assert::IsTrue((default_sleep_time + time_epsilon).count() > sw.elapsed_milliseconds());
	}

	TEST_METHOD(test_stop)
	{
		stopwatch sw;
		sw.start();
		std::this_thread::sleep_for(default_sleep_time);
		sw.stop();

		Assert::IsFalse(sw.running());
		Assert::IsTrue((default_sleep_time - time_epsilon).count() < sw.elapsed_milliseconds());
		Assert::IsTrue((default_sleep_time + time_epsilon).count() > sw.elapsed_milliseconds());
	}

	TEST_METHOD(test_start_twice)
	{
		stopwatch sw;
		sw.start();
		std::this_thread::sleep_for(default_sleep_time);
		sw.start();
		sw.stop();

		Assert::IsFalse(sw.running());
		Assert::IsTrue((default_sleep_time - time_epsilon).count() < sw.elapsed_milliseconds());
		Assert::IsTrue((default_sleep_time + time_epsilon).count() > sw.elapsed_milliseconds());
	}

	TEST_METHOD(test_stop_twice)
	{
		stopwatch sw;
		sw.start();
		sw.stop();
		auto expected = sw.elapsed_milliseconds();

		std::this_thread::sleep_for(default_sleep_time);
		sw.stop();
		
		Assert::IsFalse(sw.running());
		Assert::AreEqual(expected, sw.elapsed_milliseconds());
	}

	TEST_METHOD(test_reset)
	{
		stopwatch sw;
		sw.start();
		std::this_thread::sleep_for(default_sleep_time);
		sw.reset();
		std::this_thread::sleep_for(default_sleep_time);

		Assert::IsFalse(sw.running());
		Assert::AreEqual(0LL, sw.elapsed_milliseconds());
	}

	TEST_METHOD(test_restart)
	{
		stopwatch sw;
		sw.start();
		std::this_thread::sleep_for(default_sleep_time * 2);
		sw.restart();
		std::this_thread::sleep_for(default_sleep_time);

		Assert::IsTrue(sw.running());
		Assert::IsTrue((default_sleep_time - time_epsilon).count() < sw.elapsed_milliseconds());
		Assert::IsTrue((default_sleep_time + time_epsilon).count() > sw.elapsed_milliseconds());
	}
};
