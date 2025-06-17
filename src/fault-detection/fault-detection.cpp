#include "fault-detection/fault-detection.hpp"

#include "dynamic-subgraph/members.hpp"
#include "common.hpp"

#include <algorithm>
#include <chrono>
namespace cr = std::chrono;
using namespace std::chrono_literals;
#include <thread>


//! NOTE: reenable parameter when moving from macros to config
FaultDetection::FaultDetection(const json::json &config, Watchlist *watchlist, DataStore::Ptr dataStorePtr):
  mpWatchlist(watchlist),
  mpDataStore(dataStorePtr),
  cmLoopTargetInterval(cr::duration_cast<cr::milliseconds>(1s / config.at(CONFIG_TARGET_FREQUENCY).get<double>())),
  cmMovingWindowSize(config.at(CONFIG_MOVING_WINDOW_SIZE).get<size_t>())
{
  LOG_TRACE(LOG_THIS LOG_VAR(config) LOG_VAR(watchlist) LOG_VAR(dataStorePtr));
}

void FaultDetection::run(const std::atomic<bool> &running)
{
  LOG_TRACE(LOG_THIS LOG_VAR(running.load()));

  Timestamp start, stop;
  while (running.load())
  {
    start = cr::system_clock::now();

    // retrieve all members currently on the watchlist
    Members currentWatchlistMembers = mpWatchlist->getMembers();
    // add their attributes to the moving window
    for (Member::Ptr member: currentWatchlistMembers)
    {
      LOG_TRACE("Updating moving attribute window for member " << LOG_MEMBER(member));
      Member::AttributeMapping attributes = member->getAttributes();

      MemberWindow::iterator it = mMovingWindow.find(member);
      if (it == mMovingWindow.end())
        mMovingWindow.emplace(member, createAttrWindow(attributes));
      else
        updateAttrWindow(it->second, attributes);
    }

    for (const auto &[memberPtr, attributeWindow]: mMovingWindow)
    {
      // if there ain't enough attribute values, skip
      //! NOTE: the first attribute is selected as representing all, since all
      //!       attribute buffers for one member theoretically grow in parallel
      if (!attributeWindow.begin()->second.full())
        continue;
      mpWatchlist->notifyUsed(memberPtr);

      Alert alert;
      if (!detectFaults(memberPtr, attributeWindow, alert))
        continue;

      LOG_DEBUG("Detected fault for member " << LOG_MEMBER(memberPtr));
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

bool FaultDetection::detectFaults(Member::Ptr member, const AttributeWindow &window, Alert &oAlert)
{
  LOG_TRACE(LOG_VAR(member) LOG_VAR(&window) LOG_VAR(&oAlert));

  oAlert.timestamp = cr::system_clock::now();
  oAlert.severity = Alert::SEVERITY_NORMAL;
  if (!member->cmIsTopic && !static_cast<Node *>(member)->mAlive)
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
