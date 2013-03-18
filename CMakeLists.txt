cmake_minimum_required(VERSION 2.6)
project(avhttp)
#SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

OPTION(ENABLE_OPENSSL "Enable use of OpenSSL" ON)

find_package(Boost 1.49  REQUIRED COMPONENTS locale date_time thread filesystem system program_options regex)
find_package(Threads)

if (ENABLE_OPENSSL)
	find_package(OpenSSL)
	add_definitions(-DAVHTTP_ENABLE_OPENSSL)
endif()

include_directories(${Boost_INCLUDE_DIRS})
include_directories(include)

add_executable(avhttp example/multi_download.cpp)
target_link_libraries(avhttp ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${OPENSSL_LIBRARIES} ${CMAKE_DL_LIBS})
