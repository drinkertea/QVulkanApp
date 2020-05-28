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

    std::weak_ptr<bool> Add(uint64_t delay, std::function<void()>&& task)
    {
        std::unique_lock lock(tasks_mutex);
        while (executig)
            condition_variable.wait(lock);

        tasks.emplace_back(prev_time + delay, std::move(task));
        return tasks.back().alive_marker;
    }

    void Execute(uint64_t time)
    {
        {
            BoolScopeGuard guard(executig);
            executig = true;

            std::unique_lock lock(tasks_mutex);
            tasks.erase(std::remove_if(tasks.begin(), tasks.end(), [time](const auto& task) {
                return task.execution_time <= time;
            }), tasks.end());
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

        std::shared_ptr<bool> alive_marker = std::make_shared<bool>(true);

        TaskWrapper(uint64_t et, std::function<void()>&& t)
            : execution_time(et)
            , task(std::move(t))
        {
        }

        TaskWrapper(TaskWrapper&& r)
            : execution_time(r.execution_time)
        {
            task.swap(r.task);
            alive_marker.swap(r.alive_marker);
        }

        TaskWrapper& operator=(TaskWrapper&& r)
        {
            execution_time = r.execution_time;
            task.swap(r.task);
            alive_marker.swap(r.alive_marker);
            return *this;
        }

        ~TaskWrapper()
        {
            if (!*alive_marker)
                return;
            task();
        }

        TaskWrapper(const TaskWrapper&) = delete;
        TaskWrapper& operator=(const TaskWrapper&) = delete;
    };

    uint64_t prev_time = 0u;
    std::deque<TaskWrapper> tasks;

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
