#include "fault-detection/circular-buffer.hpp"

#include <cassert>
#include <numeric>
#include <cmath>
#include <algorithm>


CircularBuffer::CircularBuffer(size_t maxSize):
  mMaxSize(maxSize)
{
  assert(mMaxSize >= 2);
  mCurrent = mBuffer.begin();
}

CircularBuffer::CircularBuffer(const CircularBuffer &other):
  mMaxSize(other.mMaxSize),
  mBuffer(other.mBuffer),
  mCurrent(mBuffer.begin() + (other.mCurrent - other.mBuffer.begin()))
{}

CircularBuffer &CircularBuffer::operator=(const CircularBuffer &other)
{
  mMaxSize = other.mMaxSize;
  mBuffer = other.mBuffer;
  mCurrent = mBuffer.begin() + (other.mCurrent - other.mBuffer.begin());

  return *this;
}

CircularBuffer::CircularBuffer(CircularBuffer &&other):
  mMaxSize(other.mMaxSize),
  mBuffer(std::move(other.mBuffer)),
  //! NOTE: that should be fine from my testing
  mCurrent(std::move(other.mCurrent))
{}

CircularBuffer &CircularBuffer::operator=(CircularBuffer &&other)
{
  mMaxSize = other.mMaxSize;
  mBuffer = std::move(other.mBuffer);
  mCurrent = std::move(other.mCurrent);

  return *this;
}

CircularBuffer::iterator CircularBuffer::push(value_type value)
{
  iterator it = mBuffer.insert(mCurrent++, value);
  if (mCurrent == mBuffer.end() &&
      mBuffer.size() == mMaxSize)
    mCurrent = mBuffer.begin();

  return it;
}

CircularBuffer::const_iterator CircularBuffer::current(ptrdiff_t offset) const
{
  const_iterator beginIt = mBuffer.begin();
  ptrdiff_t currentOffset = mCurrent - beginIt;

  const size_t bufferSize = mBuffer.size();
  ptrdiff_t computedOffset = (currentOffset + offset) % bufferSize;
  if (computedOffset < 0)
    computedOffset += bufferSize;

  return beginIt + computedOffset;
}

double CircularBuffer::getMean() const
{
  const size_t bufferSize = mBuffer.size();
  assert(bufferSize >= 1);

  double mean = std::accumulate(
    mBuffer.begin(), mBuffer.end(), 0.0,
    [](double accumulator, double value) -> double
    {
      return accumulator + value;
    }
  ) / bufferSize;

  return mean;
}

double CircularBuffer::getStdDev(double mean) const
{
  const size_t bufferSize = mBuffer.size();
  assert(bufferSize >= 1);

  double stdDev = std::sqrt(std::accumulate(
    mBuffer.begin(), mBuffer.end(), 0.0,
    [mean](double accumulator, double value) -> double
    {
      return accumulator + (value - mean)*(value - mean);
    }
  ) / bufferSize);

  /*! NOTE: alternative version based on unbiased sample variance
  double stdDev = std::sqrt(std::accumulate(
    mBuffer.begin(), mBuffer.end(), 0.0,
    [bufferSize, mean](double accumulator, double value) -> double
    {
      return accumulator + ((value - mean)*(value - mean) / (bufferSize - 1));
    }
  ));
  */

  return stdDev;
}


