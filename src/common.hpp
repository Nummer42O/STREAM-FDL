#pragma once

#define BOOST_STACKTRACE_LINK
#include <boost/stacktrace.hpp>
#include <boost/stacktrace/frame.hpp>
namespace st = boost::stacktrace;

#include <chrono>
namespace cr = std::chrono;
#include <ctime>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <source_location>
#include <filesystem>
namespace fs = std::filesystem;
#include <thread>


#define CONFIG_IPC                              "ipc"
#define   CONFIG_PROJECT_ID                     "project-id"
#define   CONFIG_RETRY_CONNECTION               "retry-connection"
#define   CONFIG_RETRY_ATTEMPTS                 "retry-attempts"
#define   CONFIG_RETRY_TIMEOUT                  "retry-timeout-ms"
#define CONFIG_ALERT_RATE                       "alert-rate"
#define   CONFIG_NR_NORMALISATION_VALUES        "nr-normalisation-values"
#define   CONFIG_ABORTION_CRITERIA_THRESHOLD    "abortion-criteria-threshold"
#define CONFIG_BLINDSPOT_INTERVAL               "blindspot-interval"
#define CONFIG_BLINDSPOT_CPU_THRESHOLD          "blindspot-cpu-threshold"
#define CONFIG_WATCHLIST                        "initial-watchlist-members"
#define CONFIG_FAULT_DETECTION                  "fault-detection"
#define   CONFIG_MOVING_WINDOW_SIZE             "moving-window-size"
#define   CONFIG_TARGET_FREQUENCY               "target-frequency"


#define MAKE_RESPONSE sharedMem::Response{.header = sharedMem::ResponseHeader(), .numerical = sharedMem::NumericalResponse()}


#define _LOG_LEVEL_TRACE  0
#define _LOG_LEVEL_DEBUG  1
#define _LOG_LEVEL_INFO   2
#define _LOG_LEVEL_WARN   3
#define _LOG_LEVEL_ERROR  4
#define _LOG_LEVEL_FATAL  5

#ifdef FDL_LOG_TIMESTAMP
#define _LOG_GET_TIME std::time_t timestamp = cr::system_clock::to_time_t(cr::system_clock::now());
#define _LOG_TIMESTAMP "[" << std::put_time(std::localtime(&timestamp), "%x %X") << "]"
#else
#define _LOG_GET_TIME
#define _LOG_TIMESTAMP
#endif // defined(FDL_LOG_TIMESTAMP)

#ifdef FDL_LOG_MINIMAL
/*
#define _LOG(severity, color, msg) \
  { \
    const std::source_location source = std::source_location::current(); \
    fs::path sourceFile = source.file_name(), shortSourceFile = sourceFile.filename(), parent; \
    do { \
      sourceFile = sourceFile.parent_path(); \
      parent = sourceFile.filename(); \
      shortSourceFile = parent / shortSourceFile; \
    } while (!parent.empty() && parent != "src"); \
    std::stringstream logMsg; \
    logMsg << std::boolalpha \
      << color severity "(" << shortSourceFile.string() << ':' << source.line() << ") " << source.function_name() << ":\033[0m " \
      << msg << '\n'; \
    std::clog << logMsg.str(); \
  }
*/
#define _LOG(severity, color, msg) \
  { \
    st::stacktrace trace; \
    st::stacktrace::reverse_iterator \
      rit = trace.rbegin(), \
      endRit = trace.rend(); \
    while (rit != endRit && rit->source_file().find("/home/ubuntu/stream/STREAM-FDL") == std::string::npos) \
      ++rit; \
    std::stringstream logMsg; \
    logMsg << std::boolalpha << color severity "(" << rit->name(); \
    while (++rit != endRit) \
      if (rit->source_file().find("/home/ubuntu/stream/STREAM-FDL") != std::string::npos) \
        logMsg << " -> " << rit->name(); \
    logMsg << "):\033[0m " << msg << "\n\n"; \
    std::clog << logMsg.str(); \
  }
