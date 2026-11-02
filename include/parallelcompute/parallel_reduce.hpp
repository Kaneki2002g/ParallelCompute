#include <parallelcompute/thread_pool.hpp>

namespace pc
{
  template <typename Iterator, typename T, typename BinaryOp>
  T parallel_reduce(
      ThreadPool &pool,
      Iterator begin,
      Iterator end,
      T init,
      BinaryOp op)
  {
    auto count = std::distance(begin, end);

    if (count == 0)
    {
      return init;
    }
    std::vector<std::future<T>> futures;
    const std::size_t workers = pool.thread_count();

    const std::size_t block_size =
        count / workers + (count % workers != 0);
    auto block_begin = begin;
    std::exception_ptr first_exception = nullptr;
    try
    {
      auto block_end = block_begin;

      while (block_begin != end)
      {
        const auto remaining = std::distance(block_begin, end);
        const auto current_size = std::min(
            block_size,
            static_cast<std::size_t>(remaining));

        std::advance(block_end, current_size);

        futures.emplace_back(pool.submit([block_begin, block_end, &op]()
                                         {
        T outcome = *block_begin;
        for (auto it = block_begin + 1; it < block_end; ++it)
        {
            outcome = op(outcome, *it);
        }
        
        return outcome; }));
        block_begin = block_end;
      }
    }
    catch (...)
    {
      first_exception = std::current_exception();
    }
    T ret = init;
    for (auto &future : futures)
    {
      try
      {
        ret = op(ret, future.get());
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
    return ret;
  }

} // namespace pc