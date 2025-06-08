#include "dynamic-subgraph/data-store.hpp"

#include "common.hpp"

#include "ipc/datastructs/information-datastructs.hpp"
#include "ipc/ipc-exceptions.hpp"
#include "ipc/util.hpp"

#include "nlohmann/json.hpp"
namespace json = nlohmann;

#include <cassert>
#include <optional>
#include <chrono>
namespace cr = std::chrono;
#include <thread>


DataStore::DataStore(const json::json &config):
  mIpcClient(tryMakeIpcClient(config.at(CONFIG_IPC)))
{
  LOG_TRACE(LOG_THIS LOG_VAR(config));
}

const Member::Ptr DataStore::getNode(const PrimaryKey &primary)
{
  LOG_TRACE(LOG_THIS LOG_VAR(primary));

  Nodes::iterator nodeIt = mNodes.find(primary);
  if (nodeIt != mNodes.end())
  {
    ++(nodeIt->second.useCounter);
    Node *node = &(nodeIt->second.instance);
    LOG_TRACE("Returning existing node: " << LOG_MEMBER(node) " with new counter: " << nodeIt->second.useCounter);
    return node;
  }

  return requestNode(primary, true);
}

const Member::Ptr DataStore::getNodeByName(const std::string &name)
{
  LOG_TRACE(LOG_THIS LOG_VAR(name));

  Nodes::iterator nodeIt = std::find_if(
    mNodes.begin(), mNodes.end(),
    [name](const Nodes::value_type &element) -> bool
    {
      return (element.second.instance.mName == name);
    }
  );
  if (nodeIt != mNodes.end())
  {
    ++(nodeIt->second.useCounter);
    Node *node = &(nodeIt->second.instance);
    LOG_TRACE("Returning existing node: " << LOG_MEMBER(node) " with new counter: " << nodeIt->second.useCounter);
    return node;
  }

  requestId_t searchRequestId;
  SearchRequest req{
    .type = SearchRequest::NODE
  };
  util::parseString(req.name, name);
  mIpcClient.sendSearchRequest(req, searchRequestId);

  PrimaryKey primaryKey = util::parseString(mIpcClient.receiveSearchResponse().value().primaryKey);
  LOG_TRACE("SearchRequest returned primary key: '" << primaryKey << "'");
  if (primaryKey.empty())
    return nullptr;
  else
    return requestNode(primaryKey, true);
}

void DataStore::removeNode(const PrimaryKey &primary)
{
  LOG_TRACE(LOG_THIS LOG_VAR(primary));

  Nodes::iterator nodeIt = mNodes.find(primary);
  if (nodeIt != mNodes.end() && --(nodeIt->second.useCounter) == 0ul)
  {
    LOG_TRACE("Removing " << LOG_MEMBER((&(nodeIt->second.instance))) "from data store");
    UnsubscribeRequest req{.id = nodeIt->second.requestId};
    requestId_t unsubReqId;
    mIpcClient.sendUnsubscribeRequest(req, unsubReqId);
    mNodes.erase(nodeIt);
  }
}

const Member::Ptr DataStore::getTopic(const PrimaryKey &primary)
{
  LOG_TRACE(LOG_THIS LOG_VAR(primary));

  Topics::iterator topicIt = mTopics.find(primary);
  if (topicIt != mTopics.end())
  {
    ++(topicIt->second.useCounter);
    Topic *topic = &(topicIt->second.instance);
    LOG_TRACE("Returning existing topic: " << LOG_MEMBER(topic) " with new counter: " << topicIt->second.useCounter);
    return topic;
  }

  return requestTopic(primary, true);
}

const Member::Ptr DataStore::getTopicByName(const std::string &name)
{
  LOG_TRACE(LOG_THIS LOG_VAR(name));

  Topics::iterator topicIt = std::find_if(
    mTopics.begin(), mTopics.end(),
    [name](const Topics::value_type &element) -> bool
    {
      return (element.second.instance.mName == name);
    }
  );
  if (topicIt != mTopics.end())
  {
    ++(topicIt->second.useCounter);
    Topic *topic = &(topicIt->second.instance);
    LOG_TRACE("Returning existing topic: " << LOG_MEMBER(topic) " with new counter: " << topicIt->second.useCounter);
    return topic;
  }

  requestId_t searchRequestId;
  SearchRequest req{
    .type = SearchRequest::TOPIC
  };
  util::parseString(req.name, name);
  mIpcClient.sendSearchRequest(req, searchRequestId);

  PrimaryKey primaryKey = util::parseString(mIpcClient.receiveSearchResponse().value().primaryKey);
  LOG_TRACE("SearchRequest returned primary key: '" << primaryKey << "'");
  if (primaryKey.empty())
    return nullptr;
  else
    return requestTopic(primaryKey, true);
}

