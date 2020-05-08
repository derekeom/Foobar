#include "thread_pool.h"

thread_local thread_pool::work_stealing_queue* thread_pool::s_local_work_queue;
thread_local size_t thread_pool::s_local_thread_index;
