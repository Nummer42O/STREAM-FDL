#pragma once

#include "dynamic-subgraph/members.hpp"

#include "ipc/common.hpp"
#include "ipc/datastructs/information-datastructs.hpp"
#include "ipc/sharedMem.hpp"
#include "ipc/ipc-client.hpp"

#include <string>
#include <string_view>
#include <memory>
#include <vector>

#define PROJECT_ID  0


class DataStore
{
public:
  using Ptr = DataStore *const; // std::shared_ptr<DataStore>;

private:
  template<typename T>
  struct MemberData
  {
    T instance;
    requestId_t requestId;
    size_t useCounter;
  };
  using Nodes = std::map<Member::Primary, MemberData<Node>>;
  using Topics = std::map<Member::Primary, MemberData<Topic>>;

public:
  static Ptr get() { return &smInstance; }
  const Member::Ptr getNode(
    Member::Primary primary
  );
  const Member::Ptr getNode(
    const std::string &name
  );
  void removeNode(
    Member::Primary primary
  );
  const Member::Ptr getTopic(
    Member::Primary primary
  );
  const Member::Ptr getTopic(
    const std::string &name
  );
  void removeTopic(
    Member::Primary primary
  );

  void run();

private:
  DataStore();

  Node *requestNode(
    Member::Primary primary,
    bool updates
  );

  Topic *requestTopic(
    Member::Primary primary,
    bool updates
  );

private:
  Nodes         mNodes;
  Topics        mTopics;
  IpcClient     mIpcClient;

  static DataStore  smInstance;
};