void DataStore::removeTopic(const PrimaryKey &primary)
{
  LOG_TRACE(LOG_THIS LOG_VAR(primary));

  Topics::iterator topicIt = mTopics.find(primary);
  if (topicIt != mTopics.end() && --(topicIt->second.useCounter) == 0ul)
  {
    LOG_TRACE("Removing " << LOG_MEMBER((&(topicIt->second.instance))) "from data store");
    UnsubscribeRequest req{.id = topicIt->second.requestId};
    requestId_t unsubReqId;
    mIpcClient.sendUnsubscribeRequest(req, unsubReqId);
    mTopics.erase(topicIt);
  }
}

Graph DataStore::getFullGraphView() const
{
  LOG_TRACE(LOG_THIS);

  // send a request for the whole graph structure
  CustomMemberRequest req{
    .query = {
      {'O', 'P', 'T', 'I', 'O', 'N', 'A', 'L', ' ', 'M', 'A', 'T', 'C', 'H', ' ', '(', 'a', ':', 'A', 'c', 't', 'i', 'v', 'e', ')', ' ', 'W', 'H', 'E', 'R', 'E', ' ', '(', 'a', ')', '-', '[', ':', 'p', 'u', 'b', 'l', 'i', 's', 'h', 'i', 'n', 'g', ']', '-', '>', '(', ')', ' ', 'O', 'R', ' ', '(', ')', '-', '[', ':', 's', 'u'},
      {'b', 's', 'c', 'r', 'i', 'b', 'i', 'n', 'g', ']', '-', '>', '(', 'a', ')', ' ', 'O', 'R', ' ', '(', 'a', ')', '-', '[', ':', 's', 'e', 'n', 'd', 'i', 'n', 'g', ']', '-', '>', '(', ')', ' ', 'O', 'R', ' ', '(', ')', '-', '[', ':', 's', 'e', 'n', 'd', 'i', 'n', 'g', ']', '-', '>', '(', 'a', ')', ' ', 'O', 'R', ' ', '('},
      {'a', ')', '-', '[', ':', 't', 'i', 'm', 'e', 'r', ']', '-', '>', '(', ')', ' ', 'W', 'I', 'T', 'H', ' ', 'a', ' ', 'O', 'P', 'T', 'I', 'O', 'N', 'A', 'L', ' ', 'M', 'A', 'T', 'C', 'H', ' ', '(', 'p', ':', 'P', 'a', 's', 's', 'i', 'v', 'e', ')', ' ', 'W', 'H', 'E', 'R', 'E', ' ', '(', ')', '-', '[', ':', 'p', 'u', 'b'},
      {'l', 'i', 's', 'h', 'i', 'n', 'g', ']', '-', '>', '(', 'p', ')', ' ', 'O', 'R', ' ', '(', 'p', ')', '-', '[', ':', 's', 'u', 'b', 's', 'c', 'r', 'i', 'b', 'i', 'n', 'g', ']', '-', '>', '(', ')', ' ', 'W', 'I', 'T', 'H', ' ', 'a', ',', 'p', ' ', 'O', 'P', 'T', 'I', 'O', 'N', 'A', 'L', ' ', 'M', 'A', 'T', 'C', 'H', ' '},
      {'(', 'a', ')', '-', '[', 'p', 'u', 'b', ':', 'p', 'u', 'b', 'l', 'i', 's', 'h', 'i', 'n', 'g', ']', '-', '>', '(', 'p', ')', ' ', 'W', 'I', 'T', 'H', ' ', 'a', ',', 'p', ',', 'p', 'u', 'b', ' ', 'O', 'P', 'T', 'I', 'O', 'N', 'A', 'L', ' ', 'M', 'A', 'T', 'C', 'H', ' ', '(', 'p', ')', '-', '[', 's', 'u', 'b', ':', 's'},
      {'u', 'b', 's', 'c', 'r', 'i', 'b', 'i', 'n', 'g', ']', '-', '>', '(', 'a', ')', ' ', 'W', 'I', 'T', 'H', ' ', 'a', ',', 'p', ',', 'p', 'u', 'b', ',', 's', 'u', 'b', ' ', 'O', 'P', 'T', 'I', 'O', 'N', 'A', 'L', ' ', 'M', 'A', 'T', 'C', 'H', ' ', '(', 'a', ')', '-', '[', 's', 'e', 'n', 'd', ':', 's', 'e', 'n', 'd', 'i'},
      {'n', 'g', ']', '-', '>', '(', 't', 'a', 'r', 'g', 'e', 't', ')', ' ', 'R', 'E', 'T', 'U', 'R', 'N', ' ', '{', ' ', 'a', 'c', 't', 'i', 'v', 'e', ':', ' ', 'c', 'o', 'l', 'l', 'e', 'c', 't', '(', 'D', 'I', 'S', 'T', 'I', 'N', 'C', 'T', ' ', 'a', ')', ',', ' ', 'p', 'a', 's', 's', 'i', 'v', 'e', ':', ' ', 'c', 'o', 'l'},
      {'l', 'e', 'c', 't', '(', 'D', 'I', 'S', 'T', 'I', 'N', 'C', 'T', ' ', 'p', ')', ',', ' ', 'p', 'u', 'b', ':', ' ', 'c', 'o', 'l', 'l', 'e', 'c', 't', '(', 'D', 'I', 'S', 'T', 'I', 'N', 'C', 'T', ' ', '{', ' ', 'f', 'r', 'o', 'm', ':', ' ', 's', 't', 'a', 'r', 't', 'N', 'o', 'd', 'e', '(', 'p', 'u', 'b', ')', ',', ' '},
      {'t', 'o', ':', ' ', 'e', 'n', 'd', 'N', 'o', 'd', 'e', '(', 'p', 'u', 'b', ')', ',', ' ', 'r', 'e', 'l', ':', ' ', 'p', 'u', 'b', '}', ')', ',', ' ', 's', 'u', 'b', ':', ' ', 'c', 'o', 'l', 'l', 'e', 'c', 't', '(', 'D', 'I', 'S', 'T', 'I', 'N', 'C', 'T', ' ', '{', ' ', 'f', 'r', 'o', 'm', ':', ' ', 's', 't', 'a', 'r'},
      {'t', 'N', 'o', 'd', 'e', '(', 's', 'u', 'b', ')', ',', ' ', 't', 'o', ':', ' ', 'e', 'n', 'd', 'N', 'o', 'd', 'e', '(', 's', 'u', 'b', ')', ',', ' ', 'r', 'e', 'l', ':', ' ', 's', 'u', 'b', '}', ')', ',', ' ', 's', 'e', 'n', 'd', ':', ' ', 'c', 'o', 'l', 'l', 'e', 'c', 't', '(', 'D', 'I', 'S', 'T', 'I', 'N', 'C', 'T'},
      {' ', '{', ' ', 'f', 'r', 'o', 'm', ':', ' ', 's', 't', 'a', 'r', 't', 'N', 'o', 'd', 'e', '(', 's', 'e', 'n', 'd', ')', ',', ' ', 't', 'o', ':', ' ', 'e', 'n', 'd', 'N', 'o', 'd', 'e', '(', 's', 'e', 'n', 'd', ')', ',', ' ', 'r', 'e', 'l', ':', ' ', 's', 'e', 'n', 'd', '}', ')', ' ', '}', ' ', 'A', 'S', ' ', 'r', 'e'},
      {'s', 'u', 'l', 't', '\0'},
      {'\0'}
    },
    .continuous = false
  };
  requestId_t requestId;
  mIpcClient.sendCustomMemberRequest(req, requestId);

  // receive response with shared memory location for transmission
  CustomMemberResponse resp = mIpcClient.receiveCustomMemberResponse().value();
  assert(resp.requestID == requestId);
  LOG_TRACE(LOG_VAR(resp.memAddress));

  // puzzle together query response from textual responses
  std::string queryResponseString;
  sharedMem::SHMChannel<sharedMem::Response> channel(resp.memAddress, false);
  sharedMem::Response response = MAKE_RESPONSE;
  size_t previousNumber = 0;
  do {
    channel.receive(response);
    assert(response.header.type == sharedMem::ResponseType::TEXTUAL);
    assert(previousNumber < response.textual.number);
    previousNumber = response.textual.number;
    LOG_DEBUG("Receiving package " << response.textual.number << '/' << response.textual.total);

    std::string responseLine = util::parseString(response.textual.line);
    std::move(responseLine.begin(), responseLine.end(), std::back_inserter(queryResponseString));
  } while (response.textual.number < response.textual.total);
  LOG_DEBUG("Full query response: " << queryResponseString);

  // load query response as json and navigate to data of interest
  json::json queryResponseData = json::json::parse(queryResponseString);
  queryResponseData = queryResponseData.at("results");
  queryResponseData = queryResponseData.at(0);
  queryResponseData = queryResponseData.at("data");
  queryResponseData = queryResponseData.at(0);
  queryResponseData = queryResponseData.at("row");
  queryResponseData = queryResponseData.at(0);
  assert(queryResponseData.is_object());

  // construct graph vertices
  Graph output;
  PrimaryKey primaryKey;
  for (const json::json &node: queryResponseData["active"])
  {
    primaryKey = node["primaryKey"].get<PrimaryKey>();
    LOG_TRACE("Adding Node to graph with " LOG_VAR(primaryKey));
    output.addNode(primaryKey);
  }
  for (const json::json &topic: queryResponseData["passive"])
  {
    primaryKey = topic["primaryKey"].get<PrimaryKey>();
    LOG_TRACE("Adding Topic to graph with " LOG_VAR(primaryKey));
    output.addTopic(primaryKey);
  }

  // construct graph edges
  for (const json::json &sub: queryResponseData["sub"])
  {
    if (sub["rel"].is_null())
      continue;

    std::string
      fromPrimaryKey = sub["from"]["primaryKey"],
      toPrimaryKey   = sub["to"]["primaryKey"];
    LOG_TRACE("Adding sub connection from " << fromPrimaryKey << " to " << toPrimaryKey << "to graph");
    Graph::Vertex &fromVertex = output.mVertices[fromPrimaryKey];
    assert(fromVertex.type == Graph::Vertex::TYPE_TOPIC);
    fromVertex.outgoing.push_back(toPrimaryKey);
    Graph::Vertex &toVertex = output.mVertices[toPrimaryKey];
    assert(toVertex.type == Graph::Vertex::TYPE_NODE);
    toVertex.incoming.push_back(fromPrimaryKey);
  }
  for (const json::json &pub: queryResponseData["pub"])
  {
    if (pub["rel"].is_null())
      continue;

    std::string
      fromPrimaryKey = pub["from"]["primaryKey"],
      toPrimaryKey   = pub["to"]["primaryKey"];
    LOG_TRACE("Adding pub connection from " << fromPrimaryKey << " to " << toPrimaryKey << "to graph");
    Graph::Vertex &fromVertex = output.mVertices[fromPrimaryKey];
    assert(fromVertex.type == Graph::Vertex::TYPE_NODE);
    fromVertex.outgoing.push_back(toPrimaryKey);
    Graph::Vertex &toVertex = output.mVertices[toPrimaryKey];
    assert(toVertex.type == Graph::Vertex::TYPE_TOPIC);
    toVertex.incoming.push_back(fromPrimaryKey);
  }
  for (const json::json &send: queryResponseData["send"])
  {
    if (send["rel"].is_null())
      continue;

    std::string
      fromPrimaryKey = send["from"]["primaryKey"],
      toPrimaryKey   = send["to"]["primaryKey"];
    LOG_TRACE("Adding send connection from " << fromPrimaryKey << " to " << toPrimaryKey << "to graph");
    Graph::Vertex &fromVertex = output.mVertices[fromPrimaryKey];
    assert(fromVertex.type == Graph::Vertex::TYPE_NODE);
    fromVertex.outgoing.push_back(toPrimaryKey);
    Graph::Vertex &toVertex = output.mVertices[toPrimaryKey];
    assert(toVertex.type == Graph::Vertex::TYPE_NODE);
    toVertex.incoming.push_back(fromPrimaryKey);
  }

  return output;
}

