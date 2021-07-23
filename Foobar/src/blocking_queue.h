#pragma once
#include <condition_variable>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

template<typename value_t>
class blocking_queue
{
	struct node
	{
		std::unique_ptr<node> next;
		std::unique_ptr<value_t> value;
	};
public:
	blocking_queue()
		: _head(std::make_unique<node>())
		, _tail(_head.get())
	{
	}

	~blocking_queue()
	{
		// Default destructor can cause stack overflow.
		while (_head) {
			_head = std::move(_head->next);
		}
	}

	void push(value_t value)
	{
		auto new_value = std::make_unique<value_t>(std::move(value));
		auto new_tail = std::make_unique<node>();
		
		{
			std::lock_guard<std::mutex> lock(_tail_mutex);
			_tail->value = std::move(new_value);
			_tail->next = std::move(new_tail);
			_tail = _tail->next.get();
		}

		_not_empty_cond_var.notify_one();
	}

	bool try_pop(value_t* out_value)
	{
		if (!out_value) {
			throw std::invalid_argument("out_value is null");
		}

		std::lock_guard<std::mutex> lock(_head_mutex);
		
		if (_head.get() == tail()) {
			return false;
		}

		*out_value = std::move(*_head->value.get());
		_head = std::move(_head->next);
		
		return true;
	}

	void wait_pop(value_t* out_value)
	{
		if (!out_value) {
			throw std::invalid_argument("out_value is null");
		}

		std::unique_lock<std::mutex> lock(_head_mutex);
		_not_empty_cond_var.wait(lock, [this] { return _head.get() != tail(); });

		*out_value = std::move(*_head->value.get());
		_head = std::move(_head->next);
	}

	bool empty() const
	{
		std::lock_guard<std::mutex> lock(_head_mutex);
		return _head.get() == tail();
	}

private:
	node* tail() const
	{
		std::lock_guard<std::mutex> lock(_tail_mutex);
		return _tail;
	}

	mutable std::mutex _head_mutex;
	std::unique_ptr<node> _head;
	mutable std::mutex _tail_mutex;
	node* _tail;
	std::condition_variable _not_empty_cond_var;
};