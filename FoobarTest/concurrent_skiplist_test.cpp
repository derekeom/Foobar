#include "pch.h"
#include "CppUnitTest.h"
#include "../Foobar/src/concurrent_skiplist.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

TEST_CLASS(concurrent_skiplist_test)
{
public:
	TEST_METHOD(test_add)
	{
		const int key = 1;
		const std::string value = std::to_string(key);

		concurrent_skiplist<int, std::string> sl;

		auto added = sl.try_add(key, value);
		Assert::IsTrue(added);

		std::string result;
		auto found = sl.try_get_value(key, &result);
		Assert::IsTrue(found);
		Assert::AreEqual(value, result);
	}

	TEST_METHOD(test_add_duplicate)
	{
		const int key = 1;
		const std::string value = std::to_string(key);
		const std::string new_value = std::to_string(key + 1);

		concurrent_skiplist<int, std::string> sl;
		sl.try_add(key, value);

		auto added = sl.try_add(key, new_value);
		Assert::IsFalse(added);

		std::string result;
		auto found = sl.try_get_value(key, &result);
		Assert::IsTrue(found);
		Assert::AreEqual(value, result);
	}

	TEST_METHOD(test_add_multiple)
	{
		const std::unordered_map<int, std::string> keyvaluepairs
		{
			{ 7, std::to_string(7) },
			{ 31, std::to_string(31) },
			{ 911, std::to_string(911) },
		};

		concurrent_skiplist<int, std::string> sl;

		for (const auto& kvp : keyvaluepairs)
		{
			auto added = sl.try_add(kvp.first, kvp.second);
			Assert::IsTrue(added);
		}

		std::string result;
		for (const auto& kvp : keyvaluepairs)
		{
			auto found = sl.try_get_value(kvp.first, &result);
			Assert::IsTrue(found);
			Assert::AreEqual(kvp.second, result);
		}
	}

	TEST_METHOD(test_add_concurrent)
	{
		const int threads = 10;
		const int count = threads * 5;

		concurrent_skiplist<int, int> sl;
		auto add = [&sl, count, threads](int id)
		{
			for (int i = id; i < count; i += threads)
			{
				if (!sl.try_add(i, i))
				{
					return false;
				}
			}

			return true;
		};

		std::vector<std::future<bool>> futures(threads);
		for (int i = 0; i < threads; ++i)
		{
			futures[i] = std::async(std::launch::async, add, i);
		}

		for (auto& f : futures)
		{
			Assert::IsTrue(f.get());
		}

		for (int i = 0; i < count; ++i)
		{
			int result;
			auto found = sl.try_get_value(i, &result);
			Assert::IsTrue(found);
			Assert::AreEqual(i, i);
		}
	}

	TEST_METHOD(test_update)
	{
		const int key = 1;
		const std::string value = std::to_string(key);

		concurrent_skiplist<int, std::string> sl;
		sl.try_add(key, std::string{});

		auto updated = sl.try_update(key, value);
		Assert::IsTrue(updated);

		std::string result;
		auto found = sl.try_get_value(key, &result);
		Assert::IsTrue(found);
		Assert::AreEqual(value, result);
	}

	TEST_METHOD(test_update_non_existing)
	{
		const int key = 1;
		const std::string value = std::to_string(key);

		concurrent_skiplist<int, std::string> sl;

		auto updated = sl.try_update(key, value);
		Assert::IsFalse(updated);

		std::string result;
		auto found = sl.try_get_value(key, &result);
		Assert::IsFalse(found);
	}

	TEST_METHOD(test_update_concurrent)
	{
		const std::unordered_set<std::string> possible_values
		{
			"abcdefghijklmnopqrstuvwxyz",
			"derek",
			"eom"
		};

		const int count = 50;
		const int key = 1;

		concurrent_skiplist<int, std::string> sl;
		sl.try_add(key, *possible_values.begin());

		auto update = [&sl, &possible_values, count, key](std::string value)
		{
			std::string result;

			for (int i = 0; i < count; ++i)
			{
				if (!sl.try_update(key, value))
				{
					return false;
				}

				if (!sl.try_get_value(key, &result)
					|| possible_values.find(result) == possible_values.end())
				{
					return false;
				}
			}

			return true;
		};

		std::vector<std::future<bool>> futures;
		futures.reserve(possible_values.size());
		for (const auto& value : possible_values)
		{
			futures.push_back(std::async(std::launch::async, update, value));
		}

		for (auto& f : futures)
		{
			Assert::IsTrue(f.get());
		}
	}

	TEST_METHOD(test_add_or_update)
	{
		const int key = 1;
		const std::string value1 = std::to_string(key);
		const std::string value2 = std::to_string(-key);
		std::string result;

		concurrent_skiplist<int, std::string> sl;

		sl.add_or_update(key, value1);
		auto found = sl.try_get_value(key, &result);
		Assert::IsTrue(found);
		Assert::AreEqual(value1, result);

		sl.add_or_update(key, value2);
		found = sl.try_get_value(key, &result);
		Assert::IsTrue(found);
		Assert::AreEqual(value2, result);
	}

	TEST_METHOD(test_add_or_update_concurrent)
	{
		const std::unordered_set<std::string> possible_values
		{
			"abcdefghijklmnopqrstuvwxyz",
			"derek",
			"eom"
		};

		const int count = 50;
		const int key = 1;

		concurrent_skiplist<int, std::string> sl;

		auto add_or_update = [&sl, &possible_values, count, key](std::string value)
		{
			std::string result;

			for (int i = 0; i < count; ++i)
			{
				sl.add_or_update(key, value);

				if (!sl.try_get_value(key, &result)
					|| possible_values.find(result) == possible_values.end())
				{
					return false;
				}
			}

			return true;
		};

		std::vector<std::future<bool>> futures;
		futures.reserve(possible_values.size());
		for (const auto& value : possible_values)
		{
			futures.push_back(std::async(std::launch::async, add_or_update, value));
		}

		for (auto& f : futures)
		{
			Assert::IsTrue(f.get());
		}
	}

	TEST_METHOD(test_remove)
	{
		const int key = 1;
		const std::string value = std::to_string(key);

		concurrent_skiplist<int, std::string> sl;
		sl.try_add(key, value);

		auto removed = sl.try_remove(key);
		Assert::IsTrue(removed);

		std::string result;
		auto found = sl.try_get_value(key, &result);
		Assert::IsFalse(found);
	}

	TEST_METHOD(test_remove_nonexisting)
	{
		const int key = 1;
		const std::string value = std::to_string(key);

		concurrent_skiplist<int, std::string> sl;

		auto removed = sl.try_remove(key);
		Assert::IsFalse(removed);
	}

	TEST_METHOD(test_remove_multiple)
	{
		const int key = 1;
		const std::string value = std::to_string(key);

		concurrent_skiplist<int, std::string> sl;

		sl.try_add(key, value);

		sl.try_remove(key);
		auto removed = sl.try_remove(key);
		Assert::IsFalse(removed);
	}

	TEST_METHOD(test_remove_concurrent)
	{
		const int threads = 10;
		const int count = 50;
		const int key = 1;

		concurrent_skiplist<int, int> sl;
		auto add_remove = [&sl, count]() -> std::pair<int, int>
		{
			int adds = 0;
			int removes = 0;
			for (int i = 0; i < count; ++i)
			{
				adds += sl.try_add(1, 1) ? 1 : 0;
				removes += sl.try_remove(1) ? 1 : 0;
			}

			return { adds, removes };
		};

		std::vector<std::future<std::pair<int, int>>> futures(threads);
		for (int i = 0; i < threads; ++i)
		{
			futures[i] = std::async(std::launch::async, add_remove);
		}

		int total_adds = 0;
		int total_removes = 0;
		for (auto& f : futures)
		{
			int adds;
			int removes;
			std::tie(adds, removes) = f.get();
			total_adds += adds;
			total_removes += removes;
		}

		Assert::IsTrue(total_adds >= total_removes);
	}

	TEST_METHOD(test_remove_multiple_concurrent)
	{
		const int threads = 10;
		const int count = 500;
		const std::array<int, 3> keys = { 1, 2, 3 };

		concurrent_skiplist<int, int> sl;
		auto add_remove = [&sl, &keys, count](int id) -> std::pair<int, int>
		{
			int key = keys[id % keys.size()];
			int adds = 0;
			int removes = 0;
			for (int i = 0; i < count; ++i)
			{
				adds += sl.try_add(key, 1) ? 1 : 0;
				removes += sl.try_remove(key) ? 1 : 0;
			}

			return { adds, removes };
		};

		std::vector<std::future<std::pair<int, int>>> futures(threads);
		for (int i = 0; i < threads; ++i)
		{
			futures[i] = std::async(std::launch::async, add_remove, i);
		}

		int total_adds = 0;
		int total_removes = 0;
		for (auto& f : futures)
		{
			int adds;
			int removes;
			std::tie(adds, removes) = f.get();
			total_adds += adds;
			total_removes += removes;
		}

		Assert::IsTrue(total_adds >= total_removes);
	}

	TEST_METHOD(test_get_value_null_arg)
	{
		concurrent_skiplist<int, int> sl;

		try {
			sl.try_get_value(1, nullptr);
		}
		catch (const std::invalid_argument&) {
			return;
		}

		Assert::Fail();
	}
};
