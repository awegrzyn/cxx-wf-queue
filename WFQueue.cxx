#include <memory>
#include <atomic>
#include <cassert>


// Thread-safe, wait-free queue
// 
// Make sure to use power of 2 queue size
template<typename T> requires std::is_trivial_v<T>
class WFQueue {
 public:
  using pointer = T*;
  using A = std::allocator<T>;
  using size_type = typename std::allocator_traits<A>::size_type;

  WFQueue(size_type capacity) :
    mMask{capacity - 1},
    mQueue{std::allocator_traits<A>::allocate(mAlloc, capacity)} {}

  WFQueue(WFQueue const&) = delete;
  WFQueue& operator=(WFQueue const&) = delete;
  WFQueue(WFQueue&&) = delete;
  WFQueue& operator=(WFQueue&&) = delete;

  auto empty() const noexcept {
    return mPushCursor == mPullCursor;
  }

  auto full() const noexcept {
    return mPushCursor - mPullCursor >= capacity();
  }

  auto push(T const& val) {
    auto pushCursor = mPushCursor.load(std::memory_order_relaxed);
    auto pullCursor = mPullCursor.load(std::memory_order_acquire);
    if (full(pullCursor, pushCursor)) {
      return false;
    }
    new (&mQueue[pushCursor & mMask]) T(val);
    mPushCursor.store(pushCursor + 1, std::memory_order_release);
    return true;
  }

  auto pull(T& val) {
    auto pushCursor = mPushCursor.load(std::memory_order_acquire);
    auto pullCursor = mPullCursor.load(std::memory_order_relaxed);
    if (empty(pullCursor, pushCursor)) {
      return false;
    }
    val = mQueue[pullCursor & mMask];
    mQueue[pullCursor & mMask].~T();
    mPullCursor.store(pullCursor + 1, std::memory_order_release);
    return true;
  }

  ~WFQueue() {
    while(not empty()) {
      mQueue[mPullCursor++ & mMask].~T();
    }
    std::allocator_traits<A>::deallocate(mAlloc, mQueue, capacity());
  }

 private:
  A mAlloc;
  const size_type mMask;
  pointer mQueue;
  static_assert(std::atomic<size_type>::is_always_lock_free);
  alignas(std::hardware_destructive_interference_size) std::atomic<size_type> mPushCursor;
  //alignas(std::hardware_destructive_interference_size) size_type mPullCursorForPushLoop;
  alignas(std::hardware_destructive_interference_size) std::atomic<size_type> mPullCursor;
  //alignas(std::hardware_destructive_interference_size) size_type mPushCursorForPullLoop;
  char _mPadding[std::hardware_destructive_interference_size - sizeof(size_type)];

 private:
  auto empty(size_type pullCursor, size_type pushCursor) const noexcept {
    return pushCursor == pullCursor;
  }

  auto full(size_type pullCursor, size_type pushCursor) const noexcept {
     return pushCursor - pullCursor >= capacity();
  }
  auto capacity() const noexcept {
    return mMask + 1;
  }
};
