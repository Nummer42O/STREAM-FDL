#pragma once

#include "fault-detection/watchlist.hpp"
#include "dynamic-subgraph/members.hpp"
#include "dynamic-subgraph/data-store.hpp"
#include "fault-detection/circular-buffer.hpp"
#include "common.hpp"

#include "nlohmann/json.hpp"
namespace json = nlohmann;

#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>
namespace cr = std::chrono;
#include <iostream>
#include <condition_variable>


struct Alert
{
  MemberPtr member;
  std::vector<Member::AttributeDescriptor> affectedAttributes;
  Timestamp timestamp;
  enum Severity {
    SEVERITY_NORMAL //! TODO
  } severity = SEVERITY_NORMAL;
};
std::ostream &operator<<(
  std::ostream &stream,
  const Alert &alert
);


class FaultDetection
{
public:
  using Alerts = std::vector<Alert>;
  using FaultMapping = std::map<std::string /*name*/, Timestamp /*fault start time*/>;

private:
  using InternalMember = Watchlist::WatchlistMember;
  using AttributeWindow = std::map<Member::AttributeDescriptor, CircularBuffer>;
  using MemberWindow = std::map<InternalMember, AttributeWindow>;

public:
  FaultDetection(
    const json::json &config,
    Watchlist *const watchlist
  );

  void run(
    const std::atomic<bool> &running,
    cr::milliseconds loopTargetInterval
  );

  Alerts getEmittedAlerts();

  void reset() { mMovingWindow.clear(); }

private:
  AttributeWindow createAttrWindow(
    const Member::AttributeMapping &attributeMapping
  ) const;

  void updateAttrWindow(
    AttributeWindow &window,
    const Member::AttributeMapping &attributeMapping
  );

  bool detectFaults(
    MemberPtr member,
    const AttributeWindow &window,
    Alert &oAlert
  );

  void initialiseFaultMapping(
    const json::json &config
  );

public:
  FaultMapping mFaultMapping;

private:
  Alerts mAlerts;
  std::mutex mAlertMutex;
  std::condition_variable mHasNewAlerts;
  MemberWindow mMovingWindow;

  Watchlist *const mcpWatchlist;

  const json::json cmTimedFaultsConfig;
  const size_t cmMovingWindowSize;
};
