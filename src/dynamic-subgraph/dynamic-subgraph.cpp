#include "dynamic-subgraph/dynamic-subgraph.hpp"

#include "common.hpp"

#include <thread>
using namespace std::chrono_literals;


DynamicSubgraphBuilder::DynamicSubgraphBuilder(const json::json &config, DataStore::Ptr dataStorePtr):
  mWatchlist(config.at(CONFIG_WATCHLIST), dataStorePtr),
  mFD(config, &mWatchlist, dataStorePtr),
  mpDataStore(dataStorePtr),
  mSomethingIsGoingOn(false),
  mLastNrAlerts(config.at(CONFIG_ALERT_RATE).at(CONFIG_NR_NORMALISATION_VALUES).get<size_t>()),
  mBlindSpotCheckCounter(0ul),
  cmLoopTargetInterval(cr::duration_cast<cr::milliseconds>(1s / config.at(CONFIG_TARGET_FREQUENCY).get<double>())),
  cmBlindspotInterval(config.at(CONFIG_BLINDSPOT_INTERVAL).get<size_t>()),
  cmAbortionCriteriaThreshold(config.at(CONFIG_ALERT_RATE).at(CONFIG_ABORTION_CRITERIA_THRESHOLD).get<double>()),
  cmMaximumCpuUtilisation(config.at(CONFIG_BLINDSPOT_CPU_THRESHOLD).get<double>())
{
  LOG_TRACE(LOG_THIS LOG_VAR(config) LOG_VAR(dataStorePtr));

  mCpuUtilisationSource = mpDataStore->getCpuUtilisationMemory();
}

void DynamicSubgraphBuilder::run(const std::atomic<bool> &running)
{
  LOG_TRACE(LOG_THIS LOG_VAR(running.load()));

  std::thread faultDetection(&FaultDetection::run, &mFD, std::cref(running), cmLoopTargetInterval);
  std::thread dataStore(&DataStore::run, mpDataStore, std::cref(running), cmLoopTargetInterval);
  std::thread visualisation(&Graph::visualise, &mSAG, std::cref(running), cmLoopTargetInterval);

  Timestamp start, stop;
  while (running.load())
  {
    start = cr::system_clock::now();

    sharedMem::Response resp = MAKE_RESPONSE;
    mCpuUtilisationSource.receive(resp);
    assert(resp.header.type == sharedMem::ResponseType::NUMERICAL);
    assert(resp.numerical.number == 1ul && resp.numerical.total == 1ul);
    double cpuUtilisation = resp.numerical.value;

    LOG_DEBUG(LOG_VAR(mBlindSpotCheckCounter) LOG_VAR(cpuUtilisation));
    if (mBlindSpotCheckCounter == 0ul && cpuUtilisation < cmMaximumCpuUtilisation)
      blindSpotCheck();
    mBlindSpotCheckCounter = (mBlindSpotCheckCounter + 1) % cmBlindspotInterval;

    Alerts emittedAlerts = mFD.getEmittedAlerts();
    LOG_DEBUG("Got " << emittedAlerts.size() << " alerts.");
    if (!emittedAlerts.empty())
      expandSubgraph(emittedAlerts);
    if (checkAbortCirteria(emittedAlerts))
    {
      LOG_INFO("Abortion criteria reached, starting fault trajectory extraction.");
      mSomethingIsGoingOn = false;
      //mFTE.doSomething();
      mWatchlist.reset();
      mFD.reset();
      mSAG.reset();

      // get the last alerts that might have sprung up before the reset
      Alerts lastAlerts = mFD.getEmittedAlerts();
      std::move(lastAlerts.begin(), lastAlerts.end(), std::back_inserter(emittedAlerts));
    }

    //! TODO: std::move emittedAlerts into FTE-Alert-DB

    stop = cr::system_clock::now();
    cr::milliseconds remainingTime = cmLoopTargetInterval - cr::duration_cast<cr::milliseconds>(stop - start);
    if (remainingTime.count() > 0)
      std::this_thread::sleep_for(remainingTime);
  }
  LOG_INFO("Dynamic Subgraph Builder mainloop terminated.");

  faultDetection.join();
  dataStore.join();
  visualisation.join();
}

static void getBlindspotsInternal(
  DataStore::GraphView &&graph,
  DataStore::MemberConnections &&currentMember,
  MemberProxies currentPath,
  MemberProxies &oBlindSpots
)
{
  LOG_TRACE(LOG_VAR(&graph) << currentMember.member << LOG_VAR(currentPath) LOG_VAR(oBlindSpots));

  // get outgoing edges (and their count) for currently observed vertex
  size_t nrOutgoingEdges = currentMember.connections.size();

  // if there are no more outgoing edges or the only outgoing edge is already in the path,
  // i.e. the path loops, the current node should be monitored as a potential blind spot
  // otherwise continue traversing the path
  if (nrOutgoingEdges == 0ul ||
      (nrOutgoingEdges == 1ul &&
       std::find(currentPath.begin(), currentPath.end(), currentMember.connections.front()) != currentPath.end()))
  {
    LOG_TRACE(currentMember.member << " is blindspot")
    oBlindSpots.push_back(currentMember.member);
    return;
  }
  else
    currentPath.push_back(currentMember.member);

  // if there are outgoing edges, investigate all vertices they lead to
  for (const MemberProxy &outgoingEdgeTo: currentMember.connections)
  {
    // if the vertex has been visited already, ignore it
    DataStore::GraphView::iterator it = std::find_if(
      graph.begin(), graph.end(),
      [&outgoingEdgeTo](const DataStore::GraphView::value_type &member) -> bool
      {
        return member.member == outgoingEdgeTo;
      }
    );
    if (it == graph.end())
      continue;

    // if the vertex has not been visited, mark it as such and investigate it
    DataStore::MemberConnections vertex = std::move(*it);
    graph.erase(it);
    getBlindspotsInternal(std::move(graph), std::move(vertex), currentPath, oBlindSpots);
  }
}

static MemberProxies getBlindspots(
  DataStore::GraphView &&graph
)
{
  LOG_TRACE(LOG_VAR(&graph));

  MemberProxies blindspots;
  // visited vertices get removed, so initiate searches until there are no more unchecked vertices
  while (!graph.empty())
  {
    DataStore::MemberConnections vertex = std::move(graph.back());
    graph.pop_back();
    LOG_TRACE("Starting new trace path from " << vertex.member);
    getBlindspotsInternal(std::move(graph), std::move(vertex), {}, blindspots);
  }

  return blindspots;
}

void DynamicSubgraphBuilder::blindSpotCheck()
{
  LOG_TRACE(LOG_THIS);

  DataStore::GraphView fullGraph = mpDataStore->getFullGraphView();
  MemberProxies potentialBlindSpots = ::getBlindspots(std::move(fullGraph));
  // LOG_DEBUG(LOG_VAR(potentialBlindSpots));

  for (const MemberProxy &member: potentialBlindSpots)
    mWatchlist.addMember(member, Watchlist::TYPE_BLINDSPOT);
}

void DynamicSubgraphBuilder::expandSubgraph(const Alerts &newAlerts)
{
  LOG_TRACE(LOG_THIS);

  for (const Alert &alert: newAlerts)
  {
    if (mSAG.add(alert.member))
      for (const MemberProxy &incomingMember: mSAG.getIncoming(alert.member))
        mWatchlist.addMember(incomingMember);
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
