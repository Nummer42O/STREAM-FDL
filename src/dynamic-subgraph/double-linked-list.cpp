#include "dynamic-subgraph/double-linked-list.hpp"

#include <cassert>


template<>
template<>
_Element<Node>::_Element(_Element<Node> *prev, _Element<Node> *next, const NodeResponse &response, requestId_t requestId):
  mPrev(prev),
  mNext(next),
  mData(Data{
    Node(response),
    requestId,
    AtomicCounter(1ul)
  })
{}
template<>
template<>
_Element<Topic>::_Element(_Element<Topic> *prev, _Element<Topic> *next, const TopicResponse &response, requestId_t requestId):
  mPrev(prev),
  mNext(next),
  mData(Data{
    Topic(response),
    requestId,
    AtomicCounter(1ul)
  })
{}

template class _Element<Node>;
template class _Element<Topic>;


template<typename T>
_Iterator<T> &_Iterator<T>::operator=(const _Iterator<T> &other)
{
  mpElement = other.mpElement;

  return *this;
}

template<typename T>
_Iterator<T> _Iterator<T>::operator++()
{
  assert(mpElement);
  _Iterator<T> oldValue(mpElement);
  mpElement = mpElement->mNext;

  return oldValue;
}
template<typename T>
_Iterator<T> &_Iterator<T>::operator++(int)
{
  assert(mpElement);
  mpElement = mpElement->mNext;

  return *this;
}
template<typename T>
_Iterator<T> _Iterator<T>::operator--()
{
  assert(mpElement);
  _Iterator<T> oldValue(mpElement);
  mpElement = mpElement->mPrev;

  return oldValue;
}
template<typename T>
_Iterator<T> &_Iterator<T>::operator--(int)
{
  assert(mpElement);
  mpElement = mpElement->mPrev;

  return *this;
}

template class _Iterator<Node>;
template class _Iterator<Topic>;


template<typename T>
DoubleLinkedList<T>::DoubleLinkedList()
{
  mBegin = mEnd = nullptr;
}

template<typename T>
DoubleLinkedList<T>::~DoubleLinkedList()
{
  if (!mBegin)
  {
    assert(!mEnd);
    return;
  }

  element_type *current = mBegin;
  while (current)
  {
    element_type *next = current->mNext;
    //! NOTE: this is for peace of mind: the current should obviously be the previous of the next (say that 10 times in a row)
    //!       similarly, if the next is invalid we must be the last in line, so we check for that
    if (next)
      assert(next->mPrev == current);
    else
      assert(current == mEnd);

    delete current;
    current = next;
  }
}

template<>
template<>
DoubleLinkedList<Node>::iterator DoubleLinkedList<Node>::emplace_back(const NodeResponse &response, requestId_t requestId)
{
  element_type *newElement = new element_type(mEnd, nullptr, response, requestId);
  if (mEnd)
    mEnd->mNext = newElement;
  else
    mBegin = newElement;
  mEnd = newElement;

  return iterator(newElement);
}

template<>
template<>
DoubleLinkedList<Topic>::iterator DoubleLinkedList<Topic>::emplace_back(const TopicResponse &response, requestId_t requestId)
{
  element_type *newElement = new element_type(mEnd, nullptr, response, requestId);
  if (mEnd)
    mEnd->mNext = newElement;
  else
    mBegin = newElement;
  mEnd = newElement;

  return iterator(newElement);
}

template<typename T>
DoubleLinkedList<T>::iterator DoubleLinkedList<T>::erase(iterator it)
{
  element_type
    *curr = it.mpElement,
    *prev = curr->mPrev,
    *next = curr->mNext;
  if (prev)
    prev->mNext = next;
  if (next)
    next->mPrev = prev;

  delete curr;

  return iterator(next);
}

template<typename T>
DoubleLinkedList<T>::iterator DoubleLinkedList<T>::find(const PrimaryKey &primary)
{
  for (iterator it = this->begin(); it != this->end(); ++it)
    if (it->instance.mPrimaryKey == primary)
      return it;

  return this->end();
}

template<>
DoubleLinkedList<Node>::iterator DoubleLinkedList<Node>::findName(const std::string &name)
{
  for (iterator it = this->begin(); it != this->end(); ++it)
  {
    if (it->instance.mName == name)
      return it;
  }

  return this->end();
}

template<>
DoubleLinkedList<Topic>::iterator DoubleLinkedList<Topic>::findName(const std::string &name)
{
  for (iterator it = this->begin(); it != this->end(); ++it)
  {
    if (it->instance.mName == name)
      return it;
  }

  return this->end();
}

template class DoubleLinkedList<Node>;
template class DoubleLinkedList<Topic>;
