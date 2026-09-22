//
// Created by cpsusie on 9/21/26.
//

#ifndef HAND_EVALUATOR_SETUP_STREAMS_HPP
#define HAND_EVALUATOR_SETUP_STREAMS_HPP
#include <string_view>
#include <span>
#include <optional>
#include <expected>
#include <ct_str/ct_string_view.hpp>
namespace cps::ct_string::test_app
{
    using literals::operator ""_ctsv;
    using literals::operator""_fs;
    using namespace std::literals;
    // NB: the default argument uses the make_ctsv free-function spelling rather
    // than the _ctsv literal. MSVC fails to deduce the string-literal NTTP when a
    // literal operator template is named through a using-declaration and invoked
    // at namespace scope (as in a default argument); make_ctsv is portable.
    std::expected<void, std::string> setup_streams(
        ct_cstring_view locale_txt = make_ctsv<"en_US.UTF-8">(),
        std::span<const std::string_view> args = {}) noexcept;
}
#endif //HAND_EVALUATOR_SETUP_STREAMS_HPP
