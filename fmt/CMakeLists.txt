cmake_minimum_required(VERSION 3.0)
file(GLOB FMT_SRC src/*.cc include/fmt/*.h)

add_library(fmt STATIC ${FMT_SRC})

target_include_directories(fmt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/)
set_target_properties(fmt PROPERTIES FOLDER "third_party")
set_target_properties(fmt PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(FMT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/ PARENT_SCOPE)
set(FMT_LIBRARIES fmt PARENT_SCOPE)
