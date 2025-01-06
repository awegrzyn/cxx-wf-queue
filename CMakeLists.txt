CMAKE_MINIMUM_REQUIRED(VERSION 3.11.0 FATAL_ERROR)

project(WFQueue
  VERSION 0.0.5
  LANGUAGES CXX
)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

find_package(benchmark REQUIRED)
find_package(Boost 1.87)
find_package(mimalloc 2.1 REQUIRED)

add_executable(bench bench_queue.cxx)

target_compile_options(bench PRIVATE -Wno-interference-size -Wall -pedantic -Wextra -Werror -O2)

if (Boost_FOUND)
  target_compile_definitions(bench PRIVATE WFQUEUE_WITH_BOOST)
endif()

target_link_libraries(bench
  PRIVATE
    benchmark::benchmark
    mimalloc
    $<$<BOOL:${Boost_FOUND}>:Boost::boost>
    $<$<BOOL:${THREAD_SANITIZER}>:pthread>
    $<$<BOOL:${THREAD_SANITIZER}>:tsan>
)

target_compile_features(bench PRIVATE cxx_std_23)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_link_options(bench PRIVATE -fsanitize=undefined -fsanitize=address)
endif()
