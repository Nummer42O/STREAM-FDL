#include "fault-detection/fault-detection.hpp"

#include "dynamic-subgraph/members.hpp"
#include "common.hpp"

#include <algorithm>
#include <chrono>
namespace cr = std::chrono;
#include <thread>


FaultDetection::FaultDetection(const json::json &config, Watchlist *watchlist, DataStore::Ptr dataStorePtr):
  mcpWatchlist(watchlist),
  mpDataStore(dataStorePtr),
  cmMovingWindowSize(config.at(CONFIG_MOVING_WINDOW_SIZE).get<size_t>())
{
  LOG_TRACE(LOG_THIS LOG_VAR(config) LOG_VAR(watchlist) LOG_VAR(dataStorePtr));
}

void FaultDetection::run(const std::atomic<bool> &running, cr::milliseconds loopTargetInterval)
{
  LOG_TRACE(LOG_THIS LOG_VAR(running.load()));

  Timestamp start, stop;
  while (running.load())
  {
    start = cr::system_clock::now();

    // retrieve all members currently on the watchlist
    Members currentWatchlistMembers = mcpWatchlist->getMembers();
    // add their attributes to the moving window
    for (MemberPtr &member: currentWatchlistMembers)
    {
      LOG_TRACE("Updating moving attribute window for member " << member);
      Member::AttributeMapping attributes = member->getAttributes();

      MemberWindow::iterator it = mMovingWindow.find(member);
      if (it == mMovingWindow.end())
        mMovingWindow.emplace(std::move(member), createAttrWindow(attributes));
      else
        updateAttrWindow(it->second, attributes);
    }

    for (MemberWindow::iterator it = mMovingWindow.begin(); it != mMovingWindow.end();)
    {
      const auto &[memberPtr, attributeWindow] = *it;

      // if there ain't enough attribute values, skip
      //! NOTE: the first attribute is selected as representing all, since all
      //!       attribute buffers for one member theoretically grow in parallel
      if (!attributeWindow.begin()->second.full())
      {
        ++it;
        continue;
      }

      // check if there is a fault
      Alert alert;
      if (detectFaults(memberPtr, attributeWindow, alert))
      {
        LOG_DEBUG("Detected fault for member " << memberPtr);
        const ScopeLock scopedLock(mAlertMutex);
        mAlerts.push_back(std::move(alert));
      }

      // if the member for which the detection was issued is a blindspot member
      // remove it from watchlist and detection
      if (mcpWatchlist->notifyUsed(memberPtr->mPrimaryKey))
        it = mMovingWindow.erase(it);
      else
        ++it;
    }

    stop = cr::system_clock::now();
    cr::milliseconds remainingTime = loopTargetInterval - cr::duration_cast<cr::milliseconds>(stop - start);
    if (remainingTime.count() > 0)
      std::this_thread::sleep_for(remainingTime);
  }
}

FaultDetection::Alerts FaultDetection::getEmittedAlerts()
{
  LOG_TRACE(LOG_THIS);

  const ScopeLock scopeLock(mAlertMutex);

  Alerts output = mAlerts;
  //! TODO: add to DB
  mAlerts.clear();
  return output;
}

FaultDetection::AttributeWindow FaultDetection::createAttrWindow(const Member::AttributeMapping &attributeMapping) const
{
  LOG_TRACE(LOG_THIS LOG_VAR(&attributeMapping));

  AttributeWindow attrWindow;
  for (const auto &[descriptor, attributes]: attributeMapping)
  {
    CircularBuffer rawAttributes(cmMovingWindowSize);
    rawAttributes.push(attributes);
    attrWindow.emplace(
      descriptor,
      std::move(rawAttributes)
    );
  }
  return attrWindow;
}

void FaultDetection::updateAttrWindow(AttributeWindow &window, const Member::AttributeMapping &attributeMapping)
{
  LOG_TRACE(LOG_VAR(&window) LOG_VAR(&attributeMapping));

  for (const auto &[descriptor, attributes]: attributeMapping)
    window.at(descriptor).push(attributes);
}

bool FaultDetection::detectFaults(MemberPtr member, const AttributeWindow &window, Alert &oAlert)
{
  LOG_TRACE(LOG_VAR(member) LOG_VAR(&window) LOG_VAR(&oAlert));

  oAlert.timestamp = cr::system_clock::now();
  oAlert.severity = Alert::SEVERITY_NORMAL;
  if (!member->mIsTopic && !::asNode(member)->mAlive)
    return true;

  for (const auto &[descriptor, buffer]: window)
  {
    if (buffer.empty())
      continue;

    double
      mean = buffer.getMean(),
      stdDev = buffer.getStdDev(mean);
    double currentValue = *buffer.current();
    if (mean - 3 * stdDev > currentValue ||
        mean + 3 * stdDev < currentValue)
      oAlert.affectedAttributes.push_back(descriptor);
  }
  return !oAlert.affectedAttributes.empty();
}
