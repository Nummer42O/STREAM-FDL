#pragma once

#include "dynamic-subgraph/atomic-counter.hpp"
#include "common.hpp"

#include "ipc/datastructs/information-datastructs.hpp"
#include "ipc/sharedMem.hpp"
#include "ipc/common.hpp"
#include "ipc/util.hpp"

#include <string>
#include <map>
#include <iostream>
#include <cassert>


class Node;
class Topic;

class Member
{
friend class DataStore;
friend class Graph;

public:
  using AttributeDescriptor = std::string;
  using AttributeMapping = std::map<AttributeDescriptor, double>;
  using SharedMemory = sharedMem::SHMChannel<sharedMem::Response>;

protected:
  struct Attribute
  {
    AttributeDescriptor name;
    SharedMemory sharedMemory;
    requestId_t requestId;
    double lastValue;
  };
  using Attributes = std::vector<Attribute>;

protected:
  Member(
    bool isTopic,
    PrimaryKey primaryKey
  ):
    mIsTopic(isTopic),
    mPrimaryKey(primaryKey)
  {}

public:
  AttributeMapping getAttributes() const;
  void addAttributeSource(
    const AttributeDescriptor &attributeName,
    const SingleAttributesResponse &response
  );

  friend std::ostream &operator<<(
    std::ostream &stream,
    const Member *member
  );
  friend std::ostream &operator<<(
    std::ostream &stream,
    const Member &member
  ) { return operator<<(stream, &member); }

public:
  bool                mIsTopic;
  PrimaryKey          mPrimaryKey;

protected:
  mutable Attributes  mmAttributes;
};
std::ostream &operator<<(std::ostream &stream, const Member *member);
std::ostream &operator<<(std::ostream &stream, const Member &member);


class MemberPtr
{
friend class DataStore;

public:
  MemberPtr():
    mpMember(nullptr)
  {}
  ~MemberPtr();
  MemberPtr(
    const MemberPtr &other
  );
  MemberPtr &operator=(
    const MemberPtr &other
  );
  MemberPtr(
    MemberPtr &&other
  );
  MemberPtr &operator=(
    MemberPtr &&other
  );

  Member &operator*() = delete;
  const Member *operator->() const { assert(mpMember); return mpMember; }

  operator bool () const { return mpMember != nullptr; }
  bool valid() const { return mpMember != nullptr; }

  //! NOTE: to be defined in member.cpp, where Node and Topic are comlete types
  friend const Node *asNode(
    const MemberPtr &member
  );
  friend const Topic *asTopic(
    const MemberPtr &member
  );
  friend const std::string &getName(
    const MemberPtr &member
  );

  bool operator==(
    const MemberPtr &other
  ) const { return mpMember == other.mpMember; }
  bool operator<(
    const MemberPtr &other
  ) const { return mpMember < other.mpMember; }

  friend std::ostream &operator<<(
    std::ostream &stream,
    const MemberPtr &member
  );

private:
  MemberPtr(Member *member, AtomicCounter *useCounter);

private:
  Member        *mpMember;
  AtomicCounter *mpUseCounter;
};
using Members = std::vector<MemberPtr>;
const Node *asNode(const MemberPtr &member);
const Topic *asTopic(const MemberPtr &member);
const std::string &getName(const MemberPtr &member);
std::ostream &operator<<(std::ostream &stream, const MemberPtr &member);

#define MAKE_MEMBER_PTR(it) MemberPtr(static_cast<Member *>(&((it)->instance)), &((it)->useCounter))


struct MemberProxy
{
public:
  MemberProxy(
    PrimaryKey primaryKey,
    bool isTopic
  ):
    mPrimaryKey(std::move(primaryKey)),
    mIsTopic(isTopic)
  {}
  MemberProxy(
    const char (&primaryKey)[MAX_STRING_SIZE],
    bool isTopic
  ):
    mPrimaryKey(util::parseString(primaryKey)),
    mIsTopic(isTopic)
  {}

  bool operator==(const MemberProxy &other) const { return mPrimaryKey == other.mPrimaryKey; }
  bool operator<(const MemberProxy &other) const { return mPrimaryKey < other.mPrimaryKey; }

  friend std::ostream &operator<<(
    std::ostream &stream,
    const MemberProxy &proxy
  );

  PrimaryKey mPrimaryKey;
  bool       mIsTopic;
};
using MemberProxies = std::vector<MemberProxy>;
std::ostream &operator<<(std::ostream &stream, const MemberProxy &proxy);
