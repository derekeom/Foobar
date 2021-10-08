#include "concurrent_skiplist.h"
#include "stopwatch.h"
#include "thread_pool.h"

#include <chrono>
#include <iostream>

using namespace std;
using namespace std::chrono_literals;

void func()
{
    {
        thread_pool tp;
        auto ft = tp.submit([] {std::cout << "hello world\n" << std::flush; });
        ft.wait();
    }
    stopwatch sw;
    sw.start();
    std::this_thread::sleep_for(1s);
    std::cout << (sw.elapsed_nanoseconds() / 1'000'000'000.0) << std::endl;

    concurrent_skiplist<int, int> sl;
    sl.try_add(1, 2);
    int val;
    sl.try_get_value(1, &val);
    std::cout << val << std::endl << std::flush;


}

int main()
{
    func();
}
