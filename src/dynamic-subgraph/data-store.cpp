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
#include <unistd.h>
#include <cstring>
#include <algorithm>


DataStore::DataStore(const json::json &config):
  mIpcClient(tryMakeIpcClient(config.at(CONFIG_IPC)))
{
  LOG_TRACE(LOG_THIS LOG_VAR(config));
}

const MemberPtr DataStore::getNode(const PrimaryKey &primary)
{
  LOG_TRACE(LOG_THIS LOG_VAR(primary));

  Nodes::iterator it = mNodes.find(primary);
  if (it != mNodes.end())
    return MAKE_MEMBER_PTR(it);

  return requestNode(primary, true);
}

const MemberPtr DataStore::getNodeByName(const std::string &name)
{
  LOG_TRACE(LOG_THIS LOG_VAR(name));

  Nodes::iterator it = mNodes.findName(name);
  if (it != mNodes.end())
    return MAKE_MEMBER_PTR(it);

  requestId_t searchRequestId;
  SearchRequest req{
    .type = SearchRequest::NODE
  };
  util::parseString(req.name, name);
  mIpcClient.sendSearchRequest(req, searchRequestId);

  PrimaryKey primaryKey = util::parseString(mIpcClient.receiveSearchResponse().value().primaryKey);
  LOG_TRACE("SearchRequest returned primary key: '" << primaryKey << "'");
  if (primaryKey.empty())
    return MemberPtr();
  else
    return requestNode(primaryKey, true);
}

MemberPtr DataStore::requestNode(const PrimaryKey &primary, bool updates)
{
  LOG_TRACE(LOG_THIS LOG_VAR(primary) LOG_VAR(updates));

  NodeRequest nodeRequest{
    .updates = updates
  };
  util::parseString(nodeRequest.primaryKey, primary);
  requestId_t requestId;
  Nodes::iterator node;
  {
    const ScopeLock scopedLock(mTopicsMutex);

    mIpcClient.sendNodeRequest(nodeRequest, requestId);
    NodeResponse nodeResponse = mIpcClient.receiveNodeResponse().value();
    node = mNodes.emplace_back(nodeResponse, requestId);
  }
  LOG_TRACE("Created node " << node->instance);

  SingleAttributesRequest req{
    .attribute = AttributeName::CPU_UTILIZATION,
    .direction = Direction::NONE,
    .continuous = true
  };
  util::parseString(req.primaryKey, node->instance.mPrimaryKey);
  mIpcClient.sendSingleAttributesRequest(req, requestId);
  SingleAttributesResponse response = mIpcClient.receiveSingleAttributesResponse().value();
  assert(requestId == response.requestID);

  LOG_TRACE("Added CPU utilisation attribute to " << node->instance << " with shared memory location: " << response.memAddress);
  node->instance.addAttributeSource(std::to_string(AttributeName::CPU_UTILIZATION), response);

  return MAKE_MEMBER_PTR(node);
}

const MemberPtr DataStore::getTopic(const PrimaryKey &primary)
{
  LOG_TRACE(LOG_THIS LOG_VAR(primary));

  Topics::iterator it = mTopics.find(primary);
  if (it != mTopics.end())
    return MAKE_MEMBER_PTR(it);

  return requestTopic(primary, true);
}

const MemberPtr DataStore::getTopicByName(const std::string &name)
{
  LOG_TRACE(LOG_THIS LOG_VAR(name));

  Topics::iterator it = mTopics.findName(name);
  if (it != mTopics.end())
    return MAKE_MEMBER_PTR(it);

  requestId_t searchRequestId;
  SearchRequest req{
    .type = SearchRequest::TOPIC
  };
  util::parseString(req.name, name);
  mIpcClient.sendSearchRequest(req, searchRequestId);

  PrimaryKey primaryKey = util::parseString(mIpcClient.receiveSearchResponse().value().primaryKey);
  LOG_TRACE("SearchRequest returned primary key: '" << primaryKey << "'");
  if (primaryKey.empty())
    return MemberPtr();
  else
    return requestTopic(primaryKey, true);
}

MemberPtr DataStore::requestTopic(const PrimaryKey &primary, bool updates)
{
  LOG_TRACE(LOG_THIS LOG_VAR(primary) LOG_VAR(updates));

  TopicRequest topicRequest{
    .updates = updates
  };
  util::parseString(topicRequest.primaryKey, primary);
  requestId_t requestId;
  Topics::iterator topic;
  {
    const ScopeLock scopedLock(mTopicsMutex);

    mIpcClient.sendTopicRequest(topicRequest, requestId);
    TopicResponse topicResponse = mIpcClient.receiveTopicResponse().value();
    topic = mTopics.emplace_back(topicResponse, requestId);
  }
  LOG_TRACE("Created topic " << topic->instance);

  SingleAttributesRequest req{
    .attribute = AttributeName::CPU_UTILIZATION,
    .direction = Direction::NONE,
    .continuous = true
  };
  util::parseString(req.primaryKey, topic->instance.mPrimaryKey);
  mIpcClient.sendSingleAttributesRequest(req, requestId);
  SingleAttributesResponse response = mIpcClient.receiveSingleAttributesResponse().value();
  assert(requestId == response.requestID);

  LOG_TRACE("Added CPU utilisation attribute to " << topic->instance << " with shared memory location: " << response.memAddress);
  topic->instance.addAttributeSource(std::to_string(AttributeName::CPU_UTILIZATION), response);

  return MAKE_MEMBER_PTR(topic);
}

