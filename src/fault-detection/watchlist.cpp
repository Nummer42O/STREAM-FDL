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

  //! TODO: when this also accepts topics eventually, we should probably clean from ignored topics
}

void Watchlist::addMember(const MemberProxy &member, WatchlistMemberType type)
{
  LOG_TRACE(LOG_THIS << member << " type: " << (type == TYPE_NORMAL ? "normal" : ( type == TYPE_INITIAL ? "initial" : "blindspot")));

  if (member.mIsTopic && mpDataStore->checkTopicPrimaryIgnored(member.mPrimaryKey))
  {
    LOG_DEBUG("Requested ignored member " << member << ", not adding to watchlist.");
    return;
  }

  const ScopeLock scopeLock(mMembersMutex);

  InternalMembers::iterator it = std::find_if(
    mMembers.begin(), mMembers.end(),
    [&member](const InternalMembers::value_type &internalMember) -> bool
    {
      return internalMember.first->mPrimaryKey == member.mPrimaryKey;
    }
  );
  if (it == mMembers.end())
    return;

  LOG_TRACE("Adding member " << member << " to watchlist");
  MemberPtr memberPtr = mpDataStore->get(member);

  bool emplaced;
  std::tie(it, emplaced) = mMembers.emplace(std::move(memberPtr), type);
  assert(emplaced);
}

void Watchlist::addMember(MemberPtr member, WatchlistMemberType type)
{
  LOG_TRACE(LOG_THIS << member << " type: " << (type == TYPE_NORMAL ? "normal" : ( type == TYPE_INITIAL ? "initial" : "blindspot")));

  const ScopeLock scopeLock(mMembersMutex);

  InternalMembers::iterator it = std::find_if(
    mMembers.begin(), mMembers.end(),
    [&member](const InternalMembers::value_type &internalMember) -> bool
    {
      return internalMember.first == member;
    }
  );
  if (it == mMembers.end())
    return;

  LOG_TRACE("Adding member " << member << " to watchlist");
  bool emplaced;
  std::tie(it, emplaced) = mMembers.emplace(std::move(member), type);
  assert(emplaced);
}

Members Watchlist::getMembers()
{
  LOG_TRACE(LOG_THIS);

  const ScopeLock scopeLock(mMembersMutex);

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

bool Watchlist::contains(const PrimaryKey &member)
{
  LOG_TRACE(LOG_THIS);
  const ScopeLock scopeLock(mMembersMutex);

  return this->get(member) != mMembers.end();
}

void Watchlist::reset()
{
  LOG_TRACE(LOG_THIS);
  const ScopeLock scopeLock(mMembersMutex);

  mMembers.clear();
}

bool Watchlist::notifyUsed(const PrimaryKey &member)
{
  LOG_TRACE(LOG_THIS LOG_VAR(member));
  const ScopeLock scopedLock(mMembersMutex);

  InternalMembers::iterator it = this->get(member);
  if (it == mMembers.end() ||
      it->second != TYPE_BLINDSPOT)
    return false;

  mMembers.erase(it);
  return true;
}

void Watchlist::tryInitialise()
{
  LOG_TRACE(LOG_THIS);

  if (mInitialMemberNames.empty())
    return;

  for (auto it = mInitialMemberNames.begin(); it != mInitialMemberNames.end();)
  {
    MemberPtr node = mpDataStore->getNodeByName(*it);
    if (!node.valid())
    {
      ++it;
      continue;
    }

    mMembers.emplace(std::move(node), TYPE_INITIAL);
    it = mInitialMemberNames.erase(it);
  }
}
