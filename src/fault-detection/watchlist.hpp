#pragma once

#include "dynamic-subgraph/members.hpp"
#include "dynamic-subgraph/data-store.hpp"

#include "nlohmann/json.hpp"
namespace json = nlohmann;

#include <map>
#include <vector>
#include <mutex>
#include <atomic>
#include <iostream>
#include <chrono>
namespace cr = std::chrono;


class Watchlist
{
public:
  enum WatchlistMemberType: uint8_t
  {
    TYPE_NORMAL, TYPE_INITIAL, TYPE_BLINDSPOT
  };
  struct WatchlistMember
  {
    MemberPtr member;
    WatchlistMemberType type;

    bool operator<(
      const WatchlistMember &other
    ) const { return member < other.member; }
  };
  using WatchlistMembers = std::vector<WatchlistMember>;

public:
  Watchlist(
    const json::json &config,
    DataStore::Ptr dataStorePtr
  );

  void addMember(
    const MemberProxy &member,
    WatchlistMemberType type = TYPE_NORMAL
  );
  void addMember(
    MemberPtr member,
    WatchlistMemberType type = TYPE_NORMAL
  );
  void removeMember(
    const PrimaryKey &member
  );

  bool contains(
    const PrimaryKey &member
  );
  void reset();

  WatchlistMembers getMembers();

  void run(
    const std::atomic<bool> &running,
    cr::milliseconds loopTargetInterval
  );

private:
  WatchlistMembers::iterator get(
    const PrimaryKey &member
  )
  {
    return std::find_if(
      mMembers.begin(), mMembers.end(),
      [&member](const WatchlistMembers::value_type &element) -> bool
      {
        return element.member->mPrimaryKey == member;
      }
    );
  }

private:
  std::vector<std::string> mInitialMemberNames;
  WatchlistMembers mMembers;
  std::mutex mMembersMutex;
  DataStore::Ptr mpDataStore;
};
std::ostream &operator<<(std::ostream &stream, const Watchlist::WatchlistMemberType &type);
