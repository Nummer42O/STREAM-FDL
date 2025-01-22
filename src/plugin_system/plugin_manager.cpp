#include "plugin_manager.hpp"

#include <utility>
#include <string>
using namespace std::string_literals;


DynamicLibraryError::DynamicLibraryError(std::string_view errorMessage)
{
  this->message = errorMessage;
}

DynamicLibraryError::DynamicLibraryError()
{
  this->message = dlerror();
}

const char *DynamicLibraryError::what() const noexcept
{
  return this->message.begin();
}




PluginManager::PluginManager():
  loadedPlugins()
{}

PluginManager::~PluginManager()
{
  for (const std::pair<std::string, Plugin>& element: this->loadedPlugins)
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
    {
      const char *error = dlerror();
      throw DynamicLibraryError(
        error ? error : "Unknown error. See NOTES section in dlsym manual page."
      );
    }

    plugin.execute = reinterpret_cast<Plugin::executeFunction_t>(
      dlsym(libraryHandle, EXECUTE_SYMBOL_NAME)
    );
    if (!plugin.execute)
    {
      const char *error = dlerror();
      throw DynamicLibraryError(
        error ? error : "Unknown error. See NOTES section in dlsym manual page."
      );
    }

    plugin.deinitialize = reinterpret_cast<Plugin::deinitializeFunction_t>(
      dlsym(libraryHandle, DEINITIALIZE_SYMBOL_NAME)
    );
    if (!plugin.deinitialize)
    {
      const char *error = dlerror();
      throw DynamicLibraryError(
        error ? error : "Unknown error. See NOTES section in dlsym manual page."
      );
    }

    this->loadedPlugins.insert_or_assign(
      entryPath.stem().string(), plugin
    );
  }
}

const std::vector<std::string> PluginManager::getLoaded() const
{
  std::vector<std::string> loadedPluginNames;
  loadedPluginNames.reserve(this->loadedPlugins.size());

  for (const std::pair<std::string, Plugin> &element: this->loadedPlugins)
  {
    loadedPluginNames.push_back(element.first);
  }

  return loadedPluginNames;
}

PluginManager::PluginState PluginManager::getStatus(std::string_view pluginName) const
{
  const auto &plugin = this->loadedPlugins.find(pluginName.begin());
  if (plugin == this->loadedPlugins.end())
    return PLUGIN_UNKNOWN;

  if (!plugin->second.initialized)
    return PLUGIN_LOADED;

  return PLUGIN_INITIALIZED;
}

void PluginManager::initialize(bool skipInitialized)
{
  for (const std::pair<std::string, Plugin> &element: this->loadedPlugins)
  {
    if (skipInitialized && element.second.initialized)
      continue;

    if (!element.second.initialize())
      throw DynamicLibraryError(
        "Unable to initialize plugin: " + element.first
      );
  }
}

void PluginManager::initialize(std::string_view pluginName, bool skipInitialized)
{
  const Plugin &plugin = this->getPlugin(pluginName);

  if (skipInitialized && plugin.initialized)
    return;

  if (!plugin.initialize())
    throw DynamicLibraryError(
      "Unable to initialize plugin: "s + pluginName.begin()
    );
}

ErrorDescription PluginManager::execute(std::string_view pluginName, void *data)
{
  const Plugin &plugin = this->getPlugin(pluginName);

  //! TODO: make name-independent function that executes plugin with data
  //!       using information from configuration file
  return plugin.execute(data);
}

void PluginManager::deinitialize(bool skipDeinitialized)
{
  for (const std::pair<std::string, Plugin> &element: this->loadedPlugins)
  {
    if (skipDeinitialized && !element.second.initialized)
      continue;

    element.second.deinitialize();
  }
}

void PluginManager::deinitialize(std::string_view pluginName, bool skipDeinitialized)
{
  const Plugin &plugin = this->getPlugin(pluginName);

  if (skipDeinitialized && !plugin.initialized)
    return;

  plugin.deinitialize();
}

PluginManager::Plugin PluginManager::getPlugin(std::string_view pluginName)
{
  const auto &plugin = this->loadedPlugins.find(pluginName.begin());
  if (plugin == this->loadedPlugins.end())
    throw DynamicLibraryError(
      "Unknown plugin: "s + pluginName.begin()
    );

  return plugin->second;
}
