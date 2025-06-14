#include "fault-detection/watchlist.hpp"

#include "common.hpp"

#include <algorithm>


Watchlist::Watchlist(const json::json &config, DataStore::Ptr dataStorePtr):
  mpDataStore(dataStorePtr)
{
  LOG_TRACE(LOG_THIS LOG_VAR(config) LOG_VAR(dataStorePtr));

  if (config.is_null() ||
      (config.is_array() && config.empty()))
    return;
  config.get_to(mInitialMemberNames);
}

void Watchlist::addMember(Member::Ptr member, WatchlistMemberType type)
{
  assert(member != nullptr);
  LOG_TRACE(LOG_THIS LOG_VAR(member) "type: " << (type == TYPE_NORMAL ? "normal" : ( type == TYPE_INITIAL ? "initial" : "blindspot")));

  const std::lock_guard<std::mutex> scopeLock(mMembersMutex);

  InternalMembers::iterator it = mMembers.find(member);
  if (it == mMembers.end())
    return;

  mMembers.insert({member, type});
}

void Watchlist::removeMember(Member::Ptr member)
{
  LOG_TRACE(LOG_THIS LOG_VAR(member));

  const std::lock_guard<std::mutex> scopeLock(mMembersMutex);

  InternalMembers::iterator it = mMembers.find(member);
  if (it == mMembers.end())
    return;

  removeMember(it);
}

Members Watchlist::getMembers()
{
  LOG_TRACE(LOG_THIS);

  const std::lock_guard<std::mutex> scopeLock(mMembersMutex);

  tryInitialise();

  Members output;
  output.reserve(mMembers.size());
  std::transform(
    mMembers.begin(), mMembers.end(),
    std::back_inserter(output),
    [](const InternalMembers::value_type &element) -> Members::value_type
    {
      return element.first;
    }
  );

  return output;
}

void Watchlist::reset()
{
  LOG_TRACE(LOG_THIS);

  const std::lock_guard<std::mutex> scopeLock(mMembersMutex);

  for (InternalMembers::iterator it = mMembers.begin(); it != mMembers.end();)
  {
    if (it->second != TYPE_NORMAL)
      ++it;

    it = removeMember(it);
  }
}

void Watchlist::notifyUsed(Member::Ptr member)
{
  LOG_TRACE(LOG_THIS LOG_VAR(member));

  InternalMembers::iterator memberIt = mMembers.find(member);
  if (memberIt == mMembers.end() ||
      memberIt->second != TYPE_BLINDSPOT)
    return;

  {
    const std::lock_guard<std::mutex> scopedLock(mMembersMutex);

    removeMember(memberIt);
  }
}

void Watchlist::tryInitialise()
{
  LOG_TRACE(LOG_THIS);

  if (mInitialMemberNames.empty())
    return;

  std::remove_if(
    mInitialMemberNames.begin(), mInitialMemberNames.end(),
    [&](const std::string &name) -> bool
    {
      const Member::Ptr node = mpDataStore->getNodeByName(name);
      if (!node)
        return false;

      mMembers.emplace(node, TYPE_INITIAL);
      return true;
    }
  );
}

Watchlist::InternalMembers::iterator Watchlist::removeMember(InternalMembers::iterator elementIt)
{
  LOG_TRACE(LOG_THIS "element: " << elementIt->first << " (" << (elementIt->second == TYPE_NORMAL ? "normal" : ( elementIt->second == TYPE_INITIAL ? "initial" : "blindspot")) << ')');

  if (elementIt->first->cmIsTopic)
    mpDataStore->removeTopic(elementIt->first->mPrimaryKey);
  else
    mpDataStore->removeNode(elementIt->first->mPrimaryKey);

  return mMembers.erase(elementIt);
}
