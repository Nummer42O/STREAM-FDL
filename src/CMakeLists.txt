cmake_minimum_required(VERSION 3.16)


find_package(Eigen3 3.3 REQUIRED CONFIG)
# find_package(yaml-cpp 0.7 REQUIRED)
find_package(OpenCV 4.6 REQUIRED)
find_package(PkgConfig 1.8 REQUIRED)
pkg_check_modules(
  GraphViz
  IMPORTED_TARGET
    libcgraph
    libgvc
  REQUIRED
)

target_sources(main
  PRIVATE
    main.cpp
)
target_link_libraries(main
  PRIVATE
    Eigen3::Eigen
    # ${YAML_CPP_LIBRARIES}
    ${CMAKE_DL_LIBS}
    ${OpenCV_LIBS}
    ${GraphViz_LIBRARIES}
)

add_subdirectory(dynamic-subgraph)
add_subdirectory(fault-detection)
add_subdirectory(fault-trajectory-extraction)
add_subdirectory(3rd)