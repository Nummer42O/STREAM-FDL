#include "fault-detection/fault-detection.hpp"
#include "dynamic-subgraph/members.hpp"
#include "common.hpp"

#include <algorithm>
#include <chrono>
namespace cr = std::chrono;
using namespace std::chrono_literals;
#include <thread>

//! TODO: actual value
//#define CIRCULAR_BUFFER_SIZE 10
//#define WAIT_TIME 100ms


//! NOTE: reenable parameter when moving from macros to config
FaultDetection::FaultDetection(const json::json &config, Watchlist *watchlist, DataStore::Ptr dataStorePtr):
  mpWatchlist(watchlist),
  mpDataStore(dataStorePtr),
  cmLoopTargetInterval(cr::duration_cast<cr::milliseconds>(1s / config.at(CONFIG_TARGET_FREQUENCY).get<double>())),
  cmMovingWindowSize(config.at(CONFIG_MOVING_WINDOW_SIZE).get<size_t>())
{}

void FaultDetection::run(const std::atomic<bool> &running)
{
  timestamp_t start, stop;
  while (running.load())
  {
    start = cr::system_clock::now();

    // retrieve all members currently on the watchlist
    Members currentWatchlistMembers = mpWatchlist->getMembers();
    // add their attributes to the moving window
    for (Member::Ptr member: currentWatchlistMembers)
    {
      Member::AttributeMapping attributes = member->getAttributes();

      MemberWindow::iterator it = mMovingWindow.find(member);
      if (it != mMovingWindow.end())
        mMovingWindow.emplace(member, createAttrWindow(attributes));
      else
        updateAttrWindow(it->second, attributes);
    }

    for (const MemberWindow::value_type &memberAttributes: mMovingWindow)
    {
      // if there ain't enough attribute values, skip
      //! NOTE: the first attribute is selected as representing all,
      //!       since all attribute buffers theoretically grow in parallel
      if (!memberAttributes.second.begin()->second.full())
        continue;
      mpWatchlist->notifyUsed(memberAttributes.first);

      Alert alert;
      if (!detectFaults(memberAttributes.first, memberAttributes.second, alert))
        continue;

      mAlertMutex.lock();
      mAlerts.push_back(alert);
      mAlertMutex.unlock();
    }

    stop = cr::system_clock::now();
    cr::milliseconds remainingTime = cmLoopTargetInterval - cr::duration_cast<cr::milliseconds>(stop - start);
    if (remainingTime > 0ms)
      std::this_thread::sleep_for(remainingTime);
  }
}

FaultDetection::Alerts FaultDetection::getEmittedAlerts()
{
  std::scoped_lock<std::mutex> scopeLock(mAlertMutex);

  Alerts output = mAlerts;
  //! TODO: add to DB
  mAlerts.clear();
  return output;
}

FaultDetection::AttributeWindow FaultDetection::createAttrWindow(const Member::AttributeMapping &attributeMapping) const
{
  AttributeWindow attrWindow;
  for (const Member::AttributeMapping::value_type &entry: attributeMapping)
  {
    CircularBuffer rawAttributes(cmMovingWindowSize);
    rawAttributes.push(entry.second);
    attrWindow.emplace(
      entry.first,
      std::move(rawAttributes)
    );
  }
  return attrWindow;
}

void FaultDetection::updateAttrWindow(AttributeWindow &window, const Member::AttributeMapping &attributeMapping)
{
  for (const Member::AttributeMapping::value_type &entry: attributeMapping)
    window.at(entry.first).push(entry.second);
}

bool FaultDetection::detectFaults(Member::Ptr member, const AttributeWindow &window, Alert &oAlert)
{
  oAlert.timestamp = cr::system_clock::now();
  oAlert.severity = Alert::SEVERITY_NORMAL;
  if (!member->mIsTopic && !static_cast<const Node *const>(member)->mAlive)
    return true;

  for (const AttributeWindow::value_type &attr: window)
  {
    if (attr.second.empty())
      continue;

    double
      mean = attr.second.getMean(),
      stdDev = attr.second.getStdDev(mean);
    double currentValue = *attr.second.current();
    if (mean - 3 * stdDev > currentValue ||
        mean + 3 * stdDev < currentValue)
      oAlert.affectedAttributes.push_back(attr.first);
  }
  return !oAlert.affectedAttributes.empty();
}
