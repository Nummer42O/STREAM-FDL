cmake_minimum_required(VERSION 3.16)


add_subdirectory(stream-ipc)

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
add_subdirectory(json)

target_link_libraries(main
  PUBLIC
    ipc_lib
    nlohmann_json::nlohmann_json
)
