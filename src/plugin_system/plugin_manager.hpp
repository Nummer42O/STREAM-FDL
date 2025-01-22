#pragma once

#include "stream_plugins.h"

#include <dlfcn.h>
#include <link.h>

#include <iostream>
#include <unordered_map>
#include <vector>
#include <filesystem>
namespace fs = std::filesystem;
#include <string>
#include <string_view>
#include <exception>

#ifndef PLUGIN_DIRECTORY
#error "Missing PLUGIN_DIR definition."
#endif


class DynamicLibraryError: public std::exception
{
public:
  DynamicLibraryError(std::string_view errorMessage);
  DynamicLibraryError();
  const char *what() const noexcept;

private:
  std::string_view message;
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
    typedef INITIALIZE_FUNCTION_SIGNATURE((*initializeFunction_t));
    typedef EXECUTE_FUNCTION_SIGNATURE((*executeFunction_t));
    typedef DEINITIALIZE_FUNCTION_SIGNATURE((*deinitializeFunction_t));
  public:
    void *handle = nullptr;
    bool initialized = false;

    initializeFunction_t initialize;
    executeFunction_t execute;
    deinitializeFunction_t deinitialize;
  };

public:
  PluginManager();
  ~PluginManager();

  void load(bool skipExisting = true, bool lazy = true);
  const std::vector<std::string> getLoaded() const;
  PluginState getStatus(std::string_view pluginName) const;

  void initialize(bool skipInitialized = true);
  void initialize(std::string_view pluginName, bool skipInitialized = true);

  ErrorDescription execute(std::string_view pluginName, void *data);

  void deinitialize(bool skipDeinitialized = true);
  void deinitialize(std::string_view pluginName, bool skipDeinitialized = true);

private:
  Plugin getPlugin(std::string_view pluginName);

private:
  std::unordered_map<std::string, Plugin> loadedPlugins;
};
