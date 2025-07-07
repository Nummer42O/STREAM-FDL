#include "dynamic-subgraph/dynamic-subgraph.hpp"

#include "common.hpp"

#include <thread>
using namespace std::chrono_literals;
#include <iostream>
#include <fstream>


DynamicSubgraphBuilder::DynamicSubgraphBuilder(const json::json &config, DataStore::Ptr dataStorePtr, bool runHolistic):
  mWatchlist(config.at(CONFIG_WATCHLIST), dataStorePtr),
  mFD(config, &mWatchlist),
  mpDataStore(dataStorePtr),
  mRestartCounter(0ul),
  mSomethingIsGoingOn(false),
  mLastNrAlerts(config.at(CONFIG_ALERT_RATE).at(CONFIG_NR_NORMALISATION_VALUES).get<size_t>()),
  mBlindSpotCheckCounter(0ul),
  cmLoopTargetInterval(cr::duration_cast<cr::milliseconds>(1s / config.at(CONFIG_TARGET_FREQUENCY).get<double>())),
  cmBlindspotInterval(config.at(CONFIG_BLINDSPOT_INTERVAL).get<size_t>()),
  cmAbortionCriteriaThreshold(config.at(CONFIG_ALERT_RATE).at(CONFIG_ABORTION_CRITERIA_THRESHOLD).get<double>()),
  cmMaximumCpuUtilisation(config.at(CONFIG_BLINDSPOT_CPU_THRESHOLD).get<double>()),
  cmRunHolistic(runHolistic)
{
  LOG_TRACE(LOG_THIS LOG_VAR(config) LOG_VAR(dataStorePtr));

  mCpuUtilisationSource = mpDataStore->getCpuUtilisationMemory();
}

void DynamicSubgraphBuilder::run(const std::atomic<bool> &running)
{
  LOG_TRACE(LOG_THIS LOG_VAR(running.load()));
  // mRuntimeStart = cr::system_clock::now();

  std::thread updates(&DynamicSubgraphBuilder::runUpdateCycle, this, std::cref(running));
  std::thread dataStore(&DataStore::run, mpDataStore, std::cref(running), cmLoopTargetInterval);
  // std::thread visualisation(&Graph::visualise, &mSAG, std::cref(running), cmLoopTargetInterval);

  if (cmRunHolistic)
  {
    MemberProxies proxies = mpDataStore->getAllMembers();
    for (const MemberProxy &proxy: proxies)
      mWatchlist.addMemberSync(proxy, Watchlist::TYPE_INITIAL);
  }
  // std::cout << cr::duration_cast<cr::milliseconds>(cr::system_clock::now() - mRuntimeStart).count() << '\n';
  mRuntimeStart = cr::system_clock::now();
  std::cout << "Start: " << (1000 * cr::duration_cast<cr::seconds>(cr::system_clock::now().time_since_epoch()).count()) << '\n';

  std::thread watchlist(&Watchlist::run, &mWatchlist, std::cref(running), cmLoopTargetInterval);
  std::thread faultDetection(&FaultDetection::run, &mFD, std::cref(running), cmLoopTargetInterval);

  Timestamp start, stop;
  while (running.load())
  {
    start = cr::system_clock::now();

    if (!cmRunHolistic)
    {
      sharedMem::Response resp = MAKE_RESPONSE;
      mCpuUtilisationSource.receive(resp);

      //! NOTE: as the shared memory does not overwrite unread memory we need to make sure its "empty"
      while (mCpuUtilisationSource.receive(resp, false)) {}

      assert(resp.header.type == sharedMem::ResponseType::NUMERICAL);
      assert(resp.numerical.number == 1ul && resp.numerical.total == 1ul);
      double cpuUtilisation = resp.numerical.value;

      LOG_DEBUG(LOG_VAR(mBlindSpotCheckCounter) LOG_VAR(cpuUtilisation));
      if (mBlindSpotCheckCounter == 0ul && cpuUtilisation < cmMaximumCpuUtilisation)
        this->blindSpotCheck();
      mBlindSpotCheckCounter = (mBlindSpotCheckCounter + 1) % cmBlindspotInterval;
    }

    DataStore::GraphView updates = mpDataStore->getUpdates();
    LOG_INFO("Got " << updates.size() << " updates.");
    if (!updates.empty())
    {
      for (const DataStore::MemberConnections &update: updates)
      {
        if (!mSAG.contains(update.member))
          continue;
        for (const MemberProxy &member: update.connections)
        {
          if (!mWatchlist.contains(member.mPrimaryKey))
            mWatchlist.addMemberAsync(member);
        }
      }
    }

    //! NOTE: trying to lock the mutex will halt the loop while it is already externally locked for synchronisation
    mMainloopMutex.lock();
    mMainloopMutex.unlock();

    stop = cr::system_clock::now();
    cr::milliseconds elapsedTime = cr::duration_cast<cr::milliseconds>(stop - start);
    cr::milliseconds remainingTime = cmLoopTargetInterval - elapsedTime;
    if (remainingTime.count() > 0)
      std::this_thread::sleep_for(remainingTime);
  }
  LOG_INFO("Dynamic Subgraph Builder mainloop terminated.");

  updates.join();
  watchlist.join();
  faultDetection.join();
  dataStore.join();
  // visualisation.join();
}

