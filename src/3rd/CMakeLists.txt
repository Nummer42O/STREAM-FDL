cmake_minimum_required(VERSION 3.16)


add_subdirectory(stream-ipc)

find_package(nlohmann_json 3.12 QUIET)
if(NOT ${nlohmann_json_FOUND})
  set(JSON_BuildTests OFF CACHE INTERNAL "")
  set(JSON_Install OFF CACHE INTERNAL "")
  add_subdirectory(json)
endif()

target_link_libraries(main
  PUBLIC
    ipc_lib
    nlohmann_json::nlohmann_json
)
target_link_libraries(msg-queue-monitor
  PUBLIC
    ipc_lib
)
