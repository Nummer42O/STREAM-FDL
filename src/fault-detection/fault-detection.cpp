#include "fault-detection/fault-detection.hpp"

#include "dynamic-subgraph/members.hpp"
#include "common.hpp"

#include <algorithm>
#include <chrono>
namespace cr = std::chrono;
#include <thread>


FaultDetection::FaultDetection(const json::json &config, Watchlist *watchlist):
  mcpWatchlist(watchlist),
  cmTimedFaultsConfig(config.at(CONFIG_TIMED_FAULTS)),
  cmMovingWindowSize(config.at(CONFIG_MOVING_WINDOW_SIZE).get<size_t>())
{
  LOG_TRACE(LOG_THIS LOG_VAR(config) LOG_VAR(watchlist));
}

void FaultDetection::run(const std::atomic<bool> &running, cr::milliseconds loopTargetInterval)
{
  LOG_TRACE(LOG_THIS LOG_VAR(running.load()));

  this->initialiseFaultMapping(cmTimedFaultsConfig);

  Timestamp start, stop;
  while (running.load())
  {
    start = cr::system_clock::now();

    // retrieve all members currently on the watchlist
    Watchlist::WatchlistMembers currentWatchlistMembers = mcpWatchlist->getMembers();
    // LOG_DEBUG("\nCurrent watchlist: " << currentWatchlistMembers);
    // add their attributes to the moving window
    for (InternalMember &wlMember: currentWatchlistMembers)
    {
      // LOG_TRACE("Updating moving attribute window for member " << wlMember.member);
      Member::AttributeMapping attributes = wlMember.member->getAttributes();

      MemberWindow::iterator it = mMovingWindow.find(wlMember);
      if (it == mMovingWindow.end())
        mMovingWindow.emplace(std::move(wlMember), this->createAttrWindow(attributes));
      else
        this->updateAttrWindow(it->second, attributes);
    }

    for (MemberWindow::iterator it = mMovingWindow.begin(); it != mMovingWindow.end();)
    {
      const auto &[wlMember, attributeWindow] = *it;
      // if there ain't enough attribute values, skip
      //! NOTE: the first attribute is selected as representing all, since all
      //!       attribute buffers for one member theoretically grow in parallel
      if (!attributeWindow.begin()->second.full())
      {
        LOG_DEBUG("Attribute window of " << wlMember.member << " has status: " << attributeWindow.begin()->second.size() << '/' << attributeWindow.begin()->second.maxSize());
        ++it;
        continue;
      }

      // check if there is a fault
      Alert alert;
      if (this->detectFaults(wlMember.member, attributeWindow, alert))
      {
        LOG_DEBUG("Detected fault for member " << wlMember.member);
        const ScopeLock scopedLock(mAlertMutex);
        mAlerts.push_back(std::move(alert));
      }

      // if the member for which the detection was issued is a blindspot member
      // remove it from watchlist and detection
      if (wlMember.type == Watchlist::TYPE_BLINDSPOT)
      {
        it = mMovingWindow.erase(it);
        std::thread removeCall(&Watchlist::removeMember, mcpWatchlist, std::cref(wlMember.member->mPrimaryKey));
        removeCall.detach();
        // mcpWatchlist->removeMember(wlMember.member->mPrimaryKey);
      }
      else
        ++it;
    }

    stop = cr::system_clock::now();
    cr::milliseconds elapsedTime = cr::duration_cast<cr::milliseconds>(stop - start);
    cr::milliseconds remainingTime = loopTargetInterval - elapsedTime;
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
  oAlert.member = member;

  //! NOTE: for evaluation purposes, this method is replaced by a rigged
  //!       system that emulated finding faults based on a configuration
  /*
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
  */

  //! NOTE: dummy payload

  Timestamp start = cr::system_clock::now();
  while (cr::duration_cast<cr::milliseconds>(cr::system_clock::now() - start).count() < 10)
  {
    std::this_thread::sleep_for(cr::microseconds(10));
  }

  const std::string &name = ::getName(member);
  FaultMapping::iterator it = mFaultMapping.find(name);
  if (it == mFaultMapping.end())
  {
    // std::cout << "Member " << member << " with name " << name << " is not planned to fail.\n";
    return false;
  }

  int64_t diff = cr::duration_cast<cr::milliseconds>(it->second - cr::system_clock::now()).count();
  // std::cout << "Member " << member << " with name " << name << " has " << diff << "ms until failure\n";
  return diff <= 0;
}

void FaultDetection::initialiseFaultMapping(const json::json &config)
{
  assert(config.is_array());

  for (const json::json &fault: config)
  {
    std::string name = fault.at(CONFIG_MEMBER_NAME).get<std::string>();
    assert(!name.empty());
    double timeoutS = fault.at(CONFIG_MEMBER_TIMEOUT).get<double>();
    assert(timeoutS > 0.0);

    auto [it, emplaced] = mFaultMapping.emplace(
      std::move(name),
      cr::system_clock::now() + cr::milliseconds(static_cast<size_t>(timeoutS * 1'000))
    );
    //! TODO: this should probably be checked normally and throw an exception
    assert(emplaced);
  }
}

std::ostream &operator<<(std::ostream &stream, const Alert &alert)
{
  stream << "Alert(member = " << alert.member << ", ...)";
  return stream;
}