cmake_minimum_required(VERSION 3.0)
project(zlib C)

file(GLOB ZLIB_SRC *.c *.h)

add_library(zlib STATIC ${ZLIB_SRC})

target_include_directories(zlib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(zlib PROPERTIES FOLDER "third_party")
set_target_properties(zlib PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(zlib PROPERTIES OUTPUT_NAME z PREFIX "lib" CLEAN_DIRECT_OUTPUT 1)

set(ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
set(ZLIB_LIBRARIES zlib PARENT_SCOPE)
