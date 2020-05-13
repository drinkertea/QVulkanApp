#include "ChunkUtils.h"

#include <algorithm>

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
            n = std::thread::hardware_concurrency();
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
    bool adding     = false;
};
    
uint32_t GetRank(const vec2i& mid, const vec2i& pos)
{
    if (mid == pos)
        return 0;

    int32_t x = pos.x - mid.x;
    int32_t y = pos.y - mid.y;
    int32_t d = std::max(std::abs(x), std::abs(y));
    int32_t o = 0;

    if (x == d)
        o = d - y;
    else if (y == -d)
        o = d * 3 - x;
    else if (x == -d)
        o = d * 5 + y;
    else
        o = d * 7 + x;
    return static_cast<uint32_t>((d * 2 - 1)* (d * 2 - 1) + o);
}

void IterateFromMid(int distance, const utils::vec2i& mid, const std::function<void(int, const utils::vec2i&)>& callback)
{
    int ind = 0;
    callback(ind++, utils::vec2i(mid.x, mid.y));
    for (int dist = 1; dist < distance + 1; ++dist)
    {
        for (int i = dist; i > -dist; --i)
            callback(ind++, utils::vec2i(mid.x + dist, mid.y + i));
        for (int i = dist; i > -dist; --i)
            callback(ind++, utils::vec2i(mid.x + i, mid.y - dist));
        for (int i = -dist; i < dist; ++i)
            callback(ind++, utils::vec2i(mid.x - dist, mid.y + i));
        for (int i = -dist; i < dist; ++i)
            callback(ind++, utils::vec2i(mid.x + i, mid.y + dist));
    }
}

}

}
