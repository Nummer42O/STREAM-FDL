#include "dynamic-subgraph/dynamic-subgraph.hpp"

#include "common.hpp"

#include <thread>

//! TODO: actual values
//#define ALERT_RATE_NORMALISATION_WINDOW_SIZE 10ul
//#define BLINDSPOT_CHECK_ITERATION_INTERVAL 15ul
//#define ABORT_CRITERIA_ALERT_RATE_THRESHOLD 0.05


DynamicSubgraphBuilder::DynamicSubgraphBuilder(const json::json &config, DataStore::Ptr dataStorePtr):
  mWatchlist(config.at(CONFIG_WATCHLIST), dataStorePtr),
  mFD(config.at(CONFIG_FAULT_DETECTION), &mWatchlist, dataStorePtr),
  mpDataStore(dataStorePtr),
  mSomethingIsGoingOn(false),
  mLastNrAlerts(config.at(CONFIG_ALERT_RATE).at(CONFIG_NR_NORMALISATION_VALUES).get<size_t>()),
  mBlindSpotCheckCounter(0ul),
  cmBlindspotInterval(config.at(CONFIG_BLINDSPOT_INTERVAL).get<size_t>()),
  cmAbortionCriteriaThreshold(config.at(CONFIG_ALERT_RATE).at(CONFIG_ABORTION_CRITERIA_THRESHOLD).get<double>())
{
  LOG_TRACE(LOG_THIS LOG_VAR(config) LOG_VAR(dataStorePtr));
}

void DynamicSubgraphBuilder::run(const std::atomic<bool> &running)
{
  LOG_TRACE(LOG_THIS LOG_VAR(running.load()));

  std::thread faultDetection(&FaultDetection::run, &mFD, std::cref(running));
  std::thread dataStore(&DataStore::run, mpDataStore, std::cref(running));
  std::thread visualisation(&Graph::visualise, &mSAG, std::cref(running));

  while (running.load())
  {
    LOG_DEBUG(LOG_VAR(mBlindSpotCheckCounter));
    if (mBlindSpotCheckCounter == 0ul)
      blindSpotCheck();
    mBlindSpotCheckCounter = (mBlindSpotCheckCounter + 1) % cmBlindspotInterval;

    Alerts emittedAlerts = mFD.getEmittedAlerts();
    LOG_DEBUG("Got " << emittedAlerts.size() << " alerts.");
    expandSubgraph(emittedAlerts);
    if (checkAbortCirteria(emittedAlerts))
    {
      LOG_INFO("Abortion criteria reached, starting fault trajectory extraction.");
      mSomethingIsGoingOn = false;
      //mFTE.doSomething();
      mWatchlist.reset();
      mSAG.reset();

      Alerts lastDitchAlerts = mFD.getEmittedAlerts();
      std::move(lastDitchAlerts.begin(), lastDitchAlerts.end(), std::back_inserter(emittedAlerts));
    }

    //! TODO: std::move emittedAlerts into FTE-Alert-DB
  }
  LOG_INFO("Dynamic Subgraph Builder mainloop terminated.");

  faultDetection.join();
  dataStore.join();
  visualisation.join();
}

void DynamicSubgraphBuilder::blindSpotCheck()
{
  LOG_TRACE(LOG_THIS);

  Graph fullGraph = mpDataStore->getFullGraphView();
  MemberIds potentialBlindSpots = ::getBlindspots(fullGraph);
  LOG_DEBUG(LOG_VAR(potentialBlindSpots));
  Members watchlistMembers = mWatchlist.getMembers();

  for (const PrimaryKey &member: potentialBlindSpots)
  {
    Members::const_iterator it = std::find_if(
      watchlistMembers.begin(), watchlistMembers.end(),
      [member](const Members::value_type &element)
      {
        return element->mPrimaryKey == member;
      }
    );
    if (it != watchlistMembers.end())
      continue;

    LOG_TRACE("Adding blindspot member " LOG_VAR(member) " to watchlist");
    if (fullGraph.get(member)->type == Graph::Vertex::TYPE_NODE)
      mWatchlist.addMember(mpDataStore->getNode(member), Watchlist::TYPE_BLINDSPOT);
    else
      mWatchlist.addMember(mpDataStore->getTopic(member), Watchlist::TYPE_BLINDSPOT);
  }
}

void DynamicSubgraphBuilder::expandSubgraph(const Alerts &newAlerts)
{
  LOG_TRACE(LOG_THIS);

  //! TODO: This is not quite as the thesis describes…
  //!       I think I need to write new Topics/Nodes to a seperate buffer and evaluate it here.
  for (const Alert &alert: newAlerts)
  {
    auto vertex = mSAG.add(alert.member);
    if (vertex)
    {
      for (const PrimaryKey &primary: vertex->incoming)
        mWatchlist.addMember(
          vertex->type == Graph::Vertex::TYPE_NODE ?
          mpDataStore->getNode(primary) :
          mpDataStore->getTopic(primary)
        );
    }
  }
  mSAG.updateVisualisation();
}

bool DynamicSubgraphBuilder::checkAbortCirteria(const Alerts &newAlerts)
{
  LOG_TRACE(LOG_THIS);

  size_t nrNewAlerts = newAlerts.size();
  mLastNrAlerts.push(nrNewAlerts);

  // get average ammount of new alerts
  double meanNewAlerts = mLastNrAlerts.getMean();
  LOG_DEBUG(LOG_VAR(nrNewAlerts) LOG_VAR(meanNewAlerts) LOG_VAR(mSomethingIsGoingOn));

  // if we currently are not investigating any failure…
  if (!mSomethingIsGoingOn)
  {
    // …check whether the mean exceeds the threshold, i.e. we start building the subgraph
    if (meanNewAlerts > cmAbortionCriteriaThreshold)
      mSomethingIsGoingOn = true;
    // regardless of the above, in this state we definetly won't abort as there is nothing to abort yet
    return false;
  }
  // if we are investigating a failure, return wether the mean has dropped below the threshold
  return (meanNewAlerts <= cmAbortionCriteriaThreshold);
}
