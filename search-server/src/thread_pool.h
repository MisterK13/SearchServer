#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <atomic>

class ThreadPool
{
public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    template <typename F, typename... Args>
    auto Enqueue(F &&f, Args &&...args)
        -> std::future<typename std::result_of<F(Args...)>::type>;

    size_t GetThreadCount() const { return workers_.size(); }
    size_t GetPendingTasks() const;

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
};

template <typename F, typename... Args>
auto ThreadPool::Enqueue(F &&f, Args &&...args)
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> result = task->get_future();

    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (stop_)
        {
            throw std::runtime_error("ThreadPool::Enqueue on stopped ThreadPool");
        }
        tasks_.emplace([task]()
                       { (*task)(); });
    }

    condition_.notify_one();
    return result;
}
