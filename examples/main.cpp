#include <parallelcompute/thread_pool.hpp>
#include <parallelcompute/parallel_for.hpp>
#include <parallelcompute/parallel_reduce.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <vector>

#include <iostream>

constexpr size_t resultSize = 10000000;
constexpr size_t poolSize = 4;

int main()
{

    std::vector<int> results1(resultSize, 0);

    auto start = std::chrono::steady_clock::now();

    for(size_t i = 0; i < resultSize; i++){
      results1[i] = i*2;
    }

    auto end = std::chrono::steady_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(
            end - start);
    
    std::cout<<"DURATION "<<duration.count()<<std::endl;
    

    pc::ThreadPool pool(poolSize);

    std::vector<int> results(resultSize, 0);

    start = std::chrono::steady_clock::now();

    pc::parallel_for(
        pool, std::size_t{0}, results.size(),
        [&](std::size_t i) {
            results[i] = static_cast<int>(i * 2);
        }
    );

    end = std::chrono::steady_clock::now();
    duration =
        std::chrono::duration_cast<std::chrono::microseconds>(
            end - start);
    
    std::cout<<"DURATION "<<duration.count()<<std::endl;

    for (std::size_t i = 0; i < results.size(); ++i) {
        assert(results[i] == static_cast<int>(i * 2));
    }

    std::vector<int> data{};

    auto result = pc::parallel_reduce(
        pool,
        data.begin(),
        data.end(),
        10,
        std::plus<>{}
    );

    std::cout << result << '\n'; // 31

}