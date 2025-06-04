#include "dynamic-subgraph/dynamic-subgraph.hpp"
#include "dynamic-subgraph/data-store.hpp"

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


static std::atomic<bool> gIsRunning;
void sigintHandler(int)
{
  std::cout << "\rCalled SIGINT handler, aborting.\n";
  gIsRunning.store(false);
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    std::cerr <<
      "Invalid number of arguments (" << argc << ").\n"
      "Usage: " << argv[0] << " CONFIGURATION_FILE\n";
    return 1;
  }

  fs::path configFilePath(argv[1]);
  if (!fs::is_regular_file(configFilePath) ||
      configFilePath.extension() != ".json")
  {
    std::cerr << "Invalid configuration file " << configFilePath << ", must be regular json file.\n";
    return 2;
  }

  std::ifstream configFile(configFilePath, std::ios_base::in);
  if (!configFile.is_open())
  {
    std::cerr << "Can not open file" << configFilePath << ": " << std::strerror(errno) << '\n';
    return 3;
  }

  json::json config = json::json::parse(configFile);
  configFile.close();

  {
    DataStore dataStore(config);
    DynamicSubgraphBuilder dsg(config, &dataStore);

    sighandler_t defaultHandler = signal(SIGINT, sigintHandler);
    gIsRunning.store(true);
    dsg.run(gIsRunning);
    signal(SIGINT, defaultHandler);
  }

  return 0;
}
