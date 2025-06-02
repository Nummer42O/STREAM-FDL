#include "dynamic-subgraph/members.hpp"
#include "ipc/util.hpp"

#include <chrono>
namespace cr = std::chrono;
#include <cassert>

#define RESPONSE sharedMem::Response{.header = sharedMem::ResponseHeader(), .numerical = sharedMem::NumericalResponse()}


Member::AttributeMapping Member::getAttributes()
{
  AttributeMapping output;
  for (Attribute &attribute: mAttributes)
  {
    sharedMem::Response shmResponse = RESPONSE;
    if (attribute.sharedMemory.receive(shmResponse, false))
      assert(shmResponse.header.type == sharedMem::NUMERICAL);
      attribute.lastValue = shmResponse.numerical.value;
    output.emplace(attribute.name, attribute.lastValue);
  }
  return output;
}

void Member::addAttributeSource(int attributeName, const SingleAttributesResponse &response)
{
  SharedMemory shm(util::parseString(response.memAddress));
  sharedMem::Response shmResponse = RESPONSE;
  shm.receive(shmResponse);
  assert(shmResponse.header.type == sharedMem::NUMERICAL);

  mAttributes.push_back(Attribute{
    .name = attributeName,
    .sharedMemory = shm,
    .requestId = response.requestID,
    .lastValue = shmResponse.numerical.value
  });
}

void Node::update(const NodeIsServerForUpdate &update)
{
  std::string serviceName(util::parseString(update.srvName));
  ClientMapping::iterator it = mClients.find(serviceName);
  if (it == mClients.end())
    mClients.emplace(serviceName, Clients({update.clientNodeId}));
  else
    it->second.insert(update.clientNodeId);
}

inline void Node::update(const NodeIsActionServerForUpdate &update)
{
  std::string serviceName(util::parseString(update.srvName));
  ClientMapping::iterator it = mClients.find(serviceName);
  if (it == mClients.end())
    mClients.emplace(serviceName, Clients({update.actionclientNodeId}));
  else
    it->second.insert(update.actionclientNodeId);
}

inline void Node::update(const NodeStateUpdate &update)
{
  bool isAliveNow = (update.state == sharedMem::State::ACTIVE);
  if (isAliveNow != mAlive)
  {
    mAlive = isAliveNow;
    mAliveChangeTime = cr::system_clock::from_time_t(update.stateChangeTime);
  }
}

Node::Node(const NodeResponse &response):
  Member(false, response.primaryKey),
  mName(util::parseString(response.name)),
  mPkgName(util::parseString(response.pkgName)),
  mAlive(response.state == sharedMem::State::ACTIVE),
  mAliveChangeTime(cr::system_clock::from_time_t(response.stateChangeTime)),
  mBootCount(response.bootCount),
  mProcessId(response.pid)
{}

Topic::Topic(const TopicResponse &response):
  Member(true, response.primaryKey),
  mName(util::parseString(response.name)),
  mType(util::parseString(response.type))
{}
