#pragma once

#include "dynamic-subgraph/members.hpp"
#include "dynamic-subgraph/data-store.hpp"

#include "nlohmann/json.hpp"
namespace json = nlohmann;

#include <map>
#include <vector>
#include <mutex>
#include <iostream>


class Watchlist
{
public:
  enum WatchlistMemberType: uint8_t
  {
    TYPE_NORMAL, TYPE_INITIAL, TYPE_BLINDSPOT
  };

private:
  using InternalMembers = std::map<MemberPtr, WatchlistMemberType>;

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

  bool contains(
    const PrimaryKey &member
  );
  void reset();

  Members getMembers();
  bool notifyUsed(
    const PrimaryKey &member
  );

  friend std::ostream &operator<<(
    std::ostream &stream,
    const Watchlist &watchlist
  );

private:
  void tryInitialise();

  InternalMembers::iterator get(
    const PrimaryKey &member
  )
  {
    return std::find_if(
      mMembers.begin(), mMembers.end(),
      [&member](const InternalMembers::value_type &element) -> bool
      {
        return element.first->mPrimaryKey == member;
      }
    );
  }

private:
  std::vector<std::string> mInitialMemberNames;
  InternalMembers mMembers;
  std::mutex mMembersMutex;
  DataStore::Ptr mpDataStore;
};
std::ostream &operator<<(std::ostream &stream, const Watchlist::WatchlistMemberType &type);
