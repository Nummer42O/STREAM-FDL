#pragma once

#include "dynamic-subgraph/members.hpp"


class CircularBuffer
{
friend class FaultDetection;

public:
  using value_type = double;
  using buffer_type = std::vector<value_type>;
  using iterator = buffer_type::iterator;
  using const_iterator = buffer_type::const_iterator;
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
  ) { return mBuffer.at(i); }
  value_type &operator[](
    index_type i
  ) { return mBuffer[i]; }

  const_iterator begin() const { return mBuffer.begin(); }
  const_iterator end() const { return mBuffer.end(); }
  const_iterator current() const { return mCurrent; }
  const_iterator current(ptrdiff_t offset) const;

  size_t size() const { return mBuffer.size(); }
  size_t maxSize() const { return mMaxSize; }
  bool full() const { return mBuffer.size() == mMaxSize; }
  bool empty() const { return mBuffer.empty(); }

  double getMean() const;
  double getStdDev(
    double mean
  ) const;

private:
  size_t mMaxSize;
  buffer_type mBuffer;
  iterator mCurrent;
};
