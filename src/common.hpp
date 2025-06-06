#pragma once

#include <chrono>
namespace cr = std::chrono;
#include <ctime>
#include <string>
#include <iostream>
#include <iomanip>
#include <source_location>
#include <type_traits>


#define CONFIG_PROJECT_ID                       "project-id"
#define CONFIG_ALERT_RATE                       "alert-rate"
#define   CONFIG_NR_NORMALISATION_VALUES        "nr-normalisation-values"
#define   CONFIG_ABORTION_CRITERIA_THRESHOLD    "abortion-criteria-threshold"
#define CONFIG_BLINDSPOT_INTERVAL               "blindspot-interval"
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
#define _LOG_TIMESTAMP '[' << std::put_time(std::localtime(&timestamp), "%x %X") << "]"
#else
#define _LOG_GET_TIME
#define _LOG_TIMESTAMP
#endif // defined(FDL_LOG_TIMESTAMP)

#define _LOG(severity, msg) \
  { \
    const std::source_location source = std::source_location::current(); \
    _LOG_GET_TIME \
    std::clog << std::boolalpha \
      << _LOG_TIMESTAMP severity " " \
      << source.function_name() << " (" << source.file_name() << ":" << source.line() << "): " \
      << msg << '\n'; \
  }

#define LOG_THIS "(this = " << this << ") "
#define LOG_VAR(var) #var ": " << var << " "

#if FDL_LOG_LEVEL <= _LOG_LEVEL_TRACE
#define LOG_TRACE(msg)  _LOG("[TRACE]", msg)
#else
#define LOG_TRACE(msg)
#endif // FDL_LOG_LEVEL <= _LOG_LEVEL_TRACE
#if FDL_LOG_LEVEL <= _LOG_LEVEL_DEBUG
#define LOG_DEBUG(msg)  _LOG("[DEBUG]", msg)
#else
#define LOG_DEBUG(msg)
#endif // FDL_LOG_LEVEL <= _LOG_LEVEL_DEBUG
#if FDL_LOG_LEVEL <= _LOG_LEVEL_INFO
#define LOG_INFO(msg)   _LOG("[INFO] ", msg)
#else
#define LOG_INFO(msg)
#endif // FDL_LOG_LEVEL <= _LOG_LEVEL_INFO
#if FDL_LOG_LEVEL <= _LOG_LEVEL_WARN
#define LOG_WARN(msg)   _LOG("[WARN] ", msg)
#else
#define LOG_WARN(msg)
#endif // FDL_LOG_LEVEL <= _LOG_LEVEL_WARN
#if FDL_LOG_LEVEL <= _LOG_LEVEL_ERROR
#define LOG_ERROR(msg)  _LOG("[ERROR]", msg)
#else
#define LOG_ERROR(msg)
#endif // FDL_LOG_LEVEL <= _LOG_LEVEL_ERROR
#if FDL_LOG_LEVEL <= _LOG_LEVEL_FATAL
#define LOG_FATAL(msg)  _LOG("[FATAL]", msg)
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
