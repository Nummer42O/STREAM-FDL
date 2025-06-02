#include "fault-detection/fault-detection.hpp"
#include "dynamic-subgraph/members.hpp"
#include "common.hpp"

#include <algorithm>


FaultDetection::FaultDetection(const YAML::Node &config):
  mWatchlist(config[CONFIG_WL_INITIAL_MEMBERS]),
  mpDataStore(DataStore::get())
{}

void FaultDetection::run()
{
  Members currentWatchlistMembers = mWatchlist.getMembers();
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
