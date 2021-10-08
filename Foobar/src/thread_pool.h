#pragma once

#include "blocking_queue.h"

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class thread_pool
{
	class task
	{
		class callable_base
		{
		public:
			virtual void invoke() = 0;
			virtual ~callable_base() {}
		};

		template <typename Function>
		class callable : public callable_base
		{
		public:
			callable(Function&& f) : _func(std::move(f)) {}
			void invoke() override { _func(); }

		private:
			Function _func;
		};

	public:
		template<typename Function>
		task(Function&& func)
			: _callable(std::make_unique<callable<Function>>(std::move(func)))
		{
		}

		task() = default;
		task(task&&) = default;
		task& operator=(task&&) = default;

		task(const task&) = delete;
		task& operator=(const task&) = delete;

		void run() { _callable->invoke(); }

	private:
		std::unique_ptr<callable_base> _callable;
	};

	class work_stealing_queue
	{
	public:
		work_stealing_queue() {}
		work_stealing_queue(const work_stealing_queue& other) = delete;
		work_stealing_queue& operator=(const work_stealing_queue& other) = delete;

		void push(task task)
		{
			std::scoped_lock<std::mutex> lock(_mutex);
			_queue.push_front(std::move(task));
		}

		bool empty() const
		{
			std::scoped_lock<std::mutex> lock(_mutex);
			return _queue.empty();
		}

		bool try_pop(task* result)
		{
			std::scoped_lock<std::mutex> lock(_mutex);

			if (_queue.empty()) {
				return false;
			}

			*result = std::move(_queue.front());
			_queue.pop_front();
			return true;
		}

		bool try_steal(task* result)
		{
			std::scoped_lock<std::mutex> lock(_mutex);

			if (_queue.empty()) {
				return false;
			}

			*result = std::move(_queue.back());
			_queue.pop_back();
			return true;
		}

	private:
		std::deque<task> _queue;
		mutable std::mutex _mutex;
	};

public:
	thread_pool(size_t num_threads = std::thread::hardware_concurrency())
		: _done(false)
		, _queues(num_threads)
	{
		try
		{
			for (size_t i = 0; i < _queues.size(); ++i) {
				_threads.emplace_back(&thread_pool::worker_thread, this, i);
			}
		}
		catch (...)
		{
			_done = true;
			throw;
		}
	}

	~thread_pool()
	{
		_done = true;
	}

	template <typename Function>
	std::future<std::invoke_result_t<Function>> submit(Function func)
	{
		using result_t = std::invoke_result_t<Function>;
		std::packaged_task<result_t()> task(func);
		std::future<result_t> result(task.get_future());

		if (s_local_work_queue) {
			s_local_work_queue->push(std::move(task));
		}
		else {
			_pool_work_queue.push(std::move(task));
		}

		return result;
	}

	void run_pending_task()
	{
		task task;
		if (try_pop_from_local_queue(&task)
		 || try_pop_from_pool(&task)
		 || try_steal_from_other_queue(&task))
		{
			task.run();
			return;
		}

		std::this_thread::yield();
	}

private:
	void worker_thread(size_t thread_index)
	{
		s_local_thread_index = thread_index;
		s_local_work_queue = &_queues[s_local_thread_index];

		while (!_done) {
			run_pending_task();
		}
	}

	bool try_pop_from_local_queue(task* out_task)
	{
		return s_local_work_queue && s_local_work_queue->try_pop(out_task);
	}

	bool try_pop_from_pool(task* out_task)
	{
		return _pool_work_queue.try_pop(out_task);
	}

	bool try_steal_from_other_queue(task* out_task)
	{
		for (size_t i = 1; i <= _queues.size(); ++i)
		{
			const auto index = (s_local_thread_index + i) % _queues.size();
			if (_queues[index].try_steal(out_task)) {
				return true;
			}
		}

		return false;
	}

	std::atomic_bool _done;
	blocking_queue<task> _pool_work_queue;
	std::vector<work_stealing_queue> _queues;
	std::vector<std::jthread> _threads;

	static thread_local inline work_stealing_queue* s_local_work_queue;
	static thread_local inline size_t s_local_thread_index;
};
