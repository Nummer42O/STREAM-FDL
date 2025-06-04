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


struct Alert
{
  Member::Ptr member;
  std::vector<Member::AttributeNameType> affectedAttributes;
  timestamp_t timestamp;
  enum Severity {
    SEVERITY_NORMAL //! TODO
  } severity;
};


class FaultDetection
{
friend class DynamicSubgraphBuilder;

public:
  using Alerts = std::vector<Alert>;

private:
  using AttributeWindow = std::map<Member::AttributeNameType, CircularBuffer>;
  using MemberWindow = std::map<Member::Ptr, AttributeWindow>;

public:
  /**
   * initialise watchlist
   *
   * @param config json subconfiguration object
   */
  FaultDetection(
    const json::json &config,
    Watchlist *watchlist,
    DataStore::Ptr dataStorePtr
  );

  void run(
    const std::atomic<bool> &running
  );

  Alerts getEmittedAlerts();

private:
  AttributeWindow createAttrWindow(
    const Member::AttributeMapping &attributeMapping
  ) const;
  static void updateAttrWindow(
    AttributeWindow &window,
    const Member::AttributeMapping &attributeMapping
  );
  static bool detectFaults(
    Member::Ptr member,
    const AttributeWindow &window,
    Alert &oAlert
  );

private:
  Watchlist *const mpWatchlist;
  Alerts mAlerts;
  std::mutex mAlertMutex;
  MemberWindow mMovingWindow;
  DataStore::Ptr mpDataStore;

  const cr::milliseconds cmLoopTargetInterval;
  const size_t cmMovingWindowSize;
};
