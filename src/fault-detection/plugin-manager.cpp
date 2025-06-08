#include "fault-detection/plugin-manager.hpp"

#include <utility>
#include <string>
using namespace std::string_literals;


const char *DynamicLibraryError::getDlError()
{
  const char *error = dlerror();
  return error ? error : "Unknown error. See NOTES section in dlsym manual page.";
}


PluginManager::~PluginManager()
{
  for (const std::pair<std::string, Plugin>& element: mLoadedPlugins)
  {
    if (!element.second.handle)
      continue;

    if (element.second.initialized)
      element.second.deinitialize();

    //! NOTE: This currently silently ignores errors.
    //! However, this SHOULD be fine since the destructor
    //! is probably only going to run at program shutdown.
    dlclose(element.second.handle);
  }
}

void PluginManager::load(bool skipExisting, bool lazy)
{
  for (const fs::directory_entry &entry: fs::directory_iterator(PLUGIN_DIRECTORY))
  {
    const fs::path &entryPath = entry.path();
    //! TODO: maybe check mime type for application/x-sharedlib
    if (!entry.is_regular_file() ||
        entryPath.extension() != ".so")
      continue;

    std::string pluginName = entryPath.stem().string();
    if (skipExisting && mLoadedPlugins.find(pluginName) == mLoadedPlugins.end())
      continue;

    // clear error cache
    dlerror();

    void *libraryHandle = dlopen(
      entryPath.c_str(),
      (lazy ? RTLD_LAZY : RTLD_NOW)
    );
    if (!libraryHandle)
      throw DynamicLibraryError();

    Plugin plugin;
    plugin.handle = libraryHandle;
    plugin.initialized = false;

    plugin.initialize = reinterpret_cast<Plugin::initializeFunction_t>(
      dlsym(libraryHandle, INITIALIZE_SYMBOL_NAME)
    );
    if (!plugin.initialize)
      throw DynamicLibraryError();

    plugin.execute = reinterpret_cast<Plugin::executeFunction_t>(
      dlsym(libraryHandle, EXECUTE_SYMBOL_NAME)
    );
    if (!plugin.execute)
      throw DynamicLibraryError();

    plugin.deinitialize = reinterpret_cast<Plugin::deinitializeFunction_t>(
      dlsym(libraryHandle, DEINITIALIZE_SYMBOL_NAME)
    );
    if (!plugin.deinitialize)
      throw DynamicLibraryError();

    mLoadedPlugins.insert_or_assign(
      pluginName, plugin
    );
  }
}

const std::vector<std::string> PluginManager::getLoaded() const
{
  std::vector<std::string> loadedPluginNames;
  loadedPluginNames.reserve(mLoadedPlugins.size());
  std::transform(
    mLoadedPlugins.begin(), mLoadedPlugins.end(),
    std::back_inserter(loadedPluginNames),
    [](const Plugins::value_type &plugin) -> std::string
    {
      return plugin.first;
    }
  );

  return loadedPluginNames;
}

PluginManager::PluginState PluginManager::getStatus(std::string_view pluginName) const
{
  Plugins::const_iterator pluginIt = mLoadedPlugins.find(pluginName.begin());
  if (pluginIt == mLoadedPlugins.end())
    return PLUGIN_UNKNOWN;

  if (!pluginIt->second.initialized)
    return PLUGIN_LOADED;

  return PLUGIN_INITIALIZED;
}

void PluginManager::initialize(bool skipInitialized)
{
  for (Plugins::value_type &element: mLoadedPlugins)
  {
    if (skipInitialized && element.second.initialized)
      continue;

    if (!element.second.initialize())
      throw DynamicLibraryError(
        "Unable to initialize plugin: " + element.first
      );
    else
      element.second.initialized = true;
  }
}

void PluginManager::initialize(std::string_view pluginName, bool skipInitialized)
{
  Plugin &plugin = getPlugin(pluginName);

  if (skipInitialized && plugin.initialized)
    return;

  if (!plugin.initialize())
    throw DynamicLibraryError(
      "Unable to initialize plugin: "s + pluginName.begin()
    );
  else
    plugin.initialized = true;
}

FaultState PluginManager::execute(std::string_view pluginName, void *data)
{
  const Plugin &plugin = getPlugin(pluginName);

  //! TODO: make name-independent function that executes plugin with data
  //!       using information from configuration file
  // return plugin.execute(data);
}

void PluginManager::deinitialize(bool skipDeinitialized)
{
  for (Plugins::value_type &element: mLoadedPlugins)
  {
    if (skipDeinitialized && !element.second.initialized)
      continue;

    element.second.deinitialize();
    element.second.initialized = false;
  }
}

void PluginManager::deinitialize(std::string_view pluginName, bool skipDeinitialized)
{
  Plugin &plugin = getPlugin(pluginName);

  if (skipDeinitialized && !plugin.initialized)
    return;

  plugin.deinitialize();
  plugin.initialized = false;
}

PluginManager::Plugin &PluginManager::getPlugin(std::string_view pluginName)
{
  Plugins::iterator pluginIt = mLoadedPlugins.find(pluginName.begin());
  if (pluginIt == mLoadedPlugins.end())
    throw DynamicLibraryError(
      "Unknown plugin: "s + pluginName.begin()
    );

  return pluginIt->second;
}