void DataStore::run(const std::atomic<bool> &running)
{
  LOG_TRACE(LOG_THIS LOG_VAR(running.load()))

  while (running.load())
  {
    std::optional<NodePublishersToUpdate> publishersToUpdate = mIpcClient.receiveNodePublishersToUpdate(false);
    if (publishersToUpdate.has_value())
    {
      NodePublishersToUpdate publishersToUpdateValue = publishersToUpdate.value();
      LOG_TRACE("Got NodePublishersToUpdate");
      mNodes.at(util::parseString(publishersToUpdateValue.primaryKey)).instance.update(publishersToUpdateValue);
    }
    std::optional<NodeSubscribersToUpdate> subscribersToUpdate = mIpcClient.receiveNodeSubscribersToUpdate(false);
    if (subscribersToUpdate.has_value())
    {
      NodeSubscribersToUpdate subscribersToUpdateValue = subscribersToUpdate.value();
      LOG_TRACE("Got NodeSubscribersToUpdate");
      mNodes.at(util::parseString(subscribersToUpdateValue.primaryKey)).instance.update(subscribersToUpdateValue);
    }
    std::optional<NodeIsServerForUpdate> isServerForUpdate = mIpcClient.receiveNodeIsServerForUpdate(false);
    if (isServerForUpdate.has_value())
    {
      NodeIsServerForUpdate isServerForUpdateValue = isServerForUpdate.value();
      LOG_TRACE("Got NodeIsServerForUpdate");
      mNodes.at(util::parseString(isServerForUpdateValue.primaryKey)).instance.update(isServerForUpdateValue);
    }
    std::optional<NodeIsClientOfUpdate> isClientOfUpdate = mIpcClient.receiveNodeIsClientOfUpdate(false);
    if (isClientOfUpdate.has_value())
    {
      NodeIsClientOfUpdate isClientOfUpdateValue = isClientOfUpdate.value();
      LOG_TRACE("Got NodeIsClientOfUpdate");
      mNodes.at(util::parseString(isClientOfUpdateValue.primaryKey)).instance.update(isClientOfUpdateValue);
    }
    std::optional<NodeIsActionServerForUpdate> isActionServerForUpdate = mIpcClient.receiveNodeIsActionServerForUpdate(false);
    if (isActionServerForUpdate.has_value())
    {
      NodeIsActionServerForUpdate isActionServerForUpdateValue = isActionServerForUpdate.value();
      LOG_TRACE("Got NodeIsActionServerForUpdate");
      mNodes.at(util::parseString(isActionServerForUpdateValue.primaryKey)).instance.update(isActionServerForUpdateValue);
    }
    std::optional<NodeIsActionClientOfUpdate> isActionClientOfUpdate = mIpcClient.receiveNodeIsActionClientOfUpdate(false);
    if (isActionClientOfUpdate.has_value())
    {
      NodeIsActionClientOfUpdate isActionClientOfUpdateValue = isActionClientOfUpdate.value();
      LOG_TRACE("Got NodeIsActionClientOfUpdate");
      mNodes.at(util::parseString(isActionClientOfUpdateValue.primaryKey)).instance.update(isActionClientOfUpdateValue);
    }
    //! NOTE: not currently regarded
    // std::optional<NodeTimerToUpdate> timerToUpdate = mIpcClient.receiveNodeTimerToUpdate(false);
    // if (timerToUpdate.has_value())
    // {
    //   NodeTimerToUpdate timerToUpdateValue = timerToUpdate.value();
    //   LOG_TRACE("Got NodeTimerToUpdate");
    //   mNodes.at(util::parseString(timerToUpdateValue.primaryKey)).instance.update(timerToUpdateValue);
    // }
    std::optional<NodeStateUpdate> stateUpdate = mIpcClient.receiveNodeStateUpdate(false);
    if (stateUpdate.has_value())
    {
      NodeStateUpdate stateUpdateValue = stateUpdate.value();
      LOG_TRACE("Got NodeStateUpdate");
      mNodes.at(util::parseString(stateUpdateValue.primaryKey)).instance.update(stateUpdateValue);
    }
    std::optional<TopicPublishersUpdate> publishersUpdate = mIpcClient.receiveTopicPublishersUpdate(false);
    if (publishersUpdate.has_value())
    {
      TopicPublishersUpdate publishersUpdateValue = publishersUpdate.value();
      LOG_TRACE("Got TopicPublishersUpdate");
      mTopics.at(util::parseString(publishersUpdateValue.primaryKey)).instance.update(publishersUpdateValue);
    }
    std::optional<TopicSubscribersUpdate> subscribersUpdate = mIpcClient.receiveTopicSubscribersUpdate(false);
    if (subscribersUpdate.has_value())
    {
      TopicSubscribersUpdate subscribersUpdateValue = subscribersUpdate.value();
      LOG_TRACE("Got TopicSubscribersUpdate");
      mTopics.at(util::parseString(subscribersUpdateValue.primaryKey)).instance.update(subscribersUpdateValue);
    }
  }
}

Node *DataStore::requestNode(const PrimaryKey &primary, bool updates)
{
  LOG_TRACE(LOG_THIS LOG_VAR(primary) LOG_VAR(updates));

  requestId_t requestId;
  NodeRequest nodeRequest{
    .updates = updates
  };
  util::parseString(nodeRequest.primaryKey, primary);
  mIpcClient.sendNodeRequest(nodeRequest, requestId);

  auto [it, emplaced] = mNodes.emplace(
    primary,
    MemberData<Node>{
      .instance = Node(mIpcClient.receiveNodeResponse().value()),
      .requestId = requestId,
      .useCounter = 1ul
    }
  );
  assert(emplaced);
  LOG_TRACE("Created node " << LOG_MEMBER((&(it->second.instance))));

  //! TODO: This should happen in a loop over a list of all available attribute types.
  //! NOTE: However, IPC doesn't provide a pipeline to get those yet, so we set the
  //!       ones we know of manually.
  const Node &node = it->second.instance;
  SingleAttributesRequest req{
    .attribute = AttributeName::CPU_UTILIZATION,
    .direction = Direction::NONE,
    .continuous = true
  };
  util::parseString(req.primaryKey, node.mPrimaryKey);
  mIpcClient.sendSingleAttributesRequest(req, requestId);
  SingleAttributesResponse response = mIpcClient.receiveSingleAttributesResponse().value();
  assert(requestId == response.requestID);

  LOG_TRACE("Added CPU utilisation attribute to " << LOG_MEMBER((&(it->second.instance))) " with shared memory location: " << response.memAddress);
  it->second.instance.addAttributeSource(std::to_string(AttributeName::CPU_UTILIZATION), response);
  return &(it->second.instance);
}

