cmake_minimum_required(VERSION 3.11)

project(poly_value LANGUAGES CXX)

option(POLY_VALUE_ENABLE_TESTING OFF)
option(POLY_VALUE_ENABLE_EXAMPLES OFF)

add_library(poly_value INTERFACE)
target_include_directories(poly_value INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include/")
set_property(TARGET poly_value PROPERTY CXX_STANDARD 20)

add_library(poly_value::poly_value ALIAS poly_value)

if(POLY_VALUE_ENABLE_EXAMPLES)
    add_executable(poly_value_examples examples/examples.cpp)
    target_link_libraries(poly_value_examples PRIVATE poly_value::poly_value)
    set_property(TARGET poly_value_examples PROPERTY CXX_STANDARD 23)
endif()

if(POLY_VALUE_ENABLE_TESTING)
    include(FetchContent)

    FetchContent_Declare(catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.8.1
        GIT_SHALLOW ON
    )
    FetchContent_MakeAvailable(catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

    add_executable(poly_value_tests tests/tests.cpp)
    target_link_libraries(poly_value_tests PRIVATE poly_value::poly_value Catch2::Catch2WithMain)
    set_property(TARGET poly_value_tests PROPERTY CXX_STANDARD 23)

    include(CTest)
    include(Catch)
    catch_discover_tests(poly_value_tests)
endif()
