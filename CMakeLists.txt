cmake_minimum_required(VERSION 3.16)
project(stream-fdl
  LANGUAGES CXX
)

include(GNUInstallDirs)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)


add_executable(main)
target_include_directories(main
  PUBLIC
    include
  PRIVATE
    src
)
add_subdirectory(src)


# TODO: install
# TODO: uninstall
