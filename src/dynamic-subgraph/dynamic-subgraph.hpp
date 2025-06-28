#pragma once

#include "dynamic-subgraph/data-store.hpp"
#include "dynamic-subgraph/graph.hpp"
#include "fault-detection/fault-detection.hpp"
#include "fault-detection/watchlist.hpp"
#include "fault-detection/circular-buffer.hpp"
#include "fault-trajectory-extraction/fault-trajectory-extraction.hpp"

#include "nlohmann/json.hpp"
namespace json = nlohmann;

#include <atomic>
#include <chrono>
namespace cr = std::chrono;


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
    const json::json &config,
    DataStore::Ptr dataStorePtr,
    bool runHolistic
  );

  void run(
    std::atomic<bool> &running
  );

private:
  void runUpdateCycle(
    std::atomic<bool> &running
  );

  void blindSpotCheck();

  void extendSubgraph(
    const Alerts &newAlerts
  );

  bool checkAbortCirteria(
    const Alerts &newAlerts
  );

private:
  Watchlist                 mWatchlist;
  FaultDetection            mFD;
  FaultTrajectoryExtraction mFTE;
  Graph                     mSAG;
  DataStore::Ptr            mpDataStore;
  DataStore::SharedMemory   mCpuUtilisationSource;

  Timestamp                 mRuntimeStart;

  bool                      mSomethingIsGoingOn;
  CircularBuffer            mLastNrAlerts;
  size_t                    mBlindSpotCheckCounter;

  const cr::milliseconds    cmLoopTargetInterval;
  const size_t              cmBlindspotInterval;
  const double              cmAbortionCriteriaThreshold;
  const double              cmMaximumCpuUtilisation;
  const bool                cmRunHolistic;
};
