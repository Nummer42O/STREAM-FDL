#pragma once

#include "fault-detection/watchlist.hpp"
#include "dynamic-subgraph/members.hpp"
#include "dynamic-subgraph/data-store.hpp"
#include "fault-detection/circular-buffer.hpp"
#include "common.hpp"

#include <yaml-cpp/yaml.h>

#include <vector>
#include <mutex>


struct Alert
{
  Member::Ptr member;
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

public:
  /**
   * initialise watchlist
   *
   * @param config json subconfiguration object
   */
  FaultDetection(
    const YAML::Node &config
  );

  void run();

  Alerts getEmittedAlerts();

private:
  Watchlist mWatchlist;
  std::mutex mWatchlistMutex;
  Alerts mAlerts;
  std::mutex mAlertMutex;
  MemberWindow mMovingWindow;
  DataStore::Ptr mpDataStore;
};
