cmake_minimum_required(VERSION 4.0.2)
project(cstr_view VERSION 1.0.0 LANGUAGES CXX)

# The header-only library is C++20. Only the tests and the demo console app
# need C++23 (std::expected, and formerly std::ranges::to), and they ask for it
# per-target via target_compile_features rather than by mutating
# CMAKE_CXX_STANDARD half way down this file -- the latter silently applied to
# whatever happened to be declared afterwards.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(fmt CONFIG REQUIRED)

add_library(cstr_view INTERFACE)

# Declare the library's actual language floor so that consumers inherit it
# (and so that a C++17 consumer fails loudly rather than mysteriously).
target_compile_features(cstr_view INTERFACE cxx_std_20)

target_include_directories(cstr_view INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
        $<INSTALL_INTERFACE:include>
)

# Common warning flags for our own targets.
# clang-cl reports CMAKE_CXX_COMPILER_ID as "Clang" but uses the MSVC-style
# driver, so it must be routed to the /W4 /WX branch alongside real MSVC.
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"
        OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    # C4702 (unreachable code) is emitted by the MSVC back end, which ignores
    # /external:W0, so warnings originating in third-party headers such as
    # <fmt/*.h> cannot be scoped away and would trip /WX. Disable it globally:
    # it is a code-layout diagnostic we do not depend on. clang-cl accepts
    # /wd4702 as a no-op, so this branch stays shared.
    set(CT_STR_WARNING_FLAGS /W4 /WX /wd4702)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CT_STR_WARNING_FLAGS -Wall -Wextra -Werror)
endif()

add_library(ct_str_warnings INTERFACE)
target_compile_options(ct_str_warnings INTERFACE ${CT_STR_WARNING_FLAGS})

# ---------------------------------------------------------------------------
# C++20 conformance smoke test
# ---------------------------------------------------------------------------
# A single TU that includes every public header and is PINNED to C++20, so a
# C++23-only construct creeping into inc/ct_str is a build failure here rather
# than a surprise for a C++20 consumer. The standard is set via target
# properties (not merely cxx_std_20, which is a *minimum* and would happily be
# raised by anything else in the build).
add_library(cstr_view_cxx20_smoke STATIC tests/cxx20_header_smoke.cpp)
target_link_libraries(cstr_view_cxx20_smoke PRIVATE cstr_view fmt::fmt-header-only ct_str_warnings)
set_target_properties(cstr_view_cxx20_smoke PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

add_executable(test_console_app
        test_console_app/main.cpp
        test_console_app/impl_views.cpp
        test_console_app/setup_streams.cpp
        test_console_app/fmt_reg_demo.cpp
)
target_link_libraries(test_console_app PRIVATE cstr_view fmt::fmt ct_str_warnings)
# std::expected in setup_streams.hpp
target_compile_features(test_console_app PRIVATE cxx_std_23)

# unit tests
find_package(GTest CONFIG REQUIRED)

add_executable(cstr_view_tests
        tests/cstr_view_tests.cpp
        tests/ctsv_comparators_tests.cpp
        tests/ctsv_containers_tests.cpp
)
target_link_libraries(cstr_view_tests PRIVATE cstr_view GTest::gtest GTest::gtest_main fmt::fmt-header-only ct_str_warnings)
target_compile_features(cstr_view_tests PRIVATE cxx_std_23)

enable_testing()
add_test(NAME cstr_view_tests COMMAND cstr_view_tests)
