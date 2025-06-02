#pragma once

#include "dynamic-subgraph/data-store.hpp"
#include "fault-detection/fault-detection.hpp"
#include "fault-trajectory-extraction/fault-trajectory-extraction.hpp"


class DynamicSubgraphBuilder
{
public:
  /**
   * initialise FD, FTE
   *
   * @param config json configuration object
   */
  DynamicSubgraphBuilder(
    void *config
  );

  void run();

private:
  void initialiseWatchlist();
  void blindSpotCheck();

  void addToSubgraph();

private:
  FaultDetection            mFD;
  FaultTrajectoryExtraction mFTE;
  // Graph                     mGraph;
  DataStore::Ptr            mpDataStore;
};
