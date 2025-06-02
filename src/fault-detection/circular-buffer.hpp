#pragma once

#include "dynamic-subgraph/members.hpp"


class CircularBuffer
{
private:
  using Buffer = std::vector<double>;

public:
  using value_type = double;
  using iterator = Buffer::iterator;
  using const_iterator = Buffer::const_iterator;
  using index_type = size_t;

public:
  CircularBuffer(
    size_t maxSize
  );

  CircularBuffer(const CircularBuffer &other);
  CircularBuffer &operator=(const CircularBuffer &other);
  CircularBuffer(CircularBuffer &&other);
  CircularBuffer &operator=(CircularBuffer &&other);

  iterator push_back(
    double value
  );

  double &at(
    index_type i
  ) { return mBuffer.at(i); }
  double &operator[](
    index_type i
  ) { return mBuffer[i]; }

  const_iterator begin() const { return mBuffer.begin(); }
  const_iterator end() const { return mBuffer.end(); }
  const_iterator current() const { return mCurrent; }

  size_t size() const { return mBuffer.size(); }
  size_t maxSize() const { return mMaxSize; }
  bool isFull() const { return mBuffer.size() == mMaxSize; }

private:
  size_t mMaxSize;
  Buffer mBuffer;
  iterator mCurrent;
};

using AttributeWindow = std::map<Member::AttributeNameType, CircularBuffer>;
AttributeWindow createAttrWindow(const Member::AttributeMapping &attributeMapping);
void updateAttrWindow(AttributeWindow &window, const Member::AttributeMapping &attributeMapping);

using MemberWindow = std::map<Member::Ptr, AttributeWindow>;