Topic *DataStore::requestTopic(const PrimaryKey &primary, bool updates)
{
  LOG_TRACE(LOG_THIS LOG_VAR(primary) LOG_VAR(updates));

  requestId_t requestId;
  TopicRequest topicRequest{
    .updates = updates
  };
  util::parseString(topicRequest.primaryKey, primary);
  mIpcClient.sendTopicRequest(topicRequest, requestId);

  auto [it, emplaced] = mTopics.emplace(
    primary,
    MemberData<Topic>{
      .instance = Topic(mIpcClient.receiveTopicResponse().value()),
      .requestId = requestId,
      .useCounter = 1ul
    }
  );
  assert(emplaced);
  LOG_TRACE("Created topic " << LOG_MEMBER((&(it->second.instance))));

  //! TODO: This should happen in a loop over a list of all available attribute types.
  //! NOTE: However, IPC doesn't provide a pipeline to get those yet, so we set the
  //!       ones we know of manually.
  const Topic &topic = it->second.instance;
  SingleAttributesResponse response;
  std::string attributeName;
  for (const Topic::Edges::value_type &edge: topic.mPublishers)
  {
    SingleAttributesRequest req{
      .attribute = AttributeName::PUBLISHINGRATES,
      .direction = Direction::NONE,
      .continuous = true
    };
    util::parseString(req.primaryKey, edge.self);
    mIpcClient.sendSingleAttributesRequest(req, requestId);
    response = mIpcClient.receiveSingleAttributesResponse().value();
    attributeName = std::to_string(AttributeName::PUBLISHINGRATES) + ": " + edge.self;

    LOG_TRACE("Added attribute " << attributeName << " to " << LOG_MEMBER((&(it->second.instance))) " with shared memory location: " << response.memAddress);
    it->second.instance.addAttributeSource(attributeName, response);
  }

  return &(it->second.instance);
}

IpcClient DataStore::tryMakeIpcClient(const json::json &config)
{
  LOG_TRACE(LOG_VAR(config));

  int projectId = config.at(CONFIG_PROJECT_ID).get<int>();
  bool retry = config.at(CONFIG_RETRY_CONNECTION).get<bool>();
  if (!retry)
    try
    {
      return IpcClient(projectId);
    }
    catch (const IpcException &except)
    {
      LOG_FATAL("Failed to create IPC: " << except.what());
      std::exit(1);
    }

  ssize_t nrRetries = config.at(CONFIG_RETRY_ATTEMPTS).get<ssize_t>() - 1;
  cr::milliseconds timeout(config.at(CONFIG_RETRY_TIMEOUT).get<size_t>());
  while (true)
  {
    try
    {
      return IpcClient(projectId);
    }
    catch (const IpcException &except)
    {
      if (--nrRetries == 0)
      {
        LOG_ERROR("Failed to create IPC: " << except.what());
        std::exit(1);
      }
      LOG_ERROR("Failed to create IPC: " << except.what() << "(" << nrRetries << " attempts left)");
    }

    std::this_thread::sleep_for(timeout);
  }
}
