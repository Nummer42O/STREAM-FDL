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

  std::thread(&Watchlist::addMemberInteral, this, member, type).detach();
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
      LOG_TRACE("Trying to add " << *it << " to watchlist.");

      WatchlistMembers::iterator memberIt;
      {
        const ScopeLock scopeLock(mMembersMutex);
        memberIt = this->get(*it);
      }
      if (memberIt != mMembers.end())
      {
        if (memberIt->type == TYPE_BLINDSPOT)
          memberIt->type = TYPE_INITIAL;
        else
          LOG_DEBUG(*it << " already in watchlist, skipping.");
        it = mInitialMemberNames.erase(it);
      }

      MemberPtr node = mpDataStore->getNodeByName(*it);
      if (!node.valid())
      {
        ++it;
        continue;
      }

      {
        const ScopeLock scopedLock(mMembersMutex);
        mMembers.emplace_back(std::move(node), TYPE_INITIAL);
      }
      it = mInitialMemberNames.erase(it);
    }

    stop = cr::system_clock::now();
    cr::milliseconds elapsedTime = cr::duration_cast<cr::milliseconds>(stop - start);
    cr::milliseconds remainingTime = loopTargetInterval - elapsedTime;
    if (remainingTime.count() > 0)
      std::this_thread::sleep_for(remainingTime);
  }

  LOG_INFO("Exiting watchlist loop.")
}

void Watchlist::addMemberInteral(MemberProxy member, WatchlistMemberType type)
{
  LOG_TRACE(LOG_THIS LOG_VAR(member) LOG_VAR(type));

  Timestamp start = cr::system_clock::now();
  {
    const ScopeLock scopeLock(mMembersMutex);
    auto it = this->get(member.mPrimaryKey);
    if (it != mMembers.end())
    {
      if (it->type == TYPE_BLINDSPOT)
        it->type = type;
      LOG_DEBUG(member << " already in watchlist, updated type.");
      return;
    }
  }
  std::cout << "checking existence took: " << cr::duration_cast<cr::milliseconds>(cr::system_clock::now() - start).count() << "ms\n";
  LOG_TRACE("Adding " << member << " to watchlist.");

  start = cr::system_clock::now();
  MemberPtr node = mpDataStore->get(member);
  std::cout << "getting member took: " << cr::duration_cast<cr::milliseconds>(cr::system_clock::now() - start).count() << "ms\n";

  start = cr::system_clock::now();
  {
    const ScopeLock scopedLock(mMembersMutex);
    mMembers.emplace_back(std::move(node), type);
  }
  std::cout << "emplacing member took: " << cr::duration_cast<cr::milliseconds>(cr::system_clock::now() - start).count() << "ms\n";
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

std::ostream &operator<<(std::ostream &stream, const Watchlist::WatchlistMember &member)
{
  stream << "WLM(" << member.member << ", " << member.type << ')';
  return stream;
}
