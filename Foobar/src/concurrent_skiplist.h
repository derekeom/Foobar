#pragma once

#include <array>
#include <memory>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <stdexcept>
#include <utility>

template <typename key_t, typename value_t>
class concurrent_skiplist
{
	using transferable_lock = std::unique_lock<std::mutex>;

	class node
	{
	public:
		node(int levels) : node(key_t{}, value_t{}, levels)
		{
		}

		node(key_t key, value_t&& value, int levels)
			: _key(std::move(key))
			, _forward(std::make_unique<node* []>(levels))
			, _forward_mutexes(std::make_unique<std::mutex[]>(levels))
			, _top_level(levels - 1)
			, _value(std::move(value))
		{
		}

		const key_t& key() const
		{
			return _key;
		}

		value_t value() const
		{
			std::shared_lock<std::shared_mutex> lock(_value_mutex);
			return _value;
		}

		void set_value(value_t&& value)
		{
			std::unique_lock<std::shared_mutex> lock(_value_mutex);
			_value = std::move(value);
		}

		node* forward(int level) const
		{
			return _forward[level];
		}

		void set_forward(node* next, int level)
		{
			_forward[level] = next;
		}

		transferable_lock lock(int level)
		{
			return transferable_lock(_forward_mutexes[level]);
		}

		transferable_lock lock_for_modify()
		{
			return transferable_lock(_modify_mutex);
		}

		int top_level() const
		{
			return _top_level;
		}

	private:
		const key_t _key;
		std::unique_ptr<node* []> _forward;
		std::unique_ptr<std::mutex[]> _forward_mutexes;
		const int _top_level;
		value_t _value;
		mutable std::shared_mutex _value_mutex;
		std::mutex _modify_mutex;
	};

	class top_level_generator
	{
	public:
		top_level_generator(int level_cap)
			: _distribution(0, (1ULL << level_cap) - 1)
		{
		}

		int get()
		{
			unsigned long long bits;
			{
				std::lock_guard<std::mutex> lock(_random_engine_mutex);
				bits = _distribution(_generator);
			}
			
			int top_level = 0;
			while (bits & 1)
			{
				++top_level;
				bits >>= 1;
			}
			return top_level;
		}

	private:
		std::mutex _random_engine_mutex;
		std::default_random_engine _generator;
		const std::uniform_int_distribution<unsigned long long> _distribution;
	};

public:
	concurrent_skiplist()
		: _head(new node(_max_levels))
		, top_level_generator(_level_cap)
	{
	}

	~concurrent_skiplist()
	{
		auto current = _head;
		while (current)
		{
			auto next = current->forward(_bottom_level);
			delete current;
			current = next;
		}
	}

	bool try_get_value(const key_t& key, value_t* out_value) const
	{
		if (out_value == nullptr) {
			throw std::invalid_argument("out_value is null");
		}

		node* current = _head;
		node* next = nullptr;

		for (int i = _top_level_hint; i >= 0; --i)
		{
			next = current->forward(i);
			while (compare_less(next, key))
			{
				current = next;
				next = current->forward(i);
			}
		}

		if (compare_equal(next, key))
		{
			*out_value = next->value();
			return true;
		}

		return false;
	}

	void add_or_update(const key_t& key, value_t value)
	{
		add_or_update(key, value, /*add*/true, /*update*/true);
	}

	bool try_add(const key_t& key, value_t value)
	{
		return add_or_update(key, value, /*add*/true, /*update*/false);
	}

	bool try_update(const key_t& key, value_t value)
	{
		return add_or_update(key, value, /*add*/false, /*update*/true);
	}

