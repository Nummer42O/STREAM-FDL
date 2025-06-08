#pragma once

#include "dynamic-subgraph/members.hpp"
#include "dynamic-subgraph/graph.hpp"

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
#include <atomic>

//#define PROJECT_ID  0


class DataStore
{
public:
  using Ptr = DataStore *; // std::shared_ptr<DataStore>;

private:
  template<typename T>
  struct MemberData
  {
    T instance;
    requestId_t requestId;
    size_t useCounter;
  };
  using Nodes = std::map<PrimaryKey, MemberData<Node>>;
  using Topics = std::map<PrimaryKey, MemberData<Topic>>;

public:
  DataStore(
    const json::json &config
  );

  const Member::Ptr getNode(
    const PrimaryKey &primary
  );
  const Member::Ptr getNodeByName(
    const std::string &name
  );
  void removeNode(
    const PrimaryKey &primary
  );
  const Member::Ptr getTopic(
    const PrimaryKey &primary
  );
  const Member::Ptr getTopicByName(
    const std::string &name
  );
  void removeTopic(
    const PrimaryKey &primary
  );

  Graph getFullGraphView() const;

  //! TODO: implement
  void run(
    const std::atomic<bool> &running
  );

private:
  Node *requestNode(
    const PrimaryKey &primary,
    bool updates
  );

  Topic *requestTopic(
    const PrimaryKey &primary,
    bool updates
  );

  static IpcClient tryMakeIpcClient(
    const json::json &config
  );

private:
  Nodes         mNodes;
  Topics        mTopics;
  IpcClient     mIpcClient;

  static DataStore smInstance;
};
