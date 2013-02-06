cmake_minimum_required(VERSION 2.6)
project(avhttp)
#SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

find_package(Boost 1.48  REQUIRED COMPONENTS date_time filesystem system program_options regex)
find_package(Threads)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(include)

add_library(avhttp STATIC src/avhttp.cpp)
target_link_libraries(avhttp ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
