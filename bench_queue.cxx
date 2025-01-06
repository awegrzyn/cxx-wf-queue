#include "WfQueue.cxx"
#include <thread>
#include <iostream>
#include "helper.h"

constexpr auto cpu1 = 1;
constexpr auto cpu2 = 2;
constexpr auto iters = 400'000'000l;
constexpr auto queueSize = 65536;

int main() {
  WfQueue<std::int_fast64_t> queue(queueSize);
  auto t = std::jthread([&] {
    pinThread(cpu1);
    // pop warmup
    for (std::int_fast64_t i = 0; i < queueSize; ++i) {
      std::int_fast64_t val = 0;
      while (not queue.pop(val)) ;
    }

    for (std::int_fast64_t i = 0; i < iters; ++i) {
      std::int_fast64_t val = 0;
      while (not queue.pop(val)) ;
      if (val != i) {
        throw std::runtime_error("Value does not match");
      }
    }
  });


  pinThread(cpu2);
  // push warmup
  for (std::int_fast64_t i = 0; i < queueSize; ++i) {
    while (not queue.push(i)) ;
  }
  while (not queue.empty()) ;

  std::int_fast64_t i = 0;
  for (; i < iters; ++i) {
    while (not queue.push(i)) ;
  }
  while (not queue.empty()) ;
  return 0;
}
