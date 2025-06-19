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
  SharedMemory getCpuUtilisationMemory() const;

  void run(
    const std::atomic<bool> &running
  );

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
  std::mutex    mNodesMutex, mTopicsMutex;
  IpcClient     mIpcClient;

  static DataStore smInstance;
};
