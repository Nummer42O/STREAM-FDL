#pragma once

#include "dynamic-subgraph/members.hpp"

#include <memory>


class CircularBuffer
{
friend class FaultDetection;

public:
  using value_type = double;
  using buffer_type = std::shared_ptr<value_type[]>;
  using iterator = value_type*;
  using const_iterator = const value_type*;
  using index_type = size_t;

public:
  CircularBuffer(
    size_t maxSize
  );

  CircularBuffer(const CircularBuffer &other);
  CircularBuffer &operator=(const CircularBuffer &other);
  CircularBuffer(CircularBuffer &&other);
  CircularBuffer &operator=(CircularBuffer &&other);

  iterator push(
    value_type value
  );

  value_type &at(
    index_type i
  );
  value_type &operator[](
    index_type i
  ) { return mBuffer[i]; }

  const_iterator begin() const { return mBuffer.get(); }
  const_iterator end() const { return &mBuffer[mMaxSize]; }
  const_iterator current() const { return mCurrent; }
  const_iterator current(ptrdiff_t offset) const;

  size_t size() const { return mSize; }
  size_t maxSize() const { return mMaxSize; }
  bool full() const { return mSize == mMaxSize; }
  bool empty() const { return mSize == 0; }

  double getMean() const;
  double getStdDev(
    double mean
  ) const;

private:
  size_t mMaxSize;
  buffer_type mBuffer;
  iterator mCurrent;
  size_t mSize;
};
