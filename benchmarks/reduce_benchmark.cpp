#include <chrono>
#include <iostream>
#include <numeric>
#include <parallelcompute/parallel_reduce.hpp>
#include <parallelcompute/thread_pool.hpp>
#include <random>
#include <ratio>
#include <tuple>
#include <vector>

constexpr int repetitions = 10;

template <typename Duration> double median(std::vector<Duration> times) {
  std::sort(times.begin(), times.end());

  const std::size_t n = times.size();

  if (n % 2 == 1) {
    return static_cast<double>(times[n / 2].count());
  }

  return (static_cast<double>(times[n / 2 - 1].count()) +
          static_cast<double>(times[n / 2].count())) /
         2.0;
}

template <typename Generator>
void getrands(std::vector<double> &x, Generator &gen, unsigned num) {
  std::generate_n(std::back_inserter(x), num, std::ref(gen));
}

std::tuple<double, std::chrono::microseconds>
checkAccumulate(std::vector<double> &vs) {
  auto start = std::chrono::steady_clock::now();
  auto result = std::accumulate(vs.begin(), vs.end(), 0.0, std::plus<>());
  auto end = std::chrono::steady_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  return std::tuple{result, duration};
}

std::tuple<double, std::chrono::microseconds>
checkReduce(pc::ThreadPool &pool, std::vector<double> &vs) {
  auto start = std::chrono::steady_clock::now();
  auto result =
      pc::parallel_reduce(pool, vs.begin(), vs.end(), 0.0, std::plus<>{});
  auto end = std::chrono::steady_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  return std::tuple{result, duration};
}

std::tuple<double, std::chrono::microseconds>
checkOneTask(pc::ThreadPool &pool, std::vector<double> &vs) {
  auto start = std::chrono::steady_clock::now();
  auto future = pool.submit([&vs]() {
    std::plus<> op;

    double result = vs[0];

    for (std::size_t i = 1; i < vs.size(); ++i) {
      result = op(result, vs[i]);
    }

    return result;
  });

  double result = future.get();
  auto end = std::chrono::steady_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  return std::tuple{result, duration};
}

std::tuple<double, std::chrono::microseconds>
checkFor(const std::vector<double> &vs) {
  std::plus<> op;

  auto start = std::chrono::steady_clock::now();

  double result = 0.0;

  for (auto it = vs.begin(); it != vs.end(); ++it) {
    result = op(result, *it);
  }

  auto end = std::chrono::steady_clock::now();

  return {result,
          std::chrono::duration_cast<std::chrono::microseconds>(end - start)};
}

std::tuple<double, std::chrono::microseconds>
checkFor4Acc(std::vector<double> &vs) {
  double sum0 = 0.0;
  double sum1 = 0.0;
  double sum2 = 0.0;
  double sum3 = 0.0;

  auto start = std::chrono::steady_clock::now();

  std::size_t i = 0;

  for (; i < vs.size() - (vs.size() % 4); i += 4) {
    sum0 += vs[i];
    sum1 += vs[i + 1];
    sum2 += vs[i + 2];
    sum3 += vs[i + 3];
  }

  double result = (sum0 + sum1) + (sum2 + sum3);

  // Obsługa pozostałych elementów
  for (; i < vs.size(); ++i) {
    result += vs[i];
  }

  auto end = std::chrono::steady_clock::now();

  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  return std::tuple{result, duration};
}

void benchmark_reduce(int element_count, int worker_count) {
  pc::ThreadPool pool(worker_count);
  std::uniform_real_distribution<double> unif(0.0, 10.0);
  std::mt19937 re(12345);
  auto generator = bind(unif, std::ref(re));

  std::vector<double> vs;
  // warm up
  getrands(vs, generator, element_count);

  for (int i = 0; i < 3; ++i) {
    auto a = checkAccumulate(vs);
    auto b = checkReduce(pool, vs);
    auto c = checkFor(vs);
    auto d = checkOneTask(pool, vs);
    auto e = checkFor4Acc(vs);
  }
  vs.clear();
  // end warmup
  std::vector<std::chrono::microseconds> times_acc;
  std::vector<std::chrono::microseconds> times_red;
  std::vector<std::chrono::microseconds> times_for;
  std::vector<std::chrono::microseconds> times_for_one_worker;
  std::vector<std::chrono::microseconds> times_4for;

  for (int i = 0; i < repetitions; i++) {
    getrands(vs, generator, element_count);
    auto outcomeAccumulate = checkAccumulate(vs);
    double result_accumulate = std::get<0>(outcomeAccumulate);
    auto duration_accumulate = std::get<1>(outcomeAccumulate);

    auto outcomeReduce = checkReduce(pool, vs);
    double result = std::get<0>(outcomeReduce);
    auto duration_reduce = std::get<1>(outcomeReduce);

    auto outcomeFor = checkFor(vs);
    double result_for = std::get<0>(outcomeFor);
    auto duration_for = std::get<1>(outcomeFor);

    auto f_o_w = checkOneTask(pool, vs);

    auto outcomeF4 = checkFor4Acc(vs);
    auto duration_for4 = std::get<1>(outcomeF4);

    const double relative_error = std::abs(result_accumulate - result) /
                                  std::max(1.0, std::abs(result_accumulate));
    const double for_error = std::abs(result_accumulate - result_for) /
                             std::max(1.0, std::abs(result_accumulate));
    const double oneTask_error =
        std::abs(result_accumulate - std::get<0>(f_o_w)) /
        std::max(1.0, std::abs(result_accumulate));
    const double for4acc_error =
        std::abs(result_accumulate - std::get<0>(outcomeF4)) /
        std::max(1.0, std::abs(result_accumulate));

    if (relative_error > 1e-10 || for_error > 1e-10 || oneTask_error > 1e-10 ||
        for4acc_error > 1e-10) {
      throw std::runtime_error("Results differ!");
    }
    times_for_one_worker.emplace_back(std::get<1>(f_o_w));
    times_acc.emplace_back(duration_accumulate);
    times_red.emplace_back(duration_reduce);
    times_for.emplace_back(duration_for);
    times_4for.emplace_back(duration_for4);
    vs.clear();
  }

  std::cout << "Median: ACC = " << median(times_acc)
            << " | RED = " << median(times_red)
            << " | FOR = " << median(times_for)
            << " | FOR IN ONE = " << median(times_for_one_worker)
            << " | FOR 4ACC = " << median(times_4for)
            << " Speedup = " << median(times_acc) / median(times_red)
            << std::endl;
}

int main() {
  std::vector<int> workerCounts = {4};
  std::vector<int> elementCounts = {1000000, 10000000};
  for (const auto &elements : elementCounts) {
    for (const auto &workers : workerCounts) {
      std::cout << "Workers: " << workers << " Elements: " << elements
                << std::endl;
      benchmark_reduce(elements, workers);
    }
  }
}