#else
#define _LOG(severity, color, msg) \
  { \
    const std::source_location source = std::source_location::current(); \
    _LOG_GET_TIME \
    std::stringstream logMsg; \
    logMsg << std::boolalpha \
      << color _LOG_TIMESTAMP severity " " \
      << source.function_name() << " (" << source.file_name() << ":" << source.line() << "):\033[0m " \
      << msg << '\n'; \
    std::clog << logMsg.str(); \
  }
#endif //!defined(FDL_LOG_MINIMAL)

#define LOG_THIS "(this = " << this << ") "
#define LOG_VAR(var) #var ": " << var << " "
#define LOG_MEMBER(memberPtr) (memberPtr->cmIsTopic ? "Topic" : "Node") << "('" << memberPtr->mPrimaryKey << "'@" << memberPtr << ")"

#if FDL_LOG_LEVEL <= _LOG_LEVEL_TRACE
#define LOG_TRACE(msg)  _LOG("[TRACE]", "\033[0;36m", msg)
#else
#define LOG_TRACE(msg)
#endif // FDL_LOG_LEVEL <= _LOG_LEVEL_TRACE
#if FDL_LOG_LEVEL <= _LOG_LEVEL_DEBUG
#define LOG_DEBUG(msg)  _LOG("[DEBUG]", "\033[0;94m", msg)
#else
#define LOG_DEBUG(msg)
#endif // FDL_LOG_LEVEL <= _LOG_LEVEL_DEBUG
#if FDL_LOG_LEVEL <= _LOG_LEVEL_INFO
#define LOG_INFO(msg)   _LOG("[INFO] ", "\033[0;92m", msg)
#else
#define LOG_INFO(msg)
#endif // FDL_LOG_LEVEL <= _LOG_LEVEL_INFO
#if FDL_LOG_LEVEL <= _LOG_LEVEL_WARN
#define LOG_WARN(msg)   _LOG("[WARN] ", "\033[0;93m", msg)
#else
#define LOG_WARN(msg)
#endif // FDL_LOG_LEVEL <= _LOG_LEVEL_WARN
#if FDL_LOG_LEVEL <= _LOG_LEVEL_ERROR
#define LOG_ERROR(msg)  _LOG("[ERROR]", "\033[0;91m", msg)
#else
#define LOG_ERROR(msg)
#endif // FDL_LOG_LEVEL <= _LOG_LEVEL_ERROR
#if FDL_LOG_LEVEL <= _LOG_LEVEL_FATAL
#define LOG_FATAL(msg)  _LOG("[FATAL]", "\033[0;95m", msg)
#else
#define LOG_FATAL(msg)
#endif // FDL_LOG_LEVEL <= _LOG_LEVEL_FATAL


using Timestamp = cr::system_clock::time_point;
using PrimaryKey = std::string;

template<typename T>
std::ostream &operator<<(std::ostream &stream, const std::vector<T> &v)
{
  stream << '{';

  if (!v.empty())
  {
    auto
      it    = v.begin(),
      endIt = v.end();
    stream << *it;
    for (++it; it != endIt; ++it)
      stream << ", " << *it;
  }

  stream << '}';
  return stream;
}

class ScopeLock
{
public:
  explicit ScopeLock(std::mutex &mutex):
    mrMutex(mutex)
  {
    LOG_INFO(LOG_THIS " Attemting to lock " LOG_VAR(&mrMutex));
    mrMutex.lock();
    LOG_INFO(LOG_THIS " Locked mutex " LOG_VAR(&mrMutex));
  }

  ScopeLock(std::mutex &mutex, std::adopt_lock_t) noexcept:
    mrMutex(mutex)
  {
    LOG_INFO(LOG_THIS " Attemting to lock " LOG_VAR(&mrMutex));
    mrMutex.lock();
    LOG_INFO(LOG_THIS " Locked mutex " LOG_VAR(&mrMutex));
  }

  ~ScopeLock()
  {
    mrMutex.unlock();
    LOG_INFO(LOG_THIS " Unlocked mutex");
  }

  ScopeLock(const ScopeLock&) = delete;
  ScopeLock& operator=(const ScopeLock&) = delete;

private:
  std::mutex &mrMutex;
};
