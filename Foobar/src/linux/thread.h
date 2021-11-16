#include <pthread.h>

#include <cassert>
#include <iostream>
#include <memory>
#include <thread>

class Thread
{
    class CallableBase
    {
    public:
        virtual void invoke() = 0;
        virtual ~CallableBase() {}
    };

    template <typename Function>
    class Callable : public CallableBase
    {
    public:
        Callable(Function&& func) : func_(std::move(func)) {}
        void invoke() override { func_(); }
    private:
        Function func_;
    };

public:
    template <typename Function>
    Thread(Function&& func)
        : callable_(std::make_unique<Callable<Function>>(std::move(func)))
        , joinable_(true)
    {
        int r = pthread_create(&thread_handle_, nullptr, &Thread::execute, callable_.get());
        assert(r == 0);
    }

    ~Thread()
    {
        if (joinable_)
        {
            join();
        }
    }

    Thread(Thread&& other)
    {
        thread_handle_ = std::exchange(other.thread_handle_, -1);
        callable_ = std::exchange(other.callable_, nullptr);
        joinable_ = std::exchange(other.joinable_, false);
    }

    Thread& operator=(Thread&& other)
    {
        if (this != &other)
        {
            if (joinable())
            {
                join();
            }

            thread_handle_ = std::exchange(other.thread_handle_, -1);
            callable_ = std::exchange(other.callable_, nullptr);
            joinable_ = std::exchange(other.joinable_, false);
        }

        return *this;
    }

    Thread(const Thread& other) = delete;
    Thread& operator=(const Thread& other) = delete;

    void join()
    {
        assert(joinable_);
        joinable_ = false;

        int r = pthread_join(thread_handle_, nullptr);
        assert(r == 0);
    }

    bool joinable() const { return joinable_; }

private:
    static void* execute(void* ptr)
    {
        static_cast<CallableBase*>(ptr)->invoke();
    }

    pthread_t thread_handle_;
    std::unique_ptr<CallableBase> callable_;
    bool joinable_;
};
