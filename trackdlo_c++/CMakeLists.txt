cmake_minimum_required(VERSION 3.10)
project(trackdlo_app)

set(CMAKE_CXX_STANDARD 14)

set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 REQUIRED)

find_package(spdlog REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(yaml-cpp REQUIRED)
find_package(PCL 1.8 REQUIRED COMPONENTS common io filters visualization features kdtree)

include_directories(include SYSTEM PUBLIC
  ${Eigen_INCLUDE_DIRS}
  ${PCL_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/include
)

add_definitions(${PCL_DEFINITIONS})

pybind11_add_module(trackdlo_app MODULE src/trackdlo_app.cpp src/trackdlo.cpp src/utils.cpp)
target_link_libraries(trackdlo_app PRIVATE pybind11::module ${OpenCV_LIBS} ${PCL_LIBRARIES} yaml-cpp)
