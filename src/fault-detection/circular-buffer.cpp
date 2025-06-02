#include "fault-detection/circular-buffer.hpp"

#include <cassert>

//! TODO: actual value
#define CIRCULAR_BUFFER_MAX_SIZE 10


CircularBuffer::CircularBuffer(size_t maxSize):
  mMaxSize(maxSize)
{
  assert(mMaxSize > 0);
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

CircularBuffer::iterator CircularBuffer::push_back(double value)
{
  iterator it = mBuffer.insert(mCurrent++, value);
  if (mCurrent == mBuffer.end() &&
      mBuffer.size() == mMaxSize)
    mCurrent = mBuffer.begin();

  return it;
}


AttributeWindow createAttrWindow(const Member::AttributeMapping &attributeMapping)
{
  AttributeWindow attrWindow;
  for (const Member::AttributeMapping::value_type &entry: attributeMapping)
  {
    CircularBuffer cb(CIRCULAR_BUFFER_MAX_SIZE);
    cb.push_back(entry.second);
    attrWindow.emplace(entry.first, std::move(cb));
  }
  return attrWindow;
}

void updateAttrWindow(AttributeWindow &window, const Member::AttributeMapping &attributeMapping)
{
  for (const Member::AttributeMapping::value_type &entry: attributeMapping)
  {
    window.at(entry.first).push_back(entry.second);
  }
}
