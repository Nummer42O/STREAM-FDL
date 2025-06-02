#pragma once

#include "ipc/datastructs/information-datastructs.hpp"
#include "ipc/datastructs/sharedMem-datastructs.hpp"
#include "ipc/sharedMem.hpp"
#include "ipc/util.hpp"
#include "common.hpp"

#include <chrono>
namespace cr = std::chrono;
#include <string>
#include <string_view>
#include <chrono>
#include <vector>
#include <map>
#include <set>


//! TODO: are those forward declarations still necessary?
class Node;
class Topic;


class Member
{
friend class DataStore;

public:
  using AttributeNameType = int;
  using AttributeMapping = std::map<AttributeNameType, double>;
  using SharedMemory = sharedMem::SHMChannel<sharedMem::Response>;

  using Primary = primaryKey_t;
  //! TODO: only temporary fix for now as vector elements can not be const
  using Ptr = Member *; //const;

protected:
  struct Attribute
  {
    AttributeNameType name;
    SharedMemory sharedMemory;
    requestId_t requestId;
    double lastValue;
  };
  using Attributes = std::vector<Attribute>;

protected:
  inline Member(
    bool isTopic,
    primaryKey_t primaryKey
  ):
    mIsTopic(isTopic),
    mPrimaryKey(primaryKey)
  {}

public:
  AttributeMapping getAttributes();
  void addAttributeSource(
    AttributeNameType attributeName,
    const SingleAttributesResponse &response
  );

public:
  const bool mIsTopic;

  primaryKey_t mPrimaryKey;
  Attributes  mAttributes;
};
// using MemberPrimary = primaryKey_t;
// using MemberPtr = Member *const;
using Members = std::vector<Member::Ptr>;
using MemberIds = std::set<Member::Primary>;


class Node: public Member
{
friend class DataStore;

public:
  using ServiceMapping = std::map<std::string, Member::Primary>;
  using Clients = std::set<Member::Primary>;
  using ClientMapping = std::map<std::string, Clients>;

public:
  void update(
    const NodePublishersToUpdate &update
  ) { mPublishesTo.insert(update.publishesTo); }
  void update(
    const NodeSubscribersToUpdate &update
  ) { mSubrscribesTo.insert(update.subscribesTo); }
  void update(
    const NodeIsServerForUpdate &update
  );
  void update(
    const NodeIsClientOfUpdate &update
  ) { mServers.emplace(util::parseString(update.srvName), update.serverNodeId); }
  void update(
    const NodeIsActionServerForUpdate &update
  );
  void update(
    const NodeIsActionClientOfUpdate &update
  ) { mServers.emplace(util::parseString(update.srvName), update.actionserverNodeId); }
  void update(
    const NodeStateUpdate &update
  );

private:
  Node(
    const NodeResponse &response
  );

public:
  std::string     mName;
  std::string     mPkgName;
  bool            mAlive;
  timestamp_t     mAliveChangeTime;
  uint32_t        mBootCount;
  pid_t           mProcessId;

private:
  ClientMapping   mClients, // Node provides services with those clients
                  mActionClients; // Node provides services for actions with those clients
  ServiceMapping  mServers, // Node listens to other services with those servers
                  mActionServers; // Node listens to other services for actions with those servers
  MemberIds       mPublishesTo,
                  mSubrscribesTo;
};


class Topic: public Member
{
friend class DataStore;

public:
  void update(
    const TopicPublishersUpdate &update
  ) { mPublishers.insert(update.publisher); }
  void update(
    const TopicSubscribersUpdate &update
  ) { mSubscribers.insert(update.subscriber); }

private:
  Topic(
    const TopicResponse &response
  );

private:
  std::string     mName;
  std::string     mType;

private:
  MemberIds       mPublishers,
                  mSubscribers;
};
