# 3.21 is the first CMake with PROJECT_IS_TOP_LEVEL, used by the options below.
# 4.4 is the newest CMake the project is verified with: a newer CMake applies
# that version's policies (D-017).
cmake_minimum_required(VERSION 3.21...4.4)
project(ct_string VERSION 1.0.0 LANGUAGES CXX)

# What to build besides the header-only library (D-018, D-033). Each option is
# ON only when this is the top-level project, so a consumer using
# add_subdirectory or FetchContent gets the INTERFACE library alone, and needs
# neither fmt nor GoogleTest.
option(CT_STRING_BUILD_CXX20_CHECKS
        "Build cstr_view_cxx20_smoke, the C++20 floor check (uses fmt if found)"
        ${PROJECT_IS_TOP_LEVEL})
option(CT_STRING_BUILD_TESTS
        "Build cstr_view_tests, the C++23 test suite (needs GoogleTest and fmt)"
        ${PROJECT_IS_TOP_LEVEL})
option(CT_STRING_BUILD_EXAMPLES
        "Build test_console_app, the C++23 demo (needs fmt)"
        ${PROJECT_IS_TOP_LEVEL})

# The header-only library is C++20. Only the tests and the demo console app
# need C++23 (std::expected, and formerly std::ranges::to), and they ask for it
# per-target via target_compile_features rather than by mutating
# CMAKE_CXX_STANDARD half way down this file -- the latter silently applied to
# whatever happened to be declared afterwards.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The project uses no C++ modules. CMake 3.28 and later would otherwise scan
# every C++20 source for them, which with Clang needs clang-scan-deps (F-6).
# This covers the targets created below; older CMakes ignore it.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

add_library(ct_string INTERFACE)
add_library(ct_string::ct_string ALIAS ct_string)
# The target's name before T4 (Q-01), so that existing
# target_link_libraries(... cstr_view) keeps working.
add_library(cstr_view ALIAS ct_string)

# Declare the library's actual language floor so that consumers inherit it
# (and so that a C++17 consumer fails loudly rather than mysteriously).
target_compile_features(ct_string INTERFACE cxx_std_20)

target_include_directories(ct_string INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
        $<INSTALL_INTERFACE:include>
)

if (NOT (CT_STRING_BUILD_CXX20_CHECKS OR CT_STRING_BUILD_TESTS OR CT_STRING_BUILD_EXAMPLES))
    return()
endif()

# Dependencies are looked up only for what is built. The suite and the demo
# need fmt; the C++20 check uses it if it is installed, and builds without it
# otherwise.
if (CT_STRING_BUILD_TESTS OR CT_STRING_BUILD_EXAMPLES)
    find_package(fmt CONFIG REQUIRED)
else()
    find_package(fmt CONFIG QUIET)
endif()

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
# raised by anything else in the build). A second TU, made only of
# compile-time checks, pins down what the two class templates compare with
# (D-030); being here, it runs wherever the C++20 floor is checked. It needs
# neither GoogleTest nor a C++23 compiler (D-005).
if (CT_STRING_BUILD_CXX20_CHECKS)
    add_library(cstr_view_cxx20_smoke STATIC
            tests/cxx20_header_smoke.cpp
            tests/cxx20_comparison_checks.cpp
    )
    target_link_libraries(cstr_view_cxx20_smoke PRIVATE ct_string ct_str_warnings)
    # Until T11, the header enables its fmt support whenever <fmt/format.h> is
    # on the include path (F-7): "without fmt" means fmt is not installed.
    if (TARGET fmt::fmt-header-only)
        target_link_libraries(cstr_view_cxx20_smoke PRIVATE fmt::fmt-header-only)
        message(STATUS "ct_string: the C++20 checks build with fmt")
    else()
        message(STATUS "ct_string: the C++20 checks build without fmt (not found)")
    endif()
    set_target_properties(cstr_view_cxx20_smoke PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
    )
endif()

if (CT_STRING_BUILD_EXAMPLES)
    add_executable(test_console_app
            test_console_app/main.cpp
            test_console_app/impl_views.cpp
            test_console_app/setup_streams.cpp
            test_console_app/fmt_reg_demo.cpp
    )
    target_link_libraries(test_console_app PRIVATE ct_string fmt::fmt ct_str_warnings)
    # std::expected in setup_streams.hpp
    target_compile_features(test_console_app PRIVATE cxx_std_23)
endif()

# unit tests
if (CT_STRING_BUILD_TESTS)
    find_package(GTest CONFIG REQUIRED)

    add_executable(cstr_view_tests
            tests/cstr_view_tests.cpp
            tests/ctsv_comparators_tests.cpp
            tests/ctsv_containers_tests.cpp
    )
    target_link_libraries(cstr_view_tests PRIVATE ct_string GTest::gtest GTest::gtest_main fmt::fmt-header-only ct_str_warnings)
    target_compile_features(cstr_view_tests PRIVATE cxx_std_23)
    # The tests always link {fmt}, so the header must detect it. Saying so lets
    # tests/cstr_view_tests.cpp fail to compile, with a clear message, instead of
    # silently skipping its fmt::formatter tests.
    target_compile_definitions(cstr_view_tests PRIVATE CPS_CT_STRING_TESTS_EXPECT_FMT)

    enable_testing()
    add_test(NAME cstr_view_tests COMMAND cstr_view_tests)
endif()
