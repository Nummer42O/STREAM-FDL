#include "dynamic-subgraph/dynamic-subgraph.hpp"
#include "dynamic-subgraph/data-store.hpp"
#include "common.hpp"

#include "nlohmann/json.hpp"
namespace json = nlohmann;

#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;
#include <fstream>
#include <cerrno>
#include <cstring>
#include <csignal>
#include <atomic>

#define MODE_NORMAL   "--normal"
#define MODE_HOLISTIC "--holistic"


static std::atomic<bool> gIsRunning;
void sigintHandler(int signum)
{
  LOG_INFO("Received " << ::strsignal(signum) << ", aborting.");
  gIsRunning.store(false);
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    std::cerr <<
      "Invalid number of arguments " << argc - 1 << ", expected 2.\n"
      "Usage: " << argv[0] << " (" MODE_NORMAL "|" MODE_HOLISTIC ") CONFIGURATION_FILE\n";
    return 1;
  }

  const char *mode = argv[1];
  bool runHolistic;
  if (std::strcmp(mode, MODE_HOLISTIC) == 0)
    runHolistic = true;
  else if (std::strcmp(mode, MODE_NORMAL) == 0)
    runHolistic = false;
  else
  {
    std::cerr <<
      "Invalid argument '" << mode << "', use " MODE_NORMAL " or " MODE_HOLISTIC ".\n"
      "Usage: " << argv[0] << " (" MODE_NORMAL "|" MODE_HOLISTIC ") CONFIGURATION_FILE\n";
    return 1;
  }

  fs::path configFilePath(argv[2]);
  if (!fs::is_regular_file(configFilePath) ||
      configFilePath.extension() != ".json")
  {
    LOG_FATAL("Invalid configuration file " << configFilePath << ", must be regular json file.\n")
    return 2;
  }

  std::ifstream configFile(configFilePath, std::ios_base::in);
  if (!configFile.is_open())
  {
    LOG_FATAL("Can not open file" << configFilePath << ": " << std::strerror(errno) << '\n')
    return 3;
  }

  LOG_DEBUG("Opened config file: " << configFilePath);
  //! NOTE: theoretically json::json::parse has a third boolean argument to
  //!       ignore trailing commas too, but aparently not? See:
  //!       https://json.nlohmann.me/api/basic_json/parse/
  json::json config = json::json::parse(configFile, nullptr, true, true);
  configFile.close();

  {
    DataStore dataStore(config);
    DynamicSubgraphBuilder dsg(config, &dataStore, runHolistic);
    LOG_TRACE("Initialised DataStore and DSG.");

    sighandler_t intHandler = signal(SIGINT, sigintHandler);
    sighandler_t hupHandler = signal(SIGHUP, sigintHandler);
    gIsRunning.store(true);
    dsg.run(gIsRunning);
    signal(SIGINT, intHandler);
    signal(SIGHUP, hupHandler);
    LOG_TRACE("Main loop exited.")
  }

  return 0;
}
