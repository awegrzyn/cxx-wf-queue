#include "WFQueue.cxx"
#include <thread>
#include <iostream>
#include <benchmark/benchmark.h>

static void pinThread(int cpu) {
    if (cpu < 0) {
        return;
    }
    ::cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    if (::pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == -1) {
        std::perror("pthread_setaffinity_rp");
        std::exit(EXIT_FAILURE);
    }
}

constexpr auto cpu1 = 1;
constexpr auto cpu2 = 2;

static void BENCHMARK_queue(benchmark::State &state)
{
    constexpr auto iters = 100'000'000l;
    constexpr auto fifoSize = 65536;
    WFQueue<std::int_fast64_t> fifo(fifoSize);

    auto t = std::jthread([&] {
      pinThread(cpu1);
      // pull warmup
      for (std::int_fast64_t i = 0; i < fifoSize; ++i) {
        std::int_fast64_t val = 0;
        while (auto again = not fifo.pull(val)) benchmark::DoNotOptimize(again);
      }

      for (std::int_fast64_t i = 0; i < iters; ++i) {
        std::int_fast64_t val = 0;
        while (auto again = not fifo.pull(val)) benchmark::DoNotOptimize(again);
        if (val != i) {
          throw std::runtime_error("Value does not match");
        }
      }
    });


    pinThread(cpu2);
    // push warmup
    for (std::int_fast64_t i = 0; i < fifoSize; ++i) {
      while (auto again = not fifo.push(i)) benchmark::DoNotOptimize(again);
    }
    while (auto again = not fifo.empty()) benchmark::DoNotOptimize(again);

    std::int_fast64_t i = 0;
    for (; i < iters; ++i) {
      while (auto again = not fifo.push(i)) benchmark::DoNotOptimize(again);
    }
    while (auto again = not fifo.empty()) benchmark::DoNotOptimize(again);

    state.counters["ops/sec"] = benchmark::Counter(double(i), benchmark::Counter::kIsRate);
    state.PauseTiming();
    fifo.push(-1);
}

BENCHMARK(BENCHMARK_queue);
BENCHMARK_MAIN();
