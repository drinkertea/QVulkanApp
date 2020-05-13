#pragma once
#include <map>
#include <cstdint>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <future>

namespace Scene
{
namespace utils
{

template <typename T>
struct PriorityExecutor
{
    PriorityExecutor(int thread_count = 0)
    {
        int n = thread_count;
        if (n <= 0)
            n = std::max(std::thread::hardware_concurrency() / 2, 1);
        for (int i = 0; i < n; ++i)
            execution_threads.emplace_back(&PriorityExecutor::ExecutionThread, this);
    }

    ~PriorityExecutor()
    {
        terminated = true;
        condition_variable.notify_all();
        for (auto& execution_thread : execution_threads)
            execution_thread.join();
    }

    std::future<T> Add(uint32_t key, std::function<T()>&& func)
    {
        Task task(std::move(func));
        std::future<T> res = task.get_future();

        adding = true;
        {
            std::unique_lock<std::mutex> lock(tasks_mutex);
            tasks[key].swap(task);
        }
        adding = false;
        condition_variable.notify_one();

        return res;
    }

private:
    using Task = std::packaged_task<T()>;

    void ExecutionThread()
    {
        while (!terminated)
        {
            Task task;
            {
                std::unique_lock<std::mutex> lock(tasks_mutex);
                while (adding || tasks.empty())
                {
                    condition_variable.wait(lock);
                    if (terminated)
                        return;
                }

                task.swap(tasks.begin()->second);
                tasks.erase(tasks.begin());
            }
            task();
        }
    }

    std::map<uint32_t, Task> tasks;
    std::mutex               tasks_mutex;
    std::condition_variable  condition_variable;
    std::vector<std::thread> execution_threads;

    bool terminated = false;
    bool adding = false;
};

}
}
