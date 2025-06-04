#pragma once

#include "dynamic-subgraph/members.hpp"
#include "dynamic-subgraph/data-store.hpp"

#include "nlohmann/json.hpp"
namespace json = nlohmann;

#include <map>
#include <vector>
#include <mutex>


class Watchlist
{
public:
  enum WatchlistMemberType: uint8_t
  {
    TYPE_NORMAL, TYPE_INITIAL, TYPE_BLINDSPOT
  };

private:
  using InternalMembers = std::map<Member::Ptr, WatchlistMemberType>;

public:
  Watchlist(
    const json::json &config,
    DataStore::Ptr dataStorePtr
  );

  void addMember(Member::Ptr member, WatchlistMemberType type = TYPE_NORMAL);
  void removeMember(Member::Ptr member);
  Members getMembers();
  void reset();
  void notifyUsed(Member::Ptr member);

private:
  void tryInitialise();
  void removeMember(const InternalMembers::value_type &element);

private:
  std::vector<std::string> mInitialMemberNames;
  InternalMembers mMembers;
  std::mutex mMembersMutex;
  DataStore::Ptr mpDataStore;
};
