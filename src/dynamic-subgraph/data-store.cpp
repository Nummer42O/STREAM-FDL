#include "dynamic-subgraph/data-store.hpp"

#include "ipc/datastructs/information-datastructs.hpp"

#include <cassert>


DataStore DataStore::smInstance = DataStore();

DataStore::DataStore():
  mIpcClient(IpcClient(PROJECT_ID))
{}

Node *DataStore::requestNode(Member::Primary primary, bool updates)
{
  requestId_t requestId;
  NodeRequest nodeRequest{
    .primaryKey = primary,
    .updates = updates
  };
  mIpcClient.sendNodeRequest(nodeRequest, requestId);

  std::pair<Nodes::iterator, bool> result = mNodes.emplace(
    primary,
    MemberData<Node>{
      .instance = Node(mIpcClient.receiveNodeResponse().value()),
      .requestId = requestId,
      .useCounter = 1ul
    }
  );
  assert(result.second);

  return &(result.first->second.instance);
}

Topic *DataStore::requestTopic(Member::Primary primary, bool updates)
{
  requestId_t requestId;
  TopicRequest topicRequest{
    .primaryKey = primary,
    .updates = updates
  };
  mIpcClient.sendTopicRequest(topicRequest, requestId);

  std::pair<Topics::iterator, bool> result = mTopics.emplace(
    primary,
    MemberData<Topic>{
      .instance = Topic(mIpcClient.receiveTopicResponse().value()),
      .requestId = requestId,
      .useCounter = 1ul
    }
  );
  assert(result.second);

  return &(result.first->second.instance);
}

const Member::Ptr DataStore::getNode(Member::Primary primary)
{
  Nodes::iterator nodeIt = mNodes.find(primary);
  if (nodeIt != mNodes.end())
  {
    ++(nodeIt->second.useCounter);
    return &(nodeIt->second.instance);
  }

  return requestNode(primary, true);
}

const Member::Ptr DataStore::getNode(const std::string &name)
{
  Nodes::iterator nodeIt = std::find_if(
    mNodes.begin(), mNodes.end(),
    [name](const Nodes::value_type &element) -> bool
    {
      return (element.second.instance.mName == name);
    }
  );
  if (nodeIt != mNodes.end())
    return &(nodeIt->second.instance);

  requestId_t searchRequestId;
  SearchRequest req{
    .type = SearchRequest::NODE
  };
  util::parseString(req.name, name);
  mIpcClient.sendSearchRequest(req, searchRequestId);

  SearchResponse resp = mIpcClient.receiveSearchResponse().value();
  if (resp.primaryKey == -1)
    return nullptr;
  else
    return requestNode(resp.primaryKey, true);
}

void DataStore::removeNode(Member::Primary primary)
{
  Nodes::iterator nodeIt = mNodes.find(primary);
  if (nodeIt != mNodes.end() && --(nodeIt->second.useCounter) == 0ul)
  {
    UnsubscribeRequest req{.id = nodeIt->second.requestId};
    requestId_t unsubReqId;
    mIpcClient.sendUnsubscribeRequest(req, unsubReqId);
    mNodes.erase(nodeIt);
  }
}

const Member::Ptr DataStore::getTopic(Member::Primary primary)
{
  Topics::iterator topicIt = mTopics.find(primary);
  if (topicIt != mTopics.end())
  {
    ++(topicIt->second.useCounter);
    return &(topicIt->second.instance);
  }

  return requestTopic(primary, true);
}

const Member::Ptr DataStore::getTopic(const std::string &name)
{
  Topics::iterator topicIt = std::find_if(
    mTopics.begin(), mTopics.end(),
    [name](const Topics::value_type &element) -> bool
    {
      return (element.second.instance.mName == name);
    }
  );
  if (topicIt != mTopics.end())
    return &(topicIt->second.instance);

  requestId_t searchRequestId;
  SearchRequest req{
    .type = SearchRequest::TOPIC
  };
  util::parseString(req.name, name);
  mIpcClient.sendSearchRequest(req, searchRequestId);

  SearchResponse resp = mIpcClient.receiveSearchResponse().value();
  if (resp.primaryKey == -1)
    return nullptr;
  else
    return requestTopic(resp.primaryKey, true);
}

void DataStore::removeTopic(Member::Primary primary)
{
  Topics::iterator topicIt = mTopics.find(primary);
  if (topicIt != mTopics.end() && --(topicIt->second.useCounter) == 0ul)
  {
    UnsubscribeRequest req{.id = topicIt->second.requestId};
    requestId_t unsubReqId;
    mIpcClient.sendUnsubscribeRequest(req, unsubReqId);
    mTopics.erase(topicIt);
  }
}