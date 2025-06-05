include(FetchContent)

FetchContent_Declare(catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
    GIT_SHALLOW ON
)

FetchContent_MakeAvailable(catch2)

add_executable(poly_value_tests tests.cpp)
target_link_libraries(poly_value_tests PRIVATE poly_value::poly_value Catch2::Catch2WithMain)
set_property(TARGET poly_value_tests PROPERTY CXX_STANDARD 23)

include(CTest)
include(Catch)
catch_discover_tests(poly_value_tests)
