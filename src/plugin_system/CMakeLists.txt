cmake_minimum_required(VERSION 3.16)


set(BUILD_PLUGIN_DIR   "${CMAKE_BINARY_DIR}/plugins")
set(INSTALL_PLUGIN_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/plugins")


add_library(plugin_manager
  SHARED
  plugin_manager.cpp
)
target_link_libraries(plugin_manager
  PUBLIC
    ${CMAKE_DL_LIBS}
)
target_compile_definitions(plugin_manager
  PRIVATE
    PLUGIN_DIRECTORY="$<BUILD_INTERFACE:${BUILD_PLUGIN_DIR}>$<INSTALL_INTERFACE:${INSTALL_PLUGIN_DIR}>"
)
