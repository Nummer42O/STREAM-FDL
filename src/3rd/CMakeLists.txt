cmake_minimum_required(VERSION 3.16)


add_subdirectory(stream-ipc)
target_link_libraries(main
  PUBLIC
    ipc_lib
)
