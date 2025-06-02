#include "common.hpp"

#include <sstream>


ConfigurationError::ConfigurationError(std::string_view nodeName, YAML::NodeType::value expectedType, YAML::NodeType::value actualType) noexcept:
  std::runtime_error(
    (std::stringstream() << "Node '" << nodeName << "' expected type '" << getNodeTypeName(expectedType) << "' but got '" << getNodeTypeName(actualType) << "'").str()
  )
{}

const char *ConfigurationError::getNodeTypeName(YAML::NodeType::value type)
{
  switch (type)
  {
  case YAML::NodeType::Undefined:
    return "Undefined";
  case YAML::NodeType::Null:
    return "Null";
  case YAML::NodeType::Scalar:
    return "Scalar";
  case YAML::NodeType::Sequence:
    return "Sequence";
  case YAML::NodeType::Map:
    return "Map";
  default:
    return "Unknown";
  }
}
