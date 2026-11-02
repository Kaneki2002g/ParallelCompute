#include <algorithm>
#include <parallelcompute/thread_pool.hpp>
#include <stdexcept>
#include <cstddef>
#include <exception>
#include <future>
#include <vector>

namespace pc
{

  template <typename F>
  void parallel_for(
      ThreadPool &pool,
      std::size_t begin,
      std::size_t end,
      F &&func)
  {
    if(begin > end){
      throw std::invalid_argument("No range for loop");
    }
    std::vector<std::future<void>> futures;
    const std::size_t count = end - begin;
    const std::size_t workers = pool.thread_count();

    const std::size_t block_size =
        count / workers + (count % workers != 0);
    auto block_begin = begin;

    std::exception_ptr first_exception = nullptr;

    try
    {
      while (block_begin < end)
      {
        auto block_end = block_begin +
                         std::min(block_size, end - block_begin);
        futures.emplace_back(pool.submit([&func, block_begin, block_end]()
                                         {
        for (auto it = block_begin; it < block_end; ++it)
        {
            func(it);
        } }));
        block_begin = block_end;
      }
    }
    catch (...)
    {
      first_exception = std::current_exception();
    }
    for (auto &future : futures)
    {
      try
      {
        future.get();
      }
      catch (...)
      {
        if (!first_exception)
        {
          first_exception = std::current_exception();
        }
      }
    }
    if (first_exception)
    {
      std::rethrow_exception(first_exception);
    }
  };

}