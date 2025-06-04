#pragma once

#include "nlohmann/json.hpp"
namespace json = nlohmann;

#include <string_view>
#include <chrono>
namespace cr = std::chrono;
#include <stdexcept>


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


using timestamp_t = cr::system_clock::time_point;
