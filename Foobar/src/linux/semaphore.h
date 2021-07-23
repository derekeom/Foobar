#include <assert.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>

#include <string>

#include "fd_flags.h"

namespace ipc
{
class Semaphore
{
public:
    Semaphore(const std::string& name, int initial_count)
        : name_(name), unlink_on_close_(true)
        , sem_(sem_open(name.c_str(), OpenFlags::CreateOnly, Permissions::AllReadWrite, initial_count))
    {
        if (!sem_) {
            handle_error("sem_open()");
        }
    }

    Semaphore(const std::string& name)
        : name_(name), unlink_on_close_(false)
        , sem_(sem_open(name_.c_str(), OpenFlags::OpenOnly))
    {
        if (!sem_) {
            handle_error("sem_open()");
        }
    }

    Semaphore(const Semaphore&) = delete;

    ~Semaphore()
    {
        sem_close(sem_);
        if (unlink_on_close_)
        {
            if (sem_unlink(name_.c_str()) == -1) {
                handle_error("sem_unlink()");
            }
        }
    }

    Semaphore& operator=(const Semaphore&) = delete;

    void release()
    {
        if (sem_post(sem_) == -1) {
            handle_error("sem_post()");
        }
    }

    void acquire()
    {
        if (sem_wait(sem_) == -1) {
            handle_error("sem_wait()");
        }
    }

    bool try_acquire()
    {
        if (sem_trywait(sem_) == -1) {
            if (errno == EAGAIN) {
                return false;
            }
            else {
                handle_error("sem_trywait()");
            }
        }
        return true;
    }

private:
    void handle_error(const std::string& msg)
    {
        fprintf(stderr, "%s: %s: %s\n", name_.c_str(), msg.c_str(), strerror(errno));
        assert(false);
    }

    std::string name_;
    bool unlink_on_close_;
    sem_t* sem_;
};
}