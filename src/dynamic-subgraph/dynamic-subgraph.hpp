#pragma once

#include "dynamic-subgraph/data-store.hpp"
#include "dynamic-subgraph/graph.hpp"
#include "fault-detection/fault-detection.hpp"
#include "fault-detection/watchlist.hpp"
#include "fault-detection/circular-buffer.hpp"
#include "fault-trajectory-extraction/fault-trajectory-extraction.hpp"

#include <yaml-cpp/yaml.h>

#include <atomic>


class DynamicSubgraphBuilder
{
public:
  using Alerts = FaultDetection::Alerts;

public:
  /**
   * initialise FD, FTE
   *
   * @param config json configuration object
   */
  DynamicSubgraphBuilder(
    const YAML::Node &config
  );

  void run(
    const std::atomic<bool> &running
  );

private:
  void blindSpotCheck();

  void expandSubgraph(
    const Alerts &newAlerts
  );

  bool checkAbortCirteria(
    const Alerts &newAlerts
  );

private:
  FaultDetection            mFD;
  FaultTrajectoryExtraction mFTE;
  Watchlist                 mWatchlist;
  Graph                     mSAG;
  DataStore::Ptr            mpDataStore;

  bool                      mSomethingIsGoingOn;
  CircularBuffer            mLastNrAlerts;
  size_t                    mBlindSpotCheckCounter;
};
