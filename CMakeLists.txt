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

# Common warning flags for our own targets.
# clang-cl reports CMAKE_CXX_COMPILER_ID as "Clang" but uses the MSVC-style
# driver, so it must be routed to the /W4 /WX branch alongside real MSVC.
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"
        OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    set(CT_STR_WARNING_FLAGS /W4 /WX)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CT_STR_WARNING_FLAGS -Wall -Wextra -Werror)
endif()

add_library(ct_str_warnings INTERFACE)
target_compile_options(ct_str_warnings INTERFACE ${CT_STR_WARNING_FLAGS})

add_executable(test_console_app test_console_app/main.cpp test_console_app/impl_views.cpp)
target_link_libraries(test_console_app PRIVATE cstr_view fmt::fmt ct_str_warnings)

# unit tests
find_package(GTest CONFIG REQUIRED)

add_executable(cstr_view_tests tests/cstr_view_tests.cpp)
target_link_libraries(cstr_view_tests PRIVATE cstr_view GTest::gtest GTest::gtest_main fmt::fmt ct_str_warnings)

enable_testing()
add_test(NAME cstr_view_tests COMMAND cstr_view_tests)
