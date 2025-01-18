cmake_minimum_required(VERSION 3.16)
project(EdgeBLEWithOpenCV)

set(CMAKE_CXX_STANDARD 17)

# OpenCV configuration
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR})

# Boost configuration
find_package(Boost REQUIRED COMPONENTS system)
include_directories(${Boost_INCLUDE_DIRS})

# Source files
add_executable(edge_ble main.cpp edge_ble.cpp)
target_link_libraries(edge_ble ${OpenCV_LIBRARIES} Boost::system)
