#pragma once

#include <cstddef>
#include <mutex>
#include <iostream>


class AtomicCounter
{
public:
  AtomicCounter(
    size_t value = 0ul
  ):
    mValue(value)
  {}
  AtomicCounter(
    AtomicCounter &&other
  ):
    mValue(other.mValue)
  {}
  AtomicCounter &operator=(
    AtomicCounter &&other
  )
  {
    mValue = other.mValue;
    return *this;
  }
  AtomicCounter(
    const AtomicCounter &other
  ):
    mValue(other.mValue)
  {}
  AtomicCounter &operator=(
    const AtomicCounter &other
  )
  {
    mValue = other.mValue;
    return *this;
  }

  void increase();
  void decrease();
  size_t get();
  bool nonZero();

  friend std::ostream &operator<<(
    std::ostream &stream,
    const AtomicCounter &counter
  );

private:
  size_t mValue;
  std::mutex mValueMutex;
};
std::ostream &operator<<(std::ostream &stream, const AtomicCounter &counter);
