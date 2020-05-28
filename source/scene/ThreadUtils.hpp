#pragma once
#include <map>
#include <deque>
#include <cstdint>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <future>

namespace Scene
{
namespace utils
{

struct BoolScopeGuard
{
    bool& val;

    BoolScopeGuard(bool& v)
        : val(v)
    {
    }

    ~BoolScopeGuard()
    {
        val = !val;
    }
};

struct DefferedExecutor
{
    static constexpr uint64_t immediate = 0u;

    uint64_t Add(uint64_t delay, std::function<void()>&& task)
    {
        std::unique_lock lock(tasks_mutex);
        while (executig)
            condition_variable.wait(lock);

        auto id = unique_id++;
        tasks.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(prev_time + delay, std::move(task)));
        return id;
    }

    void Remove(uint64_t id)
    {
        std::unique_lock lock(tasks_mutex);
        while (executig)
            condition_variable.wait(lock);

        auto task_it = tasks.find(id);
        if (task_it == tasks.end())
            return;

        task_it->second.task.swap(std::function<void()>{});
        tasks.erase(task_it);
    }

    void Execute(uint64_t time)
    {
        {
            BoolScopeGuard guard(executig);
            executig = true;

            std::unique_lock lock(tasks_mutex);
            std::erase_if(tasks, [time](const auto& task) {
                return task.second.execution_time <= time;
            });
            prev_time = time;
        }
        condition_variable.notify_all();
    }

    ~DefferedExecutor()
    {
        Execute(std::numeric_limits<uint64_t>::max());
    }

private:
    struct TaskWrapper
    {
        uint64_t execution_time = 0u;
        std::function<void()> task;

        TaskWrapper(uint64_t et, std::function<void()>&& t)
            : execution_time(et)
            , task(std::move(t))
        {
        }

        TaskWrapper(TaskWrapper&& r)
            : execution_time(r.execution_time)
        {
            task.swap(r.task);
        }

        TaskWrapper& operator=(TaskWrapper&& r)
        {
            execution_time = r.execution_time;
            task.swap(r.task);
            return *this;
        }

        ~TaskWrapper()
        {
            if (task)
                task();
        }

        TaskWrapper(const TaskWrapper&) = delete;
        TaskWrapper& operator=(const TaskWrapper&) = delete;
    };

    uint64_t prev_time = 0u;
    uint64_t unique_id = 0u;
    std::map<uint64_t, TaskWrapper> tasks;

    std::mutex               tasks_mutex;
    std::condition_variable  condition_variable;
    bool executig = false;
};

template <typename T>
struct PriorityExecutor
{
    PriorityExecutor(uint32_t thread_count = 0)
    {
        uint32_t n = thread_count;
        if (n <= 0)
            n = std::max(std::thread::hardware_concurrency(), 1u);
        for (uint32_t i = 0; i < n; ++i)
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
        {
            BoolScopeGuard guard(adding);
            adding = true;
            std::unique_lock<std::mutex> lock(tasks_mutex);
            tasks[key].swap(task);
        }
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
