#include "fault-detection/watchlist.hpp"

#include "common.hpp"

#include <algorithm>
#include <thread>


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
  LOG_TRACE(LOG_THIS << member << " type: " << type);

  if (member.mIsTopic && mpDataStore->checkTopicPrimaryIgnored(member.mPrimaryKey))
  {
    LOG_DEBUG("Requested ignored member " << member << ", not adding to watchlist.");
    return;
  }

  const ScopeLock scopeLock(mMembersMutex);

  WatchlistMembers::iterator it = std::find_if(
    mMembers.begin(), mMembers.end(),
    [&member](const WatchlistMembers::value_type &internalMember) -> bool
    {
      return internalMember.member->mPrimaryKey == member.mPrimaryKey;
    }
  );
  if (it != mMembers.end())
    return;

  LOG_TRACE("Adding member " << member << " to watchlist");
  MemberPtr memberPtr = mpDataStore->get(member);
  assert(memberPtr.valid());

  mMembers.emplace_back(std::move(memberPtr), type);
}

void Watchlist::addMember(MemberPtr member, WatchlistMemberType type)
{
  LOG_TRACE(LOG_THIS << member << " type: " << type);

  const ScopeLock scopeLock(mMembersMutex);

  WatchlistMembers::iterator it = std::find_if(
    mMembers.begin(), mMembers.end(),
    [&member](const WatchlistMembers::value_type &internalMember) -> bool
    {
      return internalMember.member == member;
    }
  );
  if (it == mMembers.end())
    return;

  LOG_TRACE("Adding member " << member << " to watchlist");
  mMembers.emplace_back(std::move(member), type);
}

void Watchlist::removeMember(const PrimaryKey &member)
{
  LOG_TRACE(LOG_THIS);

  const ScopeLock scopedLock(mMembersMutex);

  WatchlistMembers::iterator it = this->get(member);
  if (it != mMembers.end())
    mMembers.erase(it);
}

Watchlist::WatchlistMembers Watchlist::getMembers()
{
  LOG_TRACE(LOG_THIS);

  const ScopeLock scopeLock(mMembersMutex);
  return mMembers;
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

void Watchlist::run(const std::atomic<bool> &running, cr::milliseconds loopTargetInterval)
{
  LOG_TRACE(LOG_THIS);

  Timestamp start, stop;
  while (running.load() && !mInitialMemberNames.empty())
  {
    start = cr::system_clock::now();

    for (auto it = mInitialMemberNames.begin(); it != mInitialMemberNames.end();)
    {
      MemberPtr node = mpDataStore->getNodeByName(*it);
      if (!node.valid())
      {
        ++it;
        continue;
      }

      const ScopeLock scopedLock(mMembersMutex);
      mMembers.emplace_back(std::move(node), TYPE_INITIAL);
      it = mInitialMemberNames.erase(it);
    }

    stop = cr::system_clock::now();
    cr::milliseconds elapsedTime = cr::duration_cast<cr::milliseconds>(stop - start);
    cr::milliseconds remainingTime = loopTargetInterval - elapsedTime;
    if (remainingTime.count() > 0)
      std::this_thread::sleep_for(remainingTime);
  }

  LOG_INFO("All initial watchlist members added to watchlist, stopping discovery.")
}

std::ostream &operator<<(std::ostream &stream, const Watchlist::WatchlistMemberType &type)
{
  switch (type)
  {
  case Watchlist::TYPE_NORMAL:
    stream << "normal";
    break;
  case Watchlist::TYPE_INITIAL:
    stream << "initial";
    break;
  case Watchlist::TYPE_BLINDSPOT:
    stream << "blind-spot";
    break;
  default:
    stream << "<unknown>";
  }

  return stream;
}
