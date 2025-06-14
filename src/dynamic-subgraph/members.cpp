#include "dynamic-subgraph/members.hpp"

#include "common.hpp"

#include "ipc/util.hpp"

#include <chrono>
namespace cr = std::chrono;
#include <cassert>


Member::AttributeMapping Member::getAttributes()
{
  LOG_TRACE(LOG_MEMBER(this));

  AttributeMapping output;
  for (Attribute &attribute: mAttributes)
  {
    sharedMem::Response shmResponse = MAKE_RESPONSE;
    if (attribute.sharedMemory.receive(shmResponse, false))
    {
      assert(shmResponse.header.type == sharedMem::NUMERICAL);
      LOG_TRACE(LOG_MEMBER(this) " Attribute " << attribute.name << " got new value " << shmResponse.numerical.value);
      attribute.lastValue = shmResponse.numerical.value;
    }
    output.emplace(attribute.name, attribute.lastValue);
  }
  return output;
}

void Member::addAttributeSource(const AttributeDescriptor &attributeName, const SingleAttributesResponse &response)
{
  LOG_TRACE(LOG_MEMBER(this) LOG_VAR(attributeName) "requestId: " << response.requestID << "memAddress: " << response.memAddress);

  SharedMemory shm(util::parseString(response.memAddress));
  sharedMem::Response shmResponse = MAKE_RESPONSE;
  shm.receive(shmResponse);
  assert(shmResponse.header.type == sharedMem::NUMERICAL);
  LOG_TRACE(LOG_MEMBER(this) "Initial attribute " << attributeName << " value: " << shmResponse.numerical.value);

  // Attribute attr{
  //   .name = attributeName,
  //   .sharedMemory = shm,
  //   .requestId = response.requestID,
  //   .lastValue = shmResponse.numerical.value
  // };
  mAttributes.emplace_back(attributeName, std::move(shm), response.requestID, shmResponse.numerical.value);
}

void Node::update(const NodeIsServerForUpdate &update)
{
  LOG_TRACE(LOG_MEMBER(this) "primaryKey: " << update.primaryKey << "srv: " << update.srvName << "nodeId: " << update.clientNodeId);

  std::string serviceName(util::parseString(update.srvName));
  ClientMapping::iterator it = mClients.find(serviceName);
  if (it == mClients.end())
    mClients.emplace(serviceName, Clients({update.clientNodeId}));
  else
    it->second.push_back(update.clientNodeId);
}

void Node::update(const NodeIsActionServerForUpdate &update)
{
  LOG_TRACE(LOG_MEMBER(this) "primaryKey: " << update.primaryKey << "srv: " << update.srvName << "nodeId: " << update.actionclientNodeId);

  std::string serviceName(util::parseString(update.srvName));
  ClientMapping::iterator it = mClients.find(serviceName);
  if (it == mClients.end())
    mClients.emplace(serviceName, Clients({update.actionclientNodeId}));
  else
    it->second.push_back(update.actionclientNodeId);
}

void Node::update(const NodeStateUpdate &update)
{
  LOG_TRACE(LOG_MEMBER(this) "primaryKey: " << update.primaryKey << "state: " << update.state << "change time: " << update.stateChangeTime);

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
{
  LOG_TRACE(LOG_MEMBER(this) LOG_VAR(mPkgName) LOG_VAR(mAlive) LOG_VAR(mBootCount) LOG_VAR(mProcessId));
}

Topic::Topic(const TopicResponse &response):
  Member(true, response.primaryKey),
  mName(util::parseString(response.name)),
  mType(util::parseString(response.type))
{
  LOG_TRACE(LOG_MEMBER(this) LOG_VAR(mType));
}
