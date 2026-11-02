#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <stdexcept>
#include <future>

namespace pc
{

    class ThreadPool
    {
    public:
        explicit ThreadPool(std::size_t thread_count);
        ~ThreadPool();

        ThreadPool(const ThreadPool &) = delete;
        ThreadPool &operator=(const ThreadPool &) = delete;

        void enqueue(std::function<void()> task);

        std::size_t thread_count() const noexcept
        {
            return workers_.size();
        }

        template <typename F>
        auto submit(F &&func) -> std::future<std::invoke_result_t<F &>>
        {
            using Result = std::invoke_result_t<F &>;

            auto task = std::make_shared<std::packaged_task<Result()>>(
                std::forward<F>(func));

            auto future = task->get_future();
            enqueue([task]()
                    { (*task)(); });

            return future;
        }

    private:
        void worker_loop();

        std::mutex mutex_;
        std::condition_variable condition_;
        std::queue<std::function<void()>> tasks_;
        bool stopping_ = false;

        std::vector<std::jthread> workers_;
    };

} // namespace pc