// ---------------------------------------------------------------------------
// cxx20_header_smoke.cpp -- proves the header-only library builds as C++20.
// ---------------------------------------------------------------------------
//
// The test suite and the demo console app are C++23 (GTest niceties,
// std::expected). The LIBRARY is not, and must not become so by accident: a
// stray std::ranges::to, deducing-this, or unguarded C++23 <string_view>
// member would otherwise go unnoticed until a consumer on an older toolchain
// (e.g. GCC 13, whose libstdc++ predates P1206R7) tried to build it.
//
// This TU is pinned to C++20 by the build (see CMakeLists.txt), includes every
// public header, and instantiates enough of each to force the templates to be
// checked rather than merely parsed. If it stops compiling, something in
// inc/ct_str has silently raised the language floor.
//
// Anything genuinely C++23-only in the headers must therefore stay behind a
// feature-test macro with a C++20 fallback, exactly as basic_ct_string_view's
// contains() members are (__cpp_lib_string_contains).
// ---------------------------------------------------------------------------
#include <ct_str/char_fold.hpp>
#include <ct_str/ct_string_view.hpp>
#include <ct_str/ctsv_comparators.hpp>
#include <ct_str/ctsv_containers.hpp>
#include <ct_str/ctsv_format_registration.hpp>
#include <ct_str/fixed_string.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <string>
#include <string_view>

namespace
{
    using namespace cps::ct_string;
    using namespace cps::ct_string::literals;

    // -- fixed_string / ct_string_view: the compile-time core ---------------
    constexpr auto k_fixed   = "alpha"_fs;
    constexpr auto k_view    = "alpha"_ctsv;
    constexpr auto k_made    = make_ctsv<basic_fixed_string{"alpha"}>();
    constexpr auto k_concat  = "al"_fs + "pha"_fs;

    static_assert(k_fixed.valid_cstr());
    static_assert(k_view.known_cstr);
    static_assert(k_view.size() == 5U);
    static_assert(k_view == k_made);
    static_assert(k_concat == k_fixed);
    static_assert(k_view.substr(1U) == "lpha");
    static_assert(k_view.substr(0U, 2U) == "al");
    static_assert(!decltype(k_view.substr(0U, 2U))::known_cstr);
    static_assert(k_view.starts_with("al") && k_view.ends_with("ha"));
    static_assert(k_view.contains("lph"));
    static_assert(*k_view.c_str() == 'a');
    // NB: decltype(k_view) is CONST-qualified (k_view is constexpr), and a
    // const view is not movable, hence not a view. Ask about the type itself.
    static_assert(std::ranges::borrowed_range<ct_cstring_view>);
    static_assert(std::ranges::view<ct_cstring_view>);
    static_assert(std::ranges::view<ct_string_view>);

    // Every character flavor must instantiate, not just the two streamable
    // ones -- the wide/UTF specializations are where a std:: member that only
    // exists in C++23 would most plausibly hide.
    constexpr auto k_wide    = L"alpha"_ctsv;
    constexpr auto k_utf8    = u8"alpha"_ctsv;
    constexpr auto k_utf16   = u"alpha"_ctsv;
    constexpr auto k_utf32   = U"alpha"_ctsv;
    static_assert(k_wide.size() == k_utf8.size());
    static_assert(k_utf16.size() == k_utf32.size());

    // -- char_fold: the lazy adaptors ---------------------------------------
    static_assert(ascii_to_lower<char>('A') == 'a');
    static_assert(ascii_to_upper<char>('a') == 'A');
    static_assert(std::ranges::equal(views::ascii_lower(k_view), std::string_view{"alpha"}));

    // -- comparators ---------------------------------------------------------
    static_assert(transparent_ctsv_less<ctsv_less<char>, char>);
    static_assert(transparent_ctsv_less<ctsv_ci_less<char>, char>);
    static_assert(transparent_ctsv_equal_to<ctsv_ci_equal_to<char>, char>);
    static_assert(transparent_ctsv_three_way<ctsv_ci_three_way<char>, char>);
    static_assert(consistent_ctsv_hash_equal<ctsv_ci_hash<char>, ctsv_ci_equal_to<char>, char>);
    static_assert(ctsv_ci_hash<char>{}(std::string_view{"Ace"})
               == ctsv_ci_hash<char>{}(std::string_view{"ACE"}));

    // -- containers ----------------------------------------------------------
    // Member functions of a class template are only instantiated when used, so
    // the wrappers are exercised (not merely named) by this runtime helper.
    [[nodiscard]] std::size_t exercise_containers()
    {
        ct_cstring_view_ci_map<int> ranks;
        ranks["Ace"_ctsv]  = 14;
        ranks["King"_ctsv] = 13;

        std::size_t total = ranks.at("ACE") + *ranks.at_if("kInG");
        total += ranks.contains(std::string{"queen"}) ? 1U : 0U;
        total += ranks.count(std::string_view{"ace"});
        total += ranks.erase("Ace");
        total += static_cast<std::size_t>(ranks.lower_bound("a") != ranks.end());

        ct_cstring_view_set names;
        names.insert("Alice"_ctsv);
        total += names.contains(std::string_view{"Alice"}) ? 1U : 0U;

        ct_cstring_view_unordered_map<int> um;
        um["Bob"_ctsv] = 1;
        total += um.at(std::string{"Bob"});

        ct_wcstring_view_ci_unordered_set wide;
        wide.insert(L"Wide"_ctsv);
        total += wide.contains(std::wstring_view{L"wide"}) ? 1U : 0U;

        return total;
    }

    // -- format registration -------------------------------------------------
    // The concepts must be usable at C++20; whether std::format itself is
    // available is the header's own (feature-tested) business.
    static_assert(narrow_stream_insertable<ct_cstring_view>);
    static_assert(!insertable_via_format<ct_cstring_view>,
        "nothing is registered for format-synthesis here, and an already "
        "stream-insertable type must never be");
}

namespace cps::ct_str_smoke
{
    /// \brief Exists so the TU emits a symbol (and so the container wrappers'
    /// members are actually instantiated). Never called.
    [[nodiscard]] std::size_t cxx20_header_smoke()
    {
        return exercise_containers();
    }
}


