#include "fault-detection/fault-detection.hpp"
#include "dynamic-subgraph/members.hpp"
#include "common.hpp"

#include <algorithm>
#include <chrono>
namespace cr = std::chrono;
using namespace std::chrono_literals;
#include <thread>

//! TODO: actual value
#define CIRCULAR_BUFFER_SIZE 10
#define WAIT_TIME 100ms


FaultDetection::FaultDetection(const YAML::Node &config):
  mWatchlist(config[CONFIG_WL_INITIAL_MEMBERS]),
  mpDataStore(DataStore::get())
{}

void FaultDetection::run(const std::atomic<bool> &running)
{
  while (running.load())
  {
    // retrieve all members currently on the watchlist
    Members currentWatchlistMembers = mWatchlist.getMembers();
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

    for (const MemberWindow::value_type &members: mMovingWindow)
    {
      Alerts alerts = detectFaults(members.first, members.second);
      if (alerts.empty())
        continue;

      mAlertMutex.lock();
      mAlerts.reserve(mAlerts.size() + alerts.size());
      std::move(alerts.begin(), alerts.end(), std::back_inserter(mAlerts));
      mAlertMutex.unlock();
    }

    std::this_thread::sleep_for(WAIT_TIME);
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

FaultDetection::AttributeWindow FaultDetection::createAttrWindow(const Member::AttributeMapping &attributeMapping)
{
  AttributeWindow attrWindow;
  for (const Member::AttributeMapping::value_type &entry: attributeMapping)
  {
    CircularBuffer rawAttributes(CIRCULAR_BUFFER_SIZE);
    rawAttributes.push_back(entry.second);
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
    window.at(entry.first).push_back(entry.second);
}

FaultDetection::Alerts FaultDetection::detectFaults(Member::Ptr member, const AttributeWindow &window)
{
  std::vector<Alert> output;
  for (const AttributeWindow::value_type &attr: window)
  {
    auto [mean, stdDev] = attr.second.getMetrics();
    double currentValue = *attr.second.current();
    if (mean - 3 * stdDev > currentValue ||
        mean + 3 * stdDev < currentValue)
      output.push_back(Alert{
        .member = member,
        .attribute = attr.first,
        .timestamp = cr::system_clock::now(),
        .severity = Alert::SEVERITY_NORMAL
      });
  }
  return output;
}
