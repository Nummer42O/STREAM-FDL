cmake_minimum_required(VERSION 3.16)

target_sources(main
  PUBLIC
    fault-detection.cpp
    circular-buffer.cpp
    # plugin-manager.cpp
    watchlist.cpp
)
