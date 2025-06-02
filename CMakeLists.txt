cmake_minimum_required(VERSION 3.16)
project(stream-fdl
  LANGUAGES CXX
)

include(GNUInstallDirs)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(_install_include_dir "${CMAKE_INSTALL_FULL_INCLUDEDIR}/")


add_executable(main)
target_include_directories(main
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${_install_include_dir}>
  PRIVATE
    src
)
add_subdirectory(src)


# TODO: install
# TODO: uninstall
