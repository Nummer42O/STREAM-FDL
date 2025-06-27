#pragma once

#include "dynamic-subgraph/members.hpp"
#include "dynamic-subgraph/atomic-counter.hpp"
#include "dynamic-subgraph/double-linked-list.hpp"

#include "ipc/common.hpp"
#include "ipc/datastructs/information-datastructs.hpp"
#include "ipc/sharedMem.hpp"
#include "ipc/ipc-client.hpp"

#include "nlohmann/json.hpp"
#include "nlohmann/detail/exceptions.hpp"
namespace json = nlohmann;

#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>
namespace cr = std::chrono;


class DataStore
{
public:
  using Ptr = DataStore *; // std::shared_ptr<DataStore>;
  using SharedMemory = Member::SharedMemory;
  struct MemberConnections
  {
    MemberProxy member;
    MemberProxies connections;
  };
  using GraphView = std::vector<MemberConnections>;

private:
  using Nodes = DoubleLinkedList<Node>;
  using Topics = DoubleLinkedList<Topic>;

public:
  DataStore(
    const json::json &config
  );

  const MemberPtr getNode(
    const PrimaryKey &primary
  );
  const MemberPtr getNodeByName(
    const std::string &name
  );
  const MemberPtr getTopic(
    const PrimaryKey &primary
  );
  const MemberPtr getTopicByName(
    const std::string &name
  );
  const MemberPtr get(
    const MemberProxy &proxy
  ) { return (proxy.mIsTopic ? getTopic(proxy.mPrimaryKey) : getNode(proxy.mPrimaryKey)); }

  GraphView getFullGraphView() const;
  MemberProxies getAllMembers() const;
  SharedMemory getCpuUtilisationMemory() const;

  void addSubUpdate(
    Nodes::iterator affected,
    PrimaryKey other
  );
  void addSendUpdate(
    Nodes::iterator affected,
    PrimaryKey other
  );
  void addPubUpdate(
    Topics::iterator affected,
    PrimaryKey other
  );
  GraphView getUpdates();
  void run(
    const std::atomic<bool> &running,
    cr::milliseconds loopTargetInterval
  );

  static constexpr bool checkTopicNameIgnored(
    const std::string &memberName
  );

  bool checkTopicPrimaryIgnored(
    const PrimaryKey &member
  ) const;

private:
  MemberPtr requestNode(
    const PrimaryKey &primary,
    bool updates
  );

  MemberPtr requestTopic(
    const PrimaryKey &primary,
    bool updates
  );

  static IpcClient tryMakeIpcClient(
    const json::json &config
  );

private:
  Nodes         mNodes;
  Topics        mTopics;
  GraphView     mUpdates;
  std::mutex    mNodesMutex,
                mTopicsMutex,
                mUpdatesMutex;
  IpcClient     mIpcClient;

  PrimaryKey    mTopicParameterEventsKey,
                mTopicRosoutKey;

  static DataStore smInstance;
};
