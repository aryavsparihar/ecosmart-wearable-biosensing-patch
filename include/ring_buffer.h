// Fixed-capacity ring buffer with static storage (no heap).
//
// Elements are indexed oldest-first: at(0) is the oldest sample still held,
// at(size() - 1) the newest. Pushing into a full buffer overwrites the oldest.
#pragma once

#include <stdint.h>

template <typename T, uint8_t N>
class RingBuffer {
  static_assert(N > 0, "RingBuffer capacity must be positive");

 public:
  RingBuffer() : head_(0), count_(0) {}

  void push(T value) {
    data_[head_] = value;
    head_ = static_cast<uint8_t>((head_ + 1) % N);
    if (count_ < N) ++count_;
  }

  // Oldest-first access; index must be < size().
  T at(uint8_t index) const {
    uint8_t start = static_cast<uint8_t>((head_ + N - count_) % N);
    return data_[(start + index) % N];
  }

  T newest() const { return at(static_cast<uint8_t>(count_ - 1)); }

  // Copies the contents oldest-first into `out` (must hold size() elements).
  void copyTo(T* out) const {
    for (uint8_t i = 0; i < count_; ++i) out[i] = at(i);
  }

  void clear() {
    head_ = 0;
    count_ = 0;
  }

  uint8_t size() const { return count_; }
  bool full() const { return count_ == N; }
  bool empty() const { return count_ == 0; }
  static constexpr uint8_t capacity() { return N; }

 private:
  T data_[N];
  uint8_t head_;
  uint8_t count_;
};
