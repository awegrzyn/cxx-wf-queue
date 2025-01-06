#include <mimalloc.h>
#include <memory>
#include <atomic>
#include <cassert>


// Thread-safe, wait-free queue
// 
// Limitations:
//   1. Signle writer and single reader threads
//   2. Queue size must be power of 2 (due to module operation optimization)
//   3. T must be trivial and atomic of size_type must be lock free
//
// Credits:
//  - Heavily-inspirted by Charles Frasch presentation at CppCon 2023 and Erik Rigtorp's SPSCQueue
//  - Module optimisation: "n % 2^i = n & (2^i - 1)"
//
template<typename T> requires std::is_trivial_v<T>
class WFQueue {
 public:
  using pointer = T*;
  using A = mi_stl_allocator<T>;
  using size_type = typename std::allocator_traits<A>::size_type;

  WFQueue(size_type capacity) :
    mAlloc{},
    mMask{capacity - 1},
    mQueue{std::allocator_traits<A>::allocate(mAlloc, capacity)},
    mPopIdxForPushLoop{},
    mPushIdxForPopLoop{}
    {}

  WFQueue(WFQueue const&) = delete;
  WFQueue& operator=(WFQueue const&) = delete;
  WFQueue(WFQueue&&) = delete;
  WFQueue& operator=(WFQueue&&) = delete;

  auto empty() const noexcept {
    return mPushIdx == mPopIdx;
  }

  auto full() const noexcept {
    return mPushIdx - mPopIdx >= capacity();
  }

  auto push(T const& val) {
    auto pushIdx = mPushIdx.load(std::memory_order_relaxed);
    if (full(mPopIdxForPushLoop, pushIdx)) {
      mPopIdxForPushLoop = mPopIdx.load(std::memory_order_acquire);
      if (full(mPopIdxForPushLoop, pushIdx)) {
        return false;
      }
    }
    new (&mQueue[pushIdx & mMask]) T(val);
    mPushIdx.store(pushIdx + 1, std::memory_order_release);
    return true;
  }

  auto pop(T& val) {
    auto popIdx = mPopIdx.load(std::memory_order_relaxed);
    if (empty(popIdx, mPushIdxForPopLoop)) {
      mPushIdxForPopLoop = mPushIdx.load(std::memory_order_acquire);
      if (empty(popIdx, mPushIdxForPopLoop)) {
        return false;
      }
    }
    val = mQueue[popIdx & mMask];
    mQueue[popIdx & mMask].~T();
    mPopIdx.store(popIdx + 1, std::memory_order_release);
    return true;
  }

  ~WFQueue() {
    while(not empty()) {
      mQueue[mPopIdx++ & mMask].~T();
    }
    std::allocator_traits<A>::deallocate(mAlloc, mQueue, capacity());
  }

 private:
  A mAlloc;
  const size_type mMask;
  pointer mQueue;
  static_assert(std::atomic<size_type>::is_always_lock_free);
  alignas(std::hardware_destructive_interference_size) std::atomic<size_type> mPushIdx;
  alignas(std::hardware_destructive_interference_size) size_type mPopIdxForPushLoop;
  alignas(std::hardware_destructive_interference_size) std::atomic<size_type> mPopIdx;
  alignas(std::hardware_destructive_interference_size) size_type mPushIdxForPopLoop;
  char _mPadding[std::hardware_destructive_interference_size - sizeof(size_type)];

 private:
  auto empty(size_type popIdx, size_type pushIdx) const noexcept {
    return pushIdx == popIdx;
  }

  auto full(size_type popIdx, size_type pushIdx) const noexcept {
     return pushIdx - popIdx >= capacity();
  }
  auto capacity() const noexcept {
    return mMask + 1;
  }
};
