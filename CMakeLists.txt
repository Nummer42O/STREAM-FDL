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

option(FDL_LOG_MINIMAL "Minimses what meta information is included in the logging output. Implicitly disabled FDL_LOG_TIMESTAMP." OFF)
if(${FDL_LOG_MINIMAL})
  set(_log_minimal_definition "FDL_LOG_MINIMAL")
endif()
option(FDL_LOG_TIMESTAMP "Wether to prepend the current timestamp on the log message. (default: ON)" ON)
if(${FDL_LOG_TIMESTAMP})
  set(_log_timestamp_definition "FDL_LOG_TIMESTAMP")
endif()
set(FDL_LOG_LEVEL "2" CACHE STRING "Minimum log level (TRACE: 0, DEBUG: 1, INFO: 2, WARN: 3, ERROR: 4, FATAL: 5) (default: 2)")
set(_log_level_definition "FDL_LOG_LEVEL=${FDL_LOG_LEVEL}")

add_compile_options(
  -save-temps=obj
)

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
    -Wall -Wextra -Wpedantic -Wno-ignored-qualifiers -Wno-error=unused-parameter -Werror
)
target_compile_definitions(main
  PRIVATE
    PLUGIN_DIRECTORY="$<BUILD_INTERFACE:${_build_plugin_dir}>$<INSTALL_INTERFACE:${_install_plugin_dir}>"
    ${_log_level_definition}
    ${_log_timestamp_definition}
    ${_log_minimal_definition}
    FDL_LOG_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

add_executable(msg-queue-monitor)
target_include_directories(msg-queue-monitor
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${_install_include_dir}>
  PRIVATE
    src
)
target_compile_options(msg-queue-monitor
  PRIVATE
    -Wall -Wextra -Wpedantic -Wno-ignored-qualifiers -Werror
)


add_subdirectory(src)


# TODO: install
# TODO: uninstall