DataStore::GraphView DataStore::getFullGraphView() const
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
    if (response.textual.number % 10 == 0)
      LOG_DEBUG("Receiving package " << response.textual.number << '/' << response.textual.total);

    std::string responseLine = util::parseString(response.textual.line);
    std::move(responseLine.begin(), responseLine.end(), std::back_inserter(queryResponseString));
  } while (response.textual.number < response.textual.total);
  LOG_TRACE("Full query response: " << queryResponseString);

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
  GraphView output;
  PrimaryKey primaryKey;
  for (const json::json &node: queryResponseData["active"])
  {
    primaryKey = node["primaryKey"].get<PrimaryKey>();
    LOG_TRACE("Adding Node to graph with " LOG_VAR(primaryKey));
    output.emplace_back(MemberProxy(primaryKey, false), MemberProxies());
  }
  for (const json::json &topic: queryResponseData["passive"])
  {
    primaryKey = topic["primaryKey"].get<PrimaryKey>();
    LOG_TRACE("Adding Topic to graph with " LOG_VAR(primaryKey));
    output.emplace_back(MemberProxy(primaryKey, true), MemberProxies());
  }

  // construct graph edges
  for (const json::json &sub: queryResponseData["sub"])
  {
    if (sub["rel"].is_null())
      continue;

    std::string
      fromPrimaryKey = sub["from"]["primaryKey"],
      toPrimaryKey   = sub["to"]["primaryKey"];
    GraphView::iterator it = std::find_if(
      output.begin(), output.end(),
      [&fromPrimaryKey](const GraphView::value_type &element) -> bool
      {
        return element.member.mPrimaryKey == fromPrimaryKey;
      }
    );
    LOG_TRACE("Adding sub connection from " << it->member << " to " << toPrimaryKey << "to graph");
    assert(it != output.end());
    it->connections.emplace_back(std::move(toPrimaryKey), false);
  }
  for (const json::json &pub: queryResponseData["pub"])
  {
    if (pub["rel"].is_null())
      continue;

    std::string
      fromPrimaryKey = pub["from"]["primaryKey"],
      toPrimaryKey   = pub["to"]["primaryKey"];
    GraphView::iterator it = std::find_if(
      output.begin(), output.end(),
      [&fromPrimaryKey](const GraphView::value_type &element) -> bool
      {
        return element.member.mPrimaryKey == fromPrimaryKey;
      }
    );
    LOG_TRACE("Adding pub connection from " << it->member << " to " << toPrimaryKey << "to graph");
    assert(it != output.end());
    it->connections.emplace_back(std::move(toPrimaryKey), true);
  }
  for (const json::json &send: queryResponseData["send"])
  {
    if (send["rel"].is_null())
      continue;

    std::string
      fromPrimaryKey = send["from"]["primaryKey"],
      toPrimaryKey   = send["to"]["primaryKey"];
    GraphView::iterator it = std::find_if(
      output.begin(), output.end(),
      [&fromPrimaryKey](const GraphView::value_type &element) -> bool
      {
        return element.member.mPrimaryKey == fromPrimaryKey;
      }
    );
    LOG_TRACE("Adding send connection from " << it->member << " to " << toPrimaryKey << "to graph");
    assert(it != output.end());
    it->connections.emplace_back(std::move(toPrimaryKey), false);
  }

  return output;
}

DataStore::SharedMemory DataStore::getCpuUtilisationMemory() const
{
  LOG_TRACE(LOG_THIS);

  static_assert(MAX_STRING_SIZE >= HOST_NAME_MAX);
  SearchRequest searchReq{
    //! NOTE: its technically neither but as a passive component its interpreted as a topic
    .type = SearchRequest::TOPIC
  };
  gethostname(searchReq.name, HOST_NAME_MAX);
  LOG_DEBUG("Host name in search request: " << searchReq.name);

  requestId_t requestId;
  mIpcClient.sendSearchRequest(searchReq, requestId);
  SearchResponse searchResp = mIpcClient.receiveSearchResponse().value();
  LOG_TRACE("Got member with primaryKey: " << searchResp.primaryKey);

  SingleAttributesRequest singleAttributeReq{
    .attribute = AttributeName::CPU_UTILIZATION,
    .direction = Direction::NONE,
    .continuous = true
  };
  //! NOTE: I am aware that memcpy is considered insecure in regards to writing on unowned memory
  //!       but as both buffers are the same size I don't see this exploding into our faces.
  std::memcpy(singleAttributeReq.primaryKey, searchResp.primaryKey, MAX_STRING_SIZE);
  mIpcClient.sendSingleAttributesRequest(singleAttributeReq, requestId);
  SingleAttributesResponse singleAttrResp = mIpcClient.receiveSingleAttributesResponse().value();
  assert(singleAttrResp.requestID == requestId);

  return SharedMemory(util::parseString(singleAttrResp.memAddress));
}

