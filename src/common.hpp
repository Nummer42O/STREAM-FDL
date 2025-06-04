#pragma once

#include <yaml-cpp/node/type.h>

#include <string_view>
#include <chrono>
namespace cr = std::chrono;
#include <stdexcept>


#define CONFIG_WL_INITIAL_MEMBERS "initial-members"
#define CONFIG_WATCHLIST          "watchlist-config"

#define MAKE_RESPONSE sharedMem::Response{.header = sharedMem::ResponseHeader(), .numerical = sharedMem::NumericalResponse()}


using timestamp_t = cr::sys_time<cr::system_clock::duration>;

class ConfigurationError: public std::runtime_error
{
public:
  ConfigurationError(
    std::string_view nodeName,
    YAML::NodeType::value expectedType,
    YAML::NodeType::value actualType
  ) noexcept;

private:
  static const char *getNodeTypeName(YAML::NodeType::value type);
};
