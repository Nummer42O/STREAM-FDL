#include "common.hpp"

#include <sstream>


ConfigurationError::ConfigurationError(std::string_view nodeName, json::json::value_t expectedType, json::json::value_t actualType) noexcept:
  std::runtime_error(
    (std::stringstream() << "Node '" << nodeName << "' expected type '" << getNodeTypeName(expectedType) << "' but got '" << getNodeTypeName(actualType) << "'").str()
  )
{}

const char *ConfigurationError::getNodeTypeName(json::json::value_t type)
{
  switch (type)
  {
  case json::json::value_t::null:
    return "null value";
  case json::json::value_t::object:
    return "object (unordered set of name/value pairs)";
  case json::json::value_t::array:
    return "array (ordered collection of values)";
  case json::json::value_t::string:
    return "string value";
  case json::json::value_t::boolean:
    return "boolean value";
  case json::json::value_t::number_integer:
    return "number value (signed integer)";
  case json::json::value_t::number_unsigned:
    return "number value (unsigned integer)";
  case json::json::value_t::number_float:
    return "number value (floating-point)";
  case json::json::value_t::binary:
    return "binary array (ordered collection of bytes)";
  default:
    return "unknown value";
  }
}
