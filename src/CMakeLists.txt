cmake_minimum_required(VERSION 3.16)


target_sources(main
  PRIVATE
    main.cpp
)
add_subdirectory(plugin_system)
add_subdirectory(3rd)