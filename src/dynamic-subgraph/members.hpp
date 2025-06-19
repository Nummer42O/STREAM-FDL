#pragma once

#include "dynamic-subgraph/member-base.hpp"

#include "ipc/datastructs/information-datastructs.hpp"
#include "ipc/datastructs/sharedMem-datastructs.hpp"
#include "ipc/sharedMem.hpp"
#include "ipc/common.hpp"
#include "ipc/util.hpp"
#include "common.hpp"

#include <chrono>
namespace cr = std::chrono;
#include <string>
#include <string_view>
#include <chrono>
#include <vector>
#include <map>


class Node: public Member
{
friend class DataStore;
friend class Graph;
template<typename T>
friend class _Element;

public:
  using ServiceMapping = std::map<std::string, MemberProxy>;
  using Clients = MemberProxies;
  using ClientMapping = std::map<std::string, Clients>;

public:
  void update(
    const NodePublishersToUpdate &update
  ) { mPublishesTo.emplace_back(update.publishesTo, true); }
  void update(
    const NodeSubscribersToUpdate &update
  ) { mSubscribesTo.emplace_back(update.subscribesTo, true); }
  void update(
    const NodeIsServerForUpdate &update
  );
  void update(
    const NodeIsClientOfUpdate &update
  ) { mServers.emplace(util::parseString(update.srvName), MemberProxy(update.serverNodeId, false)); }
  void update(
    const NodeIsActionServerForUpdate &update
  );
  void update(
    const NodeIsActionClientOfUpdate &update
  ) { mActionServers.emplace(util::parseString(update.srvName), MemberProxy(update.actionserverNodeId, false)); }
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
  Timestamp       mAliveChangeTime;
  uint32_t        mBootCount;
  pid_t           mProcessId;

private:
  ClientMapping   mClients, // Node provides services with those clients
                  mActionClients; // Node provides services for actions with those clients
  ServiceMapping  mServers, // Node listens to other services with those servers
                  mActionServers; // Node listens to other services for actions with those servers
  MemberProxies   mPublishesTo,
                  mSubscribesTo;
};


class Topic: public Member
{
friend class DataStore;
friend class Graph;
template<typename T>
friend class _Element;

public:
  struct CommunicationEdge
  {
    CommunicationEdge(
      PrimaryKey edge,
      PrimaryKey node
    ):
      primaryKey(edge),
      associatedNode{node, false}
    {}

    PrimaryKey primaryKey;
    MemberProxy associatedNode;
  };
  using Edges = std::vector<CommunicationEdge>;

public:
  void update(
    const TopicPublishersUpdate &update
  ) { mPublishers.emplace_back(update.edge, update.publisher); }
  void update(
    const TopicSubscribersUpdate &update
  ) { mSubscribers.emplace_back(update.edge, update.subscriber); }

private:
  Topic(
    const TopicResponse &response
  );

public:
  std::string     mName;
  std::string     mType;

private:
  Edges           mPublishers,
                  mSubscribers;
};
