#include "fault-detection/watchlist.hpp"
#include "common.hpp"

#include <algorithm>


Watchlist::Watchlist(const json::json &config, DataStore::Ptr dataStorePtr):
  mpDataStore(dataStorePtr)
{
  if (config.is_null() ||
      (config.is_array() && config.empty()))
    return;
  config.get_to(mInitialMemberNames);
}

void Watchlist::addMember(Member::Ptr member, WatchlistMemberType type)
{
  std::scoped_lock<std::mutex> scopeLock(mMembersMutex);

  InternalMembers::const_iterator it = mMembers.find(member);
  if (it == mMembers.end())
    return;

  mMembers.insert({member, type});
}

void Watchlist::removeMember(Member::Ptr member)
{
  std::scoped_lock<std::mutex> scopeLock(mMembersMutex);

  InternalMembers::const_iterator it = mMembers.find(member);
  if (it == mMembers.end())
    return;

  removeMember(*it);
  mMembers.erase(it);
}

Members Watchlist::getMembers()
{
  std::scoped_lock<std::mutex> scopeLock(mMembersMutex);

  tryInitialise();

  Members output(mMembers.size());
  std::transform(
    mMembers.begin(), mMembers.end(),
    output.begin(),
    [](const InternalMembers::value_type &element) -> Members::value_type
    {
      return element.first;
    }
  );
  /* removal should only happen after the first detection, not the first extraction -> thus use notifyUsed
  std::remove_if(
    mMembers.begin(), mMembers.end(),
    [&](const InternalMembers::value_type &element) -> bool
    {
      if (element.second != TYPE_BLINDSPOT)
        return false;

      removeMember(element);
      return true;
    }
  );
  */

  return output;
}

void Watchlist::reset()
{
  std::scoped_lock<std::mutex> scopeLock(mMembersMutex);

  for (InternalMembers::iterator it = mMembers.begin(); it != mMembers.end();)
  {
    if (it->second != TYPE_NORMAL)
      ++it;

    removeMember(*it);
    it = mMembers.erase(it);
  }
}

void Watchlist::notifyUsed(Member::Ptr member)
{
  InternalMembers::iterator memberIt = mMembers.find(member);
  if (memberIt == mMembers.end() ||
      memberIt->second != TYPE_BLINDSPOT)
    return;

  {
    std::scoped_lock<std::mutex> scopedLock(mMembersMutex);

    removeMember(*memberIt);
    mMembers.erase(memberIt);
  }
}

void Watchlist::tryInitialise()
{
  if (mInitialMemberNames.empty())
    return;

  std::remove_if(
    mInitialMemberNames.begin(), mInitialMemberNames.end(),
    [&](const std::string &name) -> bool
    {
      const Member::Ptr node = mpDataStore->getNode(name);
      if (!node)
        return false;

      mMembers.insert(InternalMembers::value_type(node, TYPE_INITIAL));
      return true;
    }
  );
}

void Watchlist::removeMember(const InternalMembers::value_type &element)
{
  if (element.first->mIsTopic)
    mpDataStore->removeTopic(element.first->mPrimaryKey);
  else
    mpDataStore->removeNode(element.first->mPrimaryKey);
}
