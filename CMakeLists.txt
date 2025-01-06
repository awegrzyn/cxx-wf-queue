CMAKE_MINIMUM_REQUIRED(VERSION 3.11.0 FATAL_ERROR)

project(WFQueue
  VERSION 0.0.5
  LANGUAGES CXX
)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

find_file(SPSCQueueIncludes SPSCQueue.h PATHS 3rd-party/SPSCQueue/include/rigtorp NO_DEFAULT_PATH)

find_package(benchmark)
find_package(Boost 1.87)
find_package(mimalloc 3.0 REQUIRED)

add_executable(bench bench_queue.cxx)
target_compile_options(bench PRIVATE -Wno-interference-size -Wall -pedantic -Wextra -Werror -O3)
target_link_libraries(bench PRIVATE mimalloc-static)
target_compile_features(bench PRIVATE cxx_std_23)

if (benchmark_FOUND)
  add_executable(gbench gbench_queue.cxx)

  target_compile_options(gbench PRIVATE -Wno-interference-size -Wall -pedantic -Wextra -Werror -O2)

  if (Boost_FOUND)
    target_compile_definitions(gbench PRIVATE WFQUEUE_WITH_BOOST)
  endif()

  if (SPSCQueueIncludes)
    target_include_directories(gbench PRIVATE 3rd-party/SPSCQueue/include)
    target_compile_definitions(gbench PRIVATE WFQUEUE_WITH_SPSCQUEUE)
  endif()

  target_link_libraries(gbench
    PRIVATE
      benchmark::benchmark
      mimalloc-static
      $<$<BOOL:${Boost_FOUND}>:Boost::boost>
      $<$<BOOL:${THREAD_SANITIZER}>:pthread>
      $<$<BOOL:${THREAD_SANITIZER}>:tsan>
  )

  target_compile_features(gbench PRIVATE cxx_std_23)

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options(gbench PRIVATE -fsanitize=undefined -fsanitize=address)
  endif()
endif()
