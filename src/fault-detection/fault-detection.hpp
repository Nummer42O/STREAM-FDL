#pragma once

#include "fault-detection/watchlist.hpp"
#include "dynamic-subgraph/members.hpp"
#include "dynamic-subgraph/data-store.hpp"
#include "fault-detection/circular-buffer.hpp"
#include "common.hpp"

#include <yaml-cpp/yaml.h>

#include <vector>
#include <mutex>
#include <atomic>


struct Alert
{
  Member::Ptr member;
  Member::AttributeNameType attribute;
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
    const YAML::Node &config
  );

  void run(
    const std::atomic<bool> &running
  );

  Alerts getEmittedAlerts();

private:
  static AttributeWindow createAttrWindow(
    const Member::AttributeMapping &attributeMapping
  );
  static void updateAttrWindow(
    AttributeWindow &window,
    const Member::AttributeMapping &attributeMapping
  );
  static Alerts detectFaults(
    Member::Ptr member,
    const AttributeWindow &window
  );

private:
  Watchlist mWatchlist;
  Alerts mAlerts;
  std::mutex mAlertMutex;
  MemberWindow mMovingWindow;
  DataStore::Ptr mpDataStore;
};
