#include "dynamic-subgraph/members.hpp"

#include "common.hpp"

#include "ipc/util.hpp"

#include <chrono>
namespace cr = std::chrono;
#include <cassert>
#include "members.hpp"


void Node::update(const NodeIsServerForUpdate &update)
{
  LOG_TRACE(this << "primaryKey: " << update.primaryKey << "srv: " << update.srvName << "nodeId: " << update.clientNodeId);

  std::string serviceName(util::parseString(update.srvName));
  ClientMapping::iterator it = mClients.find(serviceName);
  if (it == mClients.end())
  {
    bool emplaced;
    std::tie(it, emplaced) = mClients.emplace(std::move(serviceName), Clients());
    assert(emplaced);
  }

  it->second.emplace_back(update.clientNodeId, false);
}

void Node::update(const NodeIsActionServerForUpdate &update)
{
  LOG_TRACE(this << "primaryKey: " << update.primaryKey << "srv: " << update.srvName << "nodeId: " << update.actionclientNodeId);

  std::string serviceName(util::parseString(update.srvName));
  ClientMapping::iterator it = mClients.find(serviceName);
  if (it == mClients.end())
  {
    bool emplaced;
    std::tie(it, emplaced) = mClients.emplace(std::move(serviceName), Clients());
    assert(emplaced);
  }

  it->second.emplace_back(update.actionclientNodeId, false);
}

void Node::update(const NodeStateUpdate &update)
{
  LOG_TRACE(this << "primaryKey: " << update.primaryKey << "state: " << update.state << "change time: " << update.stateChangeTime);

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
  LOG_TRACE(this << LOG_VAR(mPkgName) LOG_VAR(mAlive) LOG_VAR(mBootCount) LOG_VAR(mProcessId));
}

const Node *asNode(const MemberPtr &member)
{
  assert(!member.mpMember->mIsTopic);
  return static_cast<const Node *>(member.mpMember);
}

Topic::Topic(const TopicResponse &response):
  Member(true, response.primaryKey),
  mName(util::parseString(response.name)),
  mType(util::parseString(response.type))
{
  LOG_TRACE(this << LOG_VAR(mType));
}

const Topic *asTopic(const MemberPtr &member)
{
  assert(member.mpMember->mIsTopic);
  return static_cast<const Topic *>(member.mpMember);
}