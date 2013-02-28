cmake_minimum_required(VERSION 2.6)
project(avhttp)
#SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

find_package(Boost 1.48  REQUIRED COMPONENTS date_time thread filesystem system program_options regex)
find_package(Threads)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(include)

add_executable(avhttp test/multi_download_test.cpp)
target_link_libraries(avhttp ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