void DynamicSubgraphBuilder::runUpdateCycle(const std::atomic<bool> &running)
{
  Timestamp start, stop;
  while (running.load())
  {
    start = cr::system_clock::now();

    Alerts emittedAlerts = mFD.getEmittedAlerts();
    LOG_DEBUG("Emitted alerts: " << emittedAlerts);
    if (this->checkAbortCirteria(emittedAlerts))
    {
      LOG_INFO("Abortion criteria reached, starting fault trajectory extraction.");

      std::cout << "Runtime:   " << cr::duration_cast<cr::milliseconds>(cr::system_clock::now() - mRuntimeStart).count() << "ms\n";
      std::cout << "Timestamp break: " << (1000 * cr::duration_cast<cr::seconds>(cr::system_clock::now().time_since_epoch()).count()) << '\n';

      const ScopeLock mainScopedLock(mMainloopMutex);
      const ScopeLock fdScopedLock(mFD.mMainloopMutex);
      const ScopeLock wlScopedLock(mWatchlist.mMainloopMutex);

      mSomethingIsGoingOn = false;
      //mFTE.doSomething();

      while (mWatchlist.mPendingAdditions)
        std::this_thread::sleep_for(10ms);

      if (++mRestartCounter > 10)
        std::exit(0);

      mWatchlist.reset();
      mFD.reset();
      mSAG.reset();

      mpDataStore->removeDangling();
      mRuntimeStart = cr::system_clock::now();

      std::cout << "Timestamp reset: " << (1000 * cr::duration_cast<cr::seconds>(cr::system_clock::now().time_since_epoch()).count()) << '\n';

      // get the last alerts that might have sprung up before the reset
      // Alerts lastAlerts = mFD.getEmittedAlerts();
      // std::move(lastAlerts.begin(), lastAlerts.end(), std::back_inserter(emittedAlerts));
    }
    else if (!emittedAlerts.empty())
      //! NOTE: updating the graph after the break condition check as updating before would cause the new alert rate to stay at 0 permanently
      this->extendSubgraph(emittedAlerts);

    //! TODO: std::move emittedAlerts into FTE-Alert-DB

    stop = cr::system_clock::now();
    cr::milliseconds elapsedTime = cr::duration_cast<cr::milliseconds>(stop - start);
    cr::milliseconds remainingTime = cmLoopTargetInterval - elapsedTime;
    if (remainingTime.count() > 0)
      std::this_thread::sleep_for(remainingTime);
  }
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
      (std::all_of(
        currentMember.connections.begin(), currentMember.connections.end(),
        [&currentPath](const MemberProxy &proxy) -> bool
        {
          MemberProxies::iterator endIt = currentPath.end();
          return std::find(currentPath.begin(), endIt, proxy) != endIt;
        }
      )))
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

  std::cout << "Timestamp blindspot: " << (1000 * cr::duration_cast<cr::seconds>(cr::system_clock::now().time_since_epoch()).count()) << '\n';

  ScopeTimer timer("get blindspots");
  DataStore::GraphView fullGraph = mpDataStore->getFullGraphView();
  // timer.print("get full graph", true);
  MemberProxies potentialBlindSpots = ::getBlindspots(std::move(fullGraph));
  // timer.print("calc blindspots", true);
  LOG_DEBUG(LOG_VAR(potentialBlindSpots));

  for (const MemberProxy &member: potentialBlindSpots)
    mWatchlist.addMemberAsync(member, Watchlist::TYPE_BLINDSPOT);
}

void DynamicSubgraphBuilder::extendSubgraph(const Alerts &newAlerts)
{
  LOG_TRACE(LOG_THIS);

  for (const Alert &alert: newAlerts)
  {
    if (mSAG.add(alert.member))
    {
      MemberProxies incoming = mSAG.getIncoming(alert.member);
      LOG_DEBUG("Incoming members for " << alert.member << ": " << incoming);
      for (const MemberProxy &incomingMember: incoming)
        mWatchlist.addMemberAsync(incomingMember);
    }
    else
      LOG_DEBUG("Graph already contains " << alert.member);
  }
  LOG_DEBUG(LOG_VAR(mSAG));
  mSAG.updateVisualisation();
}

bool DynamicSubgraphBuilder::checkAbortCirteria(const Alerts &newAlerts)
{
  LOG_TRACE(LOG_THIS);
  const ScopeTimer timer("breakpoint");

  size_t nrNewAlerts = 0ul;
  for (const Alert &alert: newAlerts)
  {
    if (!mSAG.contains(MemberProxy(alert.member->mPrimaryKey, alert.member->mIsTopic)))
      ++nrNewAlerts;
  }

  if (!mSomethingIsGoingOn && nrNewAlerts > 0)
  {
    mSomethingIsGoingOn = true;
    mLastNrAlerts.reset();
  }
  mLastNrAlerts.push(nrNewAlerts);

  // get average ammount of new alerts
  double meanNewAlerts = mLastNrAlerts.getMean();
  LOG_DEBUG(LOG_VAR(nrNewAlerts) LOG_VAR(meanNewAlerts) LOG_VAR(mSomethingIsGoingOn));

  if (mFD.mFaultMapping.size() != mSAG.size())
    return false;

  std::cout << "Timestamp sag size: " << (1000 * cr::duration_cast<cr::seconds>(cr::system_clock::now().time_since_epoch()).count()) << '\n';

  for (const auto &entry: mFD.mFaultMapping)
  {
    if (!mSAG.contains(entry.first))
      return false;
  }
  return true;

  // return (mSomethingIsGoingOn && meanNewAlerts <= cmAbortionCriteriaThreshold);
}
