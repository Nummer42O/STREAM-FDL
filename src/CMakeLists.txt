cmake_minimum_required(VERSION 3.16)


find_package(Eigen3 3.3 REQUIRED CONFIG)
# find_package(yaml-cpp 0.7 REQUIRED)

target_sources(main
  PRIVATE
    main.cpp
)
target_link_libraries(main
  PRIVATE
    Eigen3::Eigen
    # ${YAML_CPP_LIBRARIES}
    #yaml-cpp::yaml-cpp
    ${CMAKE_DL_LIBS}
)

add_subdirectory(dynamic-subgraph)
add_subdirectory(fault-detection)
add_subdirectory(fault-trajectory-extraction)
add_subdirectory(3rd)