	bool try_remove(const key_t& key)
	{
		std::array<node*, _max_levels> update;
		auto top_level_hint = _top_level_hint;
		auto current = search(key, top_level_hint, &update);
		transferable_lock modify_lock;

		while (true)
		{
			current = current->forward(_bottom_level);
			if (compare_greater(current, key)) {
				return false;
			}

			modify_lock = current->lock_for_modify();
			auto next = current->forward(_bottom_level);
			auto is_garbage = compare_greater(current, next);
			if (compare_equal(current, key) && !is_garbage) {
				break;
			}

			modify_lock.unlock();
		}

		for (int i = top_level_hint + 1; i <= current->top_level(); ++i)
		{
			update[i] = _head;
		}

		for (int i = current->top_level(); i >= _bottom_level; --i)
		{
			auto previous = update[i];
			auto previous_lock = find_and_lock(&previous, key, i);
			auto current_lock = current->lock(i);
			previous->set_forward(current->forward(i), i);
			current->set_forward(previous, i);
		}

		// todo: put current in garbage queue

		modify_lock.unlock();
		decrease_top_level_hint();

		return true;
	}

private:
	bool add_or_update(
		const key_t& search_key,
		value_t& value,
		bool add_if_no_exist,
		bool update_if_exist)
	{
		std::array<node*, _max_levels> update;
		auto top_level_hint = _top_level_hint;
		auto previous = search(search_key, top_level_hint, &update);
		auto previous_lock = find_and_lock(&previous, search_key, _bottom_level);
		auto current = previous->forward(_bottom_level);

		if (compare_equal(current, search_key))
		{
			if (update_if_exist)
			{
				current->set_value(std::move(value));
				return true;
			}
			return false;
		}

		if (!add_if_no_exist) {
			return false;
		}

		auto top_level = top_level_generator.get();
		current = new node(search_key, std::move(value), top_level + 1);
		auto modify_lock = current->lock_for_modify();

		for (int i = top_level_hint + 1; i <= top_level; ++i)
		{
			update[i] = _head;
		}

		for (int i = 0; i <= top_level; ++i)
		{
			if (i != _bottom_level)
			{
				previous = update[i];
				previous_lock = find_and_lock(&previous, search_key, i);
			}
			current->set_forward(previous->forward(i), i);
			previous->set_forward(current, i);
			previous_lock.unlock();
		}

		modify_lock.unlock();
		increase_top_level_hint();

		return true;
	}

	template<int array_size>
	node* search(
		const key_t& search_key,
		int top_level,
		std::array<node*, array_size>* update)
	{
		auto previous = _head;
		for (int i = top_level; i >= _bottom_level; --i)
		{
			auto current = previous->forward(i);
			while (compare_less(current, search_key))
			{
				previous = current;
				current = previous->forward(i);
			}
			update->at(i) = previous;
		}
		return previous;
	}

	transferable_lock find_and_lock(
		node** current_ptr,
		const key_t& search_key,
		int level)
	{
		auto current = *current_ptr;
		auto next = current->forward(level);

		while (compare_less(next, search_key))
		{
			current = next;
			next = current->forward(level);
		}

		auto current_lock = current->lock(level);
		next = current->forward(level);

		while (compare_less(next, search_key))
		{
			current_lock.unlock();
			current = next;
			current_lock = current->lock(level);
			next = current->forward(level);
		}

		*current_ptr = current;
		return current_lock;
	}

	void increase_top_level_hint()
	{
		if (_top_level_hint < _level_cap
			&& _head->forward(_top_level_hint + 1) != nullptr
			&& _top_level_hint_mutex.try_lock())
		{
			while (_top_level_hint < _level_cap
				&& _head->forward(_top_level_hint + 1) != nullptr)
			{
				_top_level_hint = _top_level_hint + 1;
			}
			_top_level_hint_mutex.unlock();
		}
	}

	void decrease_top_level_hint()
	{
		if (_top_level_hint > _bottom_level
			&& !_head->forward(_top_level_hint)
			&& _top_level_hint_mutex.try_lock())
		{
			while (_top_level_hint > _bottom_level
				&& !_head->forward(_top_level_hint))
			{
				--_top_level_hint;
			}
			_top_level_hint_mutex.unlock();
		}
	}

	int compare(const node* a, const key_t& search_key) const
	{
		if (a == _head) {
			return -1;
		}

		if (a == nullptr) {
			return 1;
		}

		if (a->key() == search_key) {
			return 0;
		}

		return a->key() < search_key ? -1 : 1;
	}

	int compare(const node* a, const node* b) const
	{
		if (a == b) {
			return 0;
		}

		if (a == _head || b == nullptr) {
			return -1;
		}

		if (a == nullptr || b == _head) {
			return 1;
		}

		return a->key() < b->key() ? -1 : 1;
	}

	bool compare_less(const node* a, const key_t& search_key) const
	{
		return compare(a, search_key) == -1;
	}

	bool compare_less(const node* a, const node* b) const
	{
		return compare(a, b) == -1;
	}

	bool compare_equal(const node* a, const key_t& search_key) const
	{
		return compare(a, search_key) == 0;
	}

	bool compare_equal(const node* a, const node* b) const
	{
		compare(a, b) == 0;
	}

	bool compare_greater(const node* a, const key_t& search_key) const
	{
		return compare(a, search_key) == 1;
	}

	bool compare_greater(const node* a, const node* b) const
	{
		return compare(a, b) == 1;
	}

	node* const _head;
	int _top_level_hint = 0;
	std::mutex _top_level_hint_mutex;
	top_level_generator top_level_generator;

	static constexpr int _bottom_level = 0;
	static constexpr int _max_levels = 32;
	static constexpr int _level_cap = _max_levels - 1;
};