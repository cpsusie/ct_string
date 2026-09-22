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
    std::expected<void, std::string> setup_streams(ct_cstring_view locale_txt = "en_US.UTF-8"_ctsv,
        std::span<const std::string_view> args = {}) noexcept;
}
#endif //HAND_EVALUATOR_SETUP_STREAMS_HPP
