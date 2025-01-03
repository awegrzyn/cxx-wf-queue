CMAKE_MINIMUM_REQUIRED(VERSION 3.11.0 FATAL_ERROR)

project(WFQueue
  VERSION 0.0.5
  LANGUAGES CXX
)

option(THREAD_SANITIZER OFF)

set(Boost_USE_STATIC_RUNTIME OFF)
set(Boost_USE_STATIC_LIBS OFF)
set(BUILD_SHARED_LIBS ON)
set(Boost_USE_MULTITHREADED ON)

find_package(benchmark REQUIRED)
find_package(Boost 1.87 REQUIRED)

add_executable(bench bench_queue.cxx)
target_compile_options(bench PRIVATE -Wno-interference-size)
target_link_libraries(bench PRIVATE benchmark::benchmark)
target_compile_features(bench PRIVATE cxx_std_23)
if (THREAD_SANITIZER)
    target_compile_options(bench PRIVATE -fsanitize=thread)
    target_link_libraries(bench PRIVATE pthread tsan)
endif()

add_executable(bench_boost bench_boost_lockfree.cxx)
target_link_libraries(bench_boost PRIVATE benchmark::benchmark Boost::boost)
target_compile_features(bench_boost PRIVATE cxx_std_23)
