#pragma once

#include "dynamic-subgraph/members.hpp"
#include "dynamic-subgraph/atomic-counter.hpp"

#include "ipc/datastructs/information-datastructs.hpp"
#include "ipc/common.hpp"


template<typename T>
class _Element
{
public:
  struct Data {
    T instance;
    requestId_t requestId;
    AtomicCounter useCounter;
  };

public:
  template<typename U>
  _Element(
    _Element<T> *prev,
    _Element<T> *next,
    const U &request,
    requestId_t requestId
  );

public:
  _Element *mPrev, *mNext;
  Data mData;
};

template<>
template<>
_Element<Node>::_Element(_Element<Node> *prev, _Element<Node> *next, const NodeResponse &response, requestId_t requestId);
template<>
template<>
_Element<Topic>::_Element(_Element<Topic> *prev, _Element<Topic> *next, const TopicResponse &response, requestId_t requestId);


template<typename T>
class _Iterator
{
public:
  using iterator_category = std::random_access_iterator_tag;
  using difference_type   = std::ptrdiff_t;
  using value_type        = _Element<T>::Data;
  using value_pointer     = _Element<T>::Data*;
  using value_reference   = _Element<T>::Data&;

public:
  _Iterator(_Element<T> *start = nullptr):
    mpElement(start)
  {}
  _Iterator(const _Iterator<T> &other):
    mpElement(other.mpElement)
  {}
  _Iterator<T> &operator=(const _Iterator<T> &other);

  _Iterator<T> operator++();
  _Iterator<T> &operator++(int);
  _Iterator<T> operator--();
  _Iterator<T> &operator--(int);

  value_reference operator*() { return mpElement->mData; }
  const value_reference operator*() const { return mpElement->mData; }
  value_pointer operator->() { return &(mpElement->mData); }
  const value_pointer operator->() const { return &(mpElement->mData); }

  bool operator==(const _Iterator<T> &other) const { return mpElement == other.mpElement; }
  bool operator!=(const _Iterator<T> &other) const { return mpElement != other.mpElement; }
  operator bool() const { return mpElement != nullptr; }

public:
  _Element<T> *mpElement;
};


template<typename T>
class DoubleLinkedList
{
public:
  using value_type = _Element<T>::Data;
  using iterator = _Iterator<T>;
  using const_iterator = const _Iterator<T>;

private:
  using element_type = _Element<T>;

public:
  DoubleLinkedList();
  ~DoubleLinkedList();

  template<typename U>
  iterator emplace_back(const U &response, requestId_t requestId);

  iterator erase(iterator it);

  iterator find(const PrimaryKey &primary);
  iterator findName(const std::string &name);

  iterator begin() { return iterator(mBegin); }
  constexpr iterator end() { return iterator(nullptr); }

private:
  element_type *mBegin, *mEnd;
};

template<>
template<>
DoubleLinkedList<Node>::iterator DoubleLinkedList<Node>::emplace_back(const NodeResponse &response, requestId_t requestId);

template<>
DoubleLinkedList<Node>::iterator DoubleLinkedList<Node>::findName(const std::string &name);

template<>
template<>
DoubleLinkedList<Topic>::iterator DoubleLinkedList<Topic>::emplace_back(const TopicResponse &response, requestId_t requestId);

template<>
DoubleLinkedList<Topic>::iterator DoubleLinkedList<Topic>::findName(const std::string &name);