void DataStore::run(const std::atomic<bool> &running, cr::milliseconds loopTargetInterval)
{
  LOG_TRACE(LOG_THIS LOG_VAR(running.load()))

  Timestamp start, stop;
  while (running.load())
  {
    start = cr::system_clock::now();

    for (Nodes::iterator it = mNodes.begin(); it != mNodes.end();)
    {
      if (it->useCounter.nonZero())
      {
        ++it;
        continue;
      }

      LOG_TRACE("Removing " << it->instance << "from data store");
      UnsubscribeRequest req{.id = it->requestId};
      requestId_t unsubReqId;

      const ScopeLock scopedLock(mNodesMutex);
      mIpcClient.sendUnsubscribeRequest(req, unsubReqId);
      it = mNodes.erase(it);
    }
    for (Topics::iterator it = mTopics.begin(); it != mTopics.end();)
    {
      if (it->useCounter.nonZero())
      {
        ++it;
        continue;
      }

      LOG_TRACE("Removing " << it->instance << "from data store");
      UnsubscribeRequest req{.id = it->requestId};
      requestId_t unsubReqId;

      const ScopeLock scopedLock(mTopicsMutex);
      mIpcClient.sendUnsubscribeRequest(req, unsubReqId);
      it = mTopics.erase(it);
    }

    std::optional<NodePublishersToUpdate> publishersToUpdate = mIpcClient.receiveNodePublishersToUpdate(false);
    if (publishersToUpdate.has_value())
    {
      NodePublishersToUpdate publishersToUpdateValue = publishersToUpdate.value();
      PrimaryKey primaryKey = util::parseString(publishersToUpdateValue.primaryKey);
      LOG_TRACE("Got NodePublishersToUpdate for " LOG_VAR(primaryKey));

      const ScopeLock scopedLock(mNodesMutex);
      Nodes::iterator it = mNodes.find(primaryKey);
      if (it != mNodes.end())
        it->instance.update(publishersToUpdateValue);
      else
        LOG_ERROR("No node with " LOG_VAR(primaryKey) " in data store, ignoring update");
    }
    std::optional<NodeSubscribersToUpdate> subscribersToUpdate = mIpcClient.receiveNodeSubscribersToUpdate(false);
    if (subscribersToUpdate.has_value())
    {
      NodeSubscribersToUpdate subscribersToUpdateValue = subscribersToUpdate.value();
      PrimaryKey primaryKey = util::parseString(subscribersToUpdateValue.primaryKey);
      LOG_TRACE("Got NodeSubscribersToUpdate for " LOG_VAR(primaryKey));

      const ScopeLock scopedLock(mNodesMutex);
      Nodes::iterator it = mNodes.find(primaryKey);
      if (it != mNodes.end())
        it->instance.update(subscribersToUpdateValue);
      else
        LOG_ERROR("No node with " LOG_VAR(primaryKey) " in data store, ignoring update");
    }
    std::optional<NodeIsServerForUpdate> isServerForUpdate = mIpcClient.receiveNodeIsServerForUpdate(false);
    if (isServerForUpdate.has_value())
    {
      NodeIsServerForUpdate isServerForUpdateValue = isServerForUpdate.value();
      PrimaryKey primaryKey = util::parseString(isServerForUpdateValue.primaryKey);
      LOG_TRACE("Got NodeIsServerForUpdate for " LOG_VAR(primaryKey));

      const ScopeLock scopedLock(mNodesMutex);
      Nodes::iterator it = mNodes.find(primaryKey);
      if (it != mNodes.end())
        it->instance.update(isServerForUpdateValue);
      else
        LOG_ERROR("No node with " LOG_VAR(primaryKey) " in data store, ignoring update");
    }
    std::optional<NodeIsClientOfUpdate> isClientOfUpdate = mIpcClient.receiveNodeIsClientOfUpdate(false);
    if (isClientOfUpdate.has_value())
    {
      NodeIsClientOfUpdate isClientOfUpdateValue = isClientOfUpdate.value();
      PrimaryKey primaryKey = util::parseString(isClientOfUpdateValue.primaryKey);
      LOG_TRACE("Got NodeIsClientOfUpdate for " LOG_VAR(primaryKey));

      const ScopeLock scopedLock(mNodesMutex);
      Nodes::iterator it = mNodes.find(primaryKey);
      if (it != mNodes.end())
        it->instance.update(isClientOfUpdateValue);
      else
        LOG_ERROR("No node with " LOG_VAR(primaryKey) " in data store, ignoring update");
    }
    std::optional<NodeIsActionServerForUpdate> isActionServerForUpdate = mIpcClient.receiveNodeIsActionServerForUpdate(false);
    if (isActionServerForUpdate.has_value())
    {
      NodeIsActionServerForUpdate isActionServerForUpdateValue = isActionServerForUpdate.value();
      PrimaryKey primaryKey = util::parseString(isActionServerForUpdateValue.primaryKey);
      LOG_TRACE("Got NodeIsActionServerForUpdate for " LOG_VAR(primaryKey));

      const ScopeLock scopedLock(mNodesMutex);
      Nodes::iterator it = mNodes.find(primaryKey);
      if (it != mNodes.end())
        it->instance.update(isActionServerForUpdateValue);
      else
        LOG_ERROR("No node with " LOG_VAR(primaryKey) " in data store, ignoring update");
    }
    std::optional<NodeIsActionClientOfUpdate> isActionClientOfUpdate = mIpcClient.receiveNodeIsActionClientOfUpdate(false);
    if (isActionClientOfUpdate.has_value())
    {
      NodeIsActionClientOfUpdate isActionClientOfUpdateValue = isActionClientOfUpdate.value();
      PrimaryKey primaryKey = util::parseString(isActionClientOfUpdateValue.primaryKey);
      LOG_TRACE("Got NodeIsActionClientOfUpdate for " LOG_VAR(primaryKey));

      const ScopeLock scopedLock(mNodesMutex);
      Nodes::iterator it = mNodes.find(primaryKey);
      if (it != mNodes.end())
        it->instance.update(isActionClientOfUpdateValue);
      else
        LOG_ERROR("No node with " LOG_VAR(primaryKey) " in data store, ignoring update");
    }
    //! NOTE: NodeTimerToUpdate not currently regarded
    std::optional<NodeStateUpdate> stateUpdate = mIpcClient.receiveNodeStateUpdate(false);
    if (stateUpdate.has_value())
    {
      NodeStateUpdate stateUpdateValue = stateUpdate.value();
      PrimaryKey primaryKey = util::parseString(stateUpdateValue.primaryKey);
      LOG_TRACE("Got NodeStateUpdate for " LOG_VAR(primaryKey));

      const ScopeLock scopedLock(mNodesMutex);
      Nodes::iterator it = mNodes.find(primaryKey);
      if (it != mNodes.end())
        it->instance.update(stateUpdateValue);
      else
        LOG_ERROR("No node with " LOG_VAR(primaryKey) " in data store, ignoring update");
    }
    std::optional<TopicPublishersUpdate> publishersUpdate = mIpcClient.receiveTopicPublishersUpdate(false);
    if (publishersUpdate.has_value())
    {
      TopicPublishersUpdate publishersUpdateValue = publishersUpdate.value();
      PrimaryKey primaryKey = util::parseString(publishersUpdateValue.primaryKey);
      LOG_TRACE("Got TopicPublishersUpdate for " LOG_VAR(primaryKey));

      {}

      const ScopeLock scopedLock(mTopicsMutex);
      Topics::iterator it = mTopics.find(primaryKey);
      if (it != mTopics.end())
        it->instance.update(publishersUpdateValue);
      else
        LOG_ERROR("No topic with " LOG_VAR(primaryKey) " in data store, ignoring update");
    }
    std::optional<TopicSubscribersUpdate> subscribersUpdate = mIpcClient.receiveTopicSubscribersUpdate(false);
    if (subscribersUpdate.has_value())
    {
      TopicSubscribersUpdate subscribersUpdateValue = subscribersUpdate.value();
      PrimaryKey primaryKey = util::parseString(subscribersUpdateValue.primaryKey);
      LOG_TRACE("Got TopicSubscribersUpdate for " LOG_VAR(primaryKey));

      const ScopeLock scopedLock(mTopicsMutex);
      Topics::iterator it = mTopics.find(primaryKey);
      if (it != mTopics.end())
        it->instance.update(subscribersUpdateValue);
      else
        LOG_ERROR("No topic with " LOG_VAR(primaryKey) " in data store, ignoring update");
    }

    stop = cr::system_clock::now();
    cr::milliseconds remainingTime = loopTargetInterval - cr::duration_cast<cr::milliseconds>(stop - start);
    if (remainingTime.count() > 0)
      std::this_thread::sleep_for(remainingTime);
  }
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
