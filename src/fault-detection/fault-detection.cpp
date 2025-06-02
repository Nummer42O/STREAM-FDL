#include "fault-detection/fault-detection.hpp"
#include "dynamic-subgraph/members.hpp"
#include "common.hpp"

#include <Eigen/Core>

#include <algorithm>

//! TODO: actual value
#define CIRCULAR_BUFFER_SIZE 10


FaultDetection::FaultDetection(const YAML::Node &config):
  mWatchlist(config[CONFIG_WL_INITIAL_MEMBERS]),
  mpDataStore(DataStore::get())
{}

void FaultDetection::run()
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
