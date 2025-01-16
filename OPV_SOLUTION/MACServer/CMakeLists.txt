cmake_minimum_required(VERSION 3.10)
project(mac_server)

find_package(OpenCV REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

add_executable(mac_server mac_server.cpp)
target_link_libraries(mac_server ${OpenCV_LIBS} Boost::system)
