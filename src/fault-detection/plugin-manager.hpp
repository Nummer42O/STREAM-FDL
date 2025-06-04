#pragma once

#include "stream_plugins.h"

#include "fault-detection/fault-detection.hpp"

#include <dlfcn.h>
#include <link.h>

#include <iostream>
#include <unordered_map>
#include <vector>
#include <filesystem>
namespace fs = std::filesystem;
#include <string>
#include <string_view>
#include <stdexcept>

#ifndef PLUGIN_DIRECTORY
#error "Missing PLUGIN_DIR definition."
#endif


class DynamicLibraryError: public std::runtime_error
{
public:
  DynamicLibraryError(const std::string &errorMessage) noexcept:
    std::runtime_error(errorMessage)
  {}
  DynamicLibraryError() noexcept:
    std::runtime_error(getDlError())
  {}

private:
  static const char *getDlError();
};

class PluginManager
{
public:
  enum PluginState {
    PLUGIN_UNKNOWN,
    PLUGIN_LOADED,
    PLUGIN_INITIALIZED,
    _PLUGIN_NULL_OPT
  };

private:
  struct Plugin
  {
  public:
    using initializeFunction_t = INITIALIZE_FUNCTION_SIGNATURE((*));
    using executeFunction_t = EXECUTE_FUNCTION_SIGNATURE((*));
    using deinitializeFunction_t = DEINITIALIZE_FUNCTION_SIGNATURE((*));
  public:
    void *handle = nullptr;
    bool initialized = false;

    initializeFunction_t initialize;
    executeFunction_t execute;
    deinitializeFunction_t deinitialize;
  };
  using Plugins = std::unordered_map<std::string, Plugin>;

public:
  ~PluginManager();

  void load(
    bool skipExisting = true,
    bool lazy = true
  );
  const std::vector<std::string> getLoaded() const;
  PluginState getStatus(
    std::string_view pluginName
  ) const;

  void initialize(
    bool skipInitialized = true
  );
  void initialize(
    std::string_view pluginName,
    bool skipInitialized = true
  );

  FaultState execute(
    std::string_view pluginName,
    void *data
  );

  void deinitialize(
    bool skipDeinitialized = true
  );
  void deinitialize(
    std::string_view pluginName,
    bool skipDeinitialized = true
  );

private:
  Plugin &getPlugin(
    std::string_view pluginName
  );

private:
  Plugins mLoadedPlugins;
};
