#include "WFQueue.cxx"
#ifdef WFQUEUE_WITH_BOOST
#include <boost/lockfree/spsc_queue.hpp>
#endif
#include <thread>
#include <iostream>
#include <benchmark/benchmark.h>

constexpr auto cpu1 = 1;
constexpr auto cpu2 = 2;
constexpr auto iters = 400'000'000l;
constexpr auto queueSize = 65536;

static void pinThread(int cpu) {
    ::cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    if (::pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == -1) {
        std::perror("Unable to pin thread to selected CPU core");
        std::exit(EXIT_FAILURE);
    }
}

using boost::lockfree::spsc_queue;
using boost::lockfree::fixed_sized;

template<template<typename> class QueueT>
void BENCHMARK_queue(benchmark::State &state) {
  using queue_type = QueueT<std::int_fast64_t>;
  queue_type queue(queueSize);

    auto t = std::jthread([&] {
      pinThread(cpu1);
      // pop warmup
      for (std::int_fast64_t i = 0; i < queueSize; ++i) {
        std::int_fast64_t val = 0;
        while (auto again = not queue.pop(val)) benchmark::DoNotOptimize(again);
      }

      for (std::int_fast64_t i = 0; i < iters; ++i) {
        std::int_fast64_t val = 0;
        while (auto again = not queue.pop(val)) benchmark::DoNotOptimize(again);
        if (val != i) {
          throw std::runtime_error("Value does not match");
        }
      }
    });


    pinThread(cpu2);
    // push warmup
    for (std::int_fast64_t i = 0; i < queueSize; ++i) {
      while (auto again = not queue.push(i)) benchmark::DoNotOptimize(again);
    }
    while (auto again = not queue.empty()) benchmark::DoNotOptimize(again);

    std::int_fast64_t i = 0;
    for (; i < iters; ++i) {
      while (auto again = not queue.push(i)) benchmark::DoNotOptimize(again);
    }
    while (auto again = not queue.empty()) benchmark::DoNotOptimize(again);

    state.counters["ops/sec"] = benchmark::Counter(double(i), benchmark::Counter::kIsRate);
    state.PauseTiming();
    queue.push(-1);
}

BENCHMARK_TEMPLATE(BENCHMARK_queue, WFQueue);

#ifdef WFQUEUE_WITH_BOOST
BENCHMARK_TEMPLATE(BENCHMARK_queue, spsc_queue);
#endif

BENCHMARK_MAIN();
