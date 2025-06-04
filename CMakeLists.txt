cmake_minimum_required(VERSION 3.16)
project(stream-fdl
  LANGUAGES CXX
)

include(GNUInstallDirs)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(_install_include_dir  "${CMAKE_INSTALL_FULL_INCLUDEDIR}/")
set(_build_plugin_dir     "${CMAKE_BINARY_DIR}/plugins")
set(_install_plugin_dir   "${CMAKE_INSTALL_FULL_LIBDIR}/plugins")


add_executable(main)
target_include_directories(main
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${_install_include_dir}>
  PRIVATE
    src
)
target_compile_options(main
  PRIVATE
    -Wall -Wextra -Wpedantic -Wno-ignored-qualifiers -Werror
)
target_compile_definitions(main
  PRIVATE
    PLUGIN_DIRECTORY="$<BUILD_INTERFACE:${_build_plugin_dir}>$<INSTALL_INTERFACE:${_install_plugin_dir}>"
)

add_subdirectory(src)


# TODO: install
# TODO: uninstall
