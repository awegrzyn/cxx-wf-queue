#include "WfQueue.cxx"

#ifdef WFQUEUE_WITH_BOOST
#include <boost/lockfree/spsc_queue.hpp>
#endif

#ifdef WFQUEUE_WITH_SPSCQUEUE
#include <rigtorp/SPSCQueue.h>
#endif

#include <thread>
#include <iostream>
#include <benchmark/benchmark.h>
#include "helper.h"

constexpr auto cpu1 = 1;
constexpr auto cpu2 = 2;
constexpr auto iters = 400'000'000l;
constexpr auto queueSize = 65536;

template<typename T>
struct isRigtorp : std::false_type {};

template<typename ValueT>
struct isRigtorp<rigtorp::SPSCQueue<ValueT>> : std::true_type {};

template<template<typename> class QueueT>
void BENCHMARK_queue(benchmark::State &state) {
  using queue_type = QueueT<std::int_fast64_t>;
  queue_type queue(queueSize);

    auto t = std::jthread([&] {
      pinThread(cpu1);
      // pop warmup
      for (std::int_fast64_t i = 0; i < queueSize; ++i) {
        std::int_fast64_t val = 0;
        if constexpr(isRigtorp<queue_type>::value) {
          while (not queue.front()) ;
          benchmark::DoNotOptimize(val = *queue.front());
          queue.pop();
        } else {
          while (auto again = not queue.pop(val)) benchmark::DoNotOptimize(again);
        }
      }

      for (std::int_fast64_t i = 0; i < iters; ++i) {
        std::int_fast64_t val = 0;
        if constexpr(isRigtorp<queue_type>::value) {
          while (not queue.front()) ;
          benchmark::DoNotOptimize(val = *queue.front());
          queue.pop();
        } else {
          while (auto again = not queue.pop(val)) benchmark::DoNotOptimize(again);
        }
        if (val != i) {
          throw std::runtime_error("Value does not match");
        }
      }
    });


    pinThread(cpu2);
    // push warmup
    for (std::int_fast64_t i = 0; i < queueSize; ++i) {
      if constexpr(isRigtorp<queue_type>::value) {
        while (auto again = not queue.try_push(i)) benchmark::DoNotOptimize(again);
      } else {
        while (auto again = not queue.push(i)) benchmark::DoNotOptimize(again);
      }
    }
    while (auto again = not queue.empty()) benchmark::DoNotOptimize(again);

    std::int_fast64_t i = 0;
    for (; i < iters; ++i) {
      if constexpr(isRigtorp<queue_type>::value) {
        while (auto again = not queue.try_push(i)) benchmark::DoNotOptimize(again);
      } else {
        while (auto again = not queue.push(i)) benchmark::DoNotOptimize(again);
      }
    }
    while (auto again = not queue.empty()) benchmark::DoNotOptimize(again);

    state.counters["ops/sec"] = benchmark::Counter(double(i), benchmark::Counter::kIsRate);
    state.PauseTiming();
    queue.push(-1);
}

BENCHMARK_TEMPLATE(BENCHMARK_queue, WfQueue);

#ifdef WFQUEUE_WITH_BOOST
BENCHMARK_TEMPLATE(BENCHMARK_queue, boost::lockfree::spsc_queue);
#endif

#ifdef WFQUEUE_WITH_SPSCQUEUE
BENCHMARK_TEMPLATE(BENCHMARK_queue, rigtorp::SPSCQueue);
#endif

BENCHMARK_MAIN();
