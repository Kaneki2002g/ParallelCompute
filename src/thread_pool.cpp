#include <parallelcompute/thread_pool.hpp>

namespace pc
{
  ThreadPool::ThreadPool(std::size_t thread_count)
  {
    if (thread_count == 0)
    {
      throw std::invalid_argument("Thread count must be positive");
    }

    workers_.reserve(thread_count);

    try
    {
      for (std::size_t i = 0; i < thread_count; ++i)
      {
        workers_.emplace_back([this]
                              { worker_loop(); });
      }
    }
    catch (...)
    {
      {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
      }

      condition_.notify_all();

      for(auto& worker : workers_){
        worker.join();
      }
      
      throw;
    }
  }

  void ThreadPool::worker_loop()
  {
    while (true)
    {
      std::function<void()> task;

      {
        std::unique_lock<std::mutex> lock(mutex_);

        condition_.wait(lock, [this]
                        { return stopping_ || !tasks_.empty(); });
        // If stopping, and no tasks available then end worker
        if (stopping_ && tasks_.empty())
        {
          return;
        }

        // Take workload
        task = tasks_.front();
        tasks_.pop();
      }
      // Do the task
      task();
    }
  }

  void ThreadPool::enqueue(std::function<void()> task)
  {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (stopping_)
      {
        throw std::runtime_error("Enqueue not permitted after stopping");
      }
      tasks_.emplace(std::move(task));
    }
    condition_.notify_one();
  }

  ThreadPool::~ThreadPool()
  {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      stopping_ = true;
    }
    condition_.notify_all();
    for (auto &worker : workers_)
      worker.join();
  }
}