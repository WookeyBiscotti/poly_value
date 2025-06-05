cmake_minimum_required(VERSION 3.11)

project(poly_value LANGUAGES CXX)

option(POLY_VALUE_ENABLE_TESTING OFF)
option(POLY_VALUE_ENABLE_BENCHMARKS OFF)

add_library(poly_value INTERFACE)
target_include_directories(poly_value INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include/")
set_property(TARGET poly_value PROPERTY CXX_STANDARD 20)

add_library(poly_value::poly_value ALIAS poly_value)

if(POLY_VALUE_ENABLE_TESTING)
    add_subdirectory(tests)
endif()
