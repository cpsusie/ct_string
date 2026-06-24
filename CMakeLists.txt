cmake_minimum_required(VERSION 4.0.2)
project(cstr_view VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(fmt CONFIG REQUIRED)

add_library(cstr_view INTERFACE)

target_include_directories(cstr_view INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
        $<INSTALL_INTERFACE:include>
)


add_executable(test_console_app test_console_app/main.cpp test_console_app/impl_views.cpp)
target_link_libraries(test_console_app PRIVATE cstr_view fmt::fmt)

# unit tests
find_package(GTest CONFIG REQUIRED)

add_executable(cstr_view_tests tests/cstr_view_tests.cpp)
target_link_libraries(cstr_view_tests PRIVATE cstr_view GTest::gtest GTest::gtest_main fmt::fmt)

enable_testing()
add_test(NAME cstr_view_tests COMMAND cstr_view_tests)
