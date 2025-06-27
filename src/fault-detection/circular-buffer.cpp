#include "fault-detection/circular-buffer.hpp"

#include "common.hpp"

#include <cassert>
#include <numeric>
#include <cmath>
#include <algorithm>


CircularBuffer::CircularBuffer(size_t maxSize):
  mMaxSize(maxSize),
  mBuffer(std::make_shared<value_type[]>(maxSize)),
  mSize(0ul)
{
  assert(mMaxSize >= 2);
  LOG_TRACE(LOG_THIS LOG_VAR(maxSize));

  mCurrent = &mBuffer[mMaxSize];
}

CircularBuffer::CircularBuffer(const CircularBuffer &other):
  mMaxSize(other.mMaxSize),
  mSize(other.mSize)
{
  LOG_TRACE(LOG_THIS LOG_VAR(&other));

  ptrdiff_t offset = other.mCurrent - other.mBuffer.get();
  mBuffer = other.mBuffer;
  mCurrent = mBuffer.get() + offset;
}

CircularBuffer &CircularBuffer::operator=(const CircularBuffer &other)
{
  LOG_TRACE(LOG_THIS LOG_VAR(&other));

  mMaxSize = other.mMaxSize;
  ptrdiff_t offset = other.mCurrent - other.mBuffer.get();
  mBuffer = other.mBuffer;
  mCurrent = mBuffer.get() + offset;
  mSize = other.mSize;

  return *this;
}

CircularBuffer::CircularBuffer(CircularBuffer &&other):
  mMaxSize(other.mMaxSize),
  mSize(other.mSize)
{
  LOG_TRACE(LOG_THIS LOG_VAR(&other));

  ptrdiff_t offset = other.mCurrent - other.mBuffer.get();
  mBuffer = std::move(other.mBuffer);
  mCurrent = mBuffer.get() + offset;
}

CircularBuffer &CircularBuffer::operator=(CircularBuffer &&other)
{
  LOG_TRACE(LOG_THIS LOG_VAR(&other));

  mMaxSize = other.mMaxSize;
  ptrdiff_t offset = other.mCurrent - other.mBuffer.get();
  mBuffer = std::move(other.mBuffer);
  mCurrent = mBuffer.get() + offset;
  mSize = other.mSize;

  return *this;
}

CircularBuffer::iterator CircularBuffer::push(value_type value)
{
  LOG_TRACE(LOG_THIS LOG_VAR(value));

  if (mSize < mMaxSize)
    ++mSize;
  if (mCurrent == this->absoluteEnd())
    mCurrent = mBuffer.get();
  else
    ++mCurrent;

  *mCurrent = value;

  return mCurrent;
}

void CircularBuffer::reset()
{
  mSize = 0ul;
  mCurrent = &mBuffer[mMaxSize];
}

CircularBuffer::value_type &CircularBuffer::at(index_type i)
{
  LOG_TRACE(LOG_THIS LOG_VAR(i));
  assert(i < mSize);

  return mBuffer[i];
}

CircularBuffer::const_iterator CircularBuffer::current(ptrdiff_t offset) const
{
  LOG_TRACE(LOG_THIS LOG_VAR(offset));

  const_iterator beginIt = mBuffer.get();
  ptrdiff_t currentOffset = mCurrent - beginIt;

  ptrdiff_t computedOffset = (currentOffset + offset) % mSize;
  if (computedOffset < 0)
    computedOffset += mSize;

  return beginIt + computedOffset;
}

double CircularBuffer::getMean() const
{
  LOG_TRACE(LOG_THIS);

  assert(mSize >= 1);

  double mean = std::accumulate(
    this->begin(), this->end(), 0.0,
    [](double accumulator, double value) -> double
    {
      return accumulator + value;
    }
  ) / mSize;

  return mean;
}

double CircularBuffer::getStdDev(double mean) const
{
  LOG_TRACE(LOG_THIS LOG_VAR(mean));

  assert(mSize >= 1);

  double stdDev = std::sqrt(std::accumulate(
    this->begin(), this->end(), 0.0,
    [mean](double accumulator, double value) -> double
    {
      return accumulator + (value - mean)*(value - mean);
    }
  ) / mSize);

  /*! NOTE: alternative version based on unbiased sample variance
  double stdDev = std::sqrt(std::accumulate(
    this->begin(), this->end(), 0.0,
    [mSize, mean](double accumulator, double value) -> double
    {
      return accumulator + ((value - mean)*(value - mean) / (mSize - 1));
    }
  ));
  */

  return stdDev;
}


