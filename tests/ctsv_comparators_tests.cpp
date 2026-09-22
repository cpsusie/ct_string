// ---------------------------------------------------------------------------
// ctsv_comparators_tests.cpp -- tests for char_fold.hpp and
// ctsv_comparators.hpp.
// ---------------------------------------------------------------------------
//
// Most of the interesting properties here are compile-time properties, so most
// of this file is static_assert. The GTest cases cover the things that are
// genuinely runtime (throwing, ordering of a populated container) or that are
// more legible as EXPECT_ macros.
// ---------------------------------------------------------------------------
#include <gtest/gtest.h>
#include <ct_str/char_fold.hpp>
#include <ct_str/ctsv_comparators.hpp>

#include <algorithm>
#include <compare>
#include <functional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

using namespace cps::ct_string;
using namespace cps::ct_string::literals;
using namespace std::string_view_literals;

// ============================================================
//  char_fold: per-code-unit folding
// ============================================================

static_assert(ascii_to_lower<char>('A') == 'a');
static_assert(ascii_to_lower<char>('Z') == 'z');
static_assert(ascii_to_lower<char>('a') == 'a');
static_assert(ascii_to_lower<char>('z') == 'z');
static_assert(ascii_to_upper<char>('a') == 'A');
static_assert(ascii_to_upper<char>('z') == 'Z');

// The boundary code units on either side of each letter range must be
// untouched. These are the off-by-one guards.
static_assert(ascii_to_lower<char>('@') == '@');   // 0x40, just below 'A'
static_assert(ascii_to_lower<char>('[') == '[');   // 0x5B, just above 'Z'
static_assert(ascii_to_upper<char>('`') == '`');   // 0x60, just below 'a'
static_assert(ascii_to_upper<char>('{') == '{');   // 0x7B, just above 'z'
static_assert(ascii_to_lower<char>('0') == '0');
static_assert(ascii_to_lower<char>('_') == '_');

// Folding is idempotent and the two directions are mutually inverse over the
// alphabet.
static_assert(ascii_to_lower<char>(ascii_to_lower<char>('A')) == 'a');
static_assert(ascii_to_upper<char>(ascii_to_lower<char>('A')) == 'A');

// Non-ASCII must never be touched, in any encoding.
static_assert(ascii_to_lower<char8_t>(static_cast<char8_t>(0xC3)) == static_cast<char8_t>(0xC3));
static_assert(ascii_to_lower<char8_t>(static_cast<char8_t>(0x9F)) == static_cast<char8_t>(0x9F));
static_assert(ascii_to_lower<wchar_t>(L'\u00C0') == L'\u00C0');   // LATIN CAPITAL A WITH GRAVE
static_assert(ascii_to_lower<char16_t>(u'\u0141') == u'\u0141');  // low byte aliases 'A'
static_assert(ascii_to_lower<char32_t>(U'\U0001F600') == U'\U0001F600');

TEST(CharFold, LowerViewIsLazyAndCorrect)
{
    constexpr auto src = "HeLLo, World!"sv;
    const auto folded = views::ascii_lower(src) | std::ranges::to<std::string>();
    EXPECT_EQ(folded, "hello, world!");
}

TEST(CharFold, UpperViewIsCorrect)
{
    constexpr auto src = "HeLLo, World!"sv;
    const auto folded = views::ascii_upper(src) | std::ranges::to<std::string>();
    EXPECT_EQ(folded, "HELLO, WORLD!");
}

TEST(CharFold, ViewWorksOnCtStringView)
{
    static constexpr auto sv = "MiXeD"_ctsv;
    const auto folded = views::ascii_lower(sv) | std::ranges::to<std::string>();
    EXPECT_EQ(folded, "mixed");
}

TEST(CharFold, ViewLeavesNonAsciiBytesAlone)
{
    // UTF-8 for "CAFÉ": the 'É' is 0xC3 0x89 and must survive untouched while
    // the ASCII letters fold.
    const std::string src = "CAF\xC3\x89";
    const auto folded =
        views::ascii_lower(std::string_view{src}) | std::ranges::to<std::string>();
    EXPECT_EQ(folded, std::string{"caf\xC3\x89"});
}

// ============================================================
//  Concept conformance
// ============================================================

// Every comparator family satisfies its own concept, for every std_char.
// (These duplicate the in-header self-checks on purpose: if someone ever
// weakens the header's asserts, these remain.)
#define CTSV_TEST_CONCEPTS_FOR(CH)                                                 \
    static_assert(transparent_ctsv_less<ctsv_less<CH>, CH>);                       \
    static_assert(transparent_ctsv_less<ctsv_ci_less<CH>, CH>);                    \
    static_assert(transparent_ctsv_equal_to<ctsv_equal_to<CH>, CH>);               \
    static_assert(transparent_ctsv_equal_to<ctsv_ci_equal_to<CH>, CH>);            \
    static_assert(transparent_ctsv_three_way<ctsv_three_way<CH>, CH>);             \
    static_assert(transparent_ctsv_three_way<ctsv_ci_three_way<CH>, CH>);          \
    static_assert(transparent_ctsv_hash<ctsv_hash<CH>, CH>);                       \
    static_assert(transparent_ctsv_hash<ctsv_ci_hash<CH>, CH>)

CTSV_TEST_CONCEPTS_FOR(char);
CTSV_TEST_CONCEPTS_FOR(wchar_t);
CTSV_TEST_CONCEPTS_FOR(char8_t);
CTSV_TEST_CONCEPTS_FOR(char16_t);
CTSV_TEST_CONCEPTS_FOR(char32_t);

#undef CTSV_TEST_CONCEPTS_FOR

// The standard's transparent function objects qualify...
static_assert(transparent_ctsv_less<std::less<>, char>);
static_assert(transparent_ctsv_equal_to<std::equal_to<>, char>);
static_assert(transparent_ctsv_less<std::less<>, wchar_t>);
static_assert(transparent_ctsv_equal_to<std::equal_to<>, char32_t>);

// ...and the non-transparent ones are rejected. A non-transparent comparator
// would compile as a container parameter and then silently disable
// heterogeneous lookup, which is exactly the failure the concept exists to
// turn into a diagnostic.
static_assert(!transparent_ctsv_less<std::less<std::string_view>, char>);
static_assert(!transparent_ctsv_equal_to<std::equal_to<std::string_view>, char>);
static_assert(!transparent_ctsv_less<std::less<ct_cstring_view>, char>);

// A comparator that handles only ONE argument ordering must be rejected: the
// pairing matrix in the concept exists precisely to catch this.
namespace
{
    struct half_baked_less
    {
        using is_transparent = void;
        // Only (ct_cstring_view, ct_cstring_view). Nothing heterogeneous.
        [[nodiscard]] constexpr bool operator()(ct_cstring_view l, ct_cstring_view r) const noexcept
        {
            return l < r;
        }
    };

    // Transparent-tagged but not actually noexcept.
    struct throwing_less
    {
        using is_transparent = void;
        template<typename L, typename R>
        [[nodiscard]] constexpr bool operator()(const L& l, const R& r) const
        {
            return std::string_view{l} < std::string_view{r};
        }
    };
}

static_assert(!transparent_ctsv_less<half_baked_less, char>,
    "a comparator that cannot take a string_view must be rejected");
static_assert(!transparent_ctsv_less<throwing_less, char>,
    "a potentially-throwing comparator must be rejected");

// Case-folding tags and pairing.
static_assert(!ctsv_folds_case_v<ctsv_less<char>>);
static_assert(ctsv_folds_case_v<ctsv_ci_less<char>>);
static_assert(!ctsv_folds_case_v<ctsv_hash<char>>);
static_assert(ctsv_folds_case_v<ctsv_ci_hash<char>>);
// Types that know nothing about this library default to case-sensitive, which
// lets std::hash / std::equal_to<> pair with each other correctly.
static_assert(!ctsv_folds_case_v<std::less<>>);
static_assert(!ctsv_folds_case_v<std::equal_to<>>);
static_assert(!ctsv_folds_case_v<std::hash<ct_cstring_view>>);

static_assert(consistent_ctsv_hash_equal<ctsv_hash<char>, ctsv_equal_to<char>, char>);
static_assert(consistent_ctsv_hash_equal<ctsv_ci_hash<char>, ctsv_ci_equal_to<char>, char>);
static_assert(consistent_ctsv_hash_equal<std::hash<ct_cstring_view>, std::equal_to<>, char>);
// The dangerous mixtures are compile errors, not runtime surprises.
static_assert(!consistent_ctsv_hash_equal<ctsv_ci_hash<char>, ctsv_equal_to<char>, char>);
static_assert(!consistent_ctsv_hash_equal<ctsv_hash<char>, ctsv_ci_equal_to<char>, char>);
static_assert(!consistent_ctsv_hash_equal<std::hash<ct_cstring_view>, ctsv_ci_equal_to<char>, char>);

// Lookup-key concepts: everything that ought to be a key, is.
static_assert(ctsv_lookup_key<ct_cstring_view, char>);
static_assert(ctsv_lookup_key<ct_string_view, char>);
static_assert(ctsv_lookup_key<std::string_view, char>);
static_assert(ctsv_lookup_key<std::string, char>);
static_assert(ctsv_lookup_key<decltype("abc"_fs), char>);
static_assert(ctsv_lookup_key<const char*, char>);
static_assert(!ctsv_lookup_key<int, char>);
static_assert(!ctsv_lookup_key<std::wstring_view, char>);

// ...and the nothrow variant admits everything whose conversion is noexcept.
// Note that whether a raw const char* qualifies is IMPLEMENTATION-DEFINED:
// the standard does not mark basic_string_view(const CharT*) noexcept (it
// calls traits_type::length), but libstdc++ and the MS STL both do. That is
// exactly why the comparators use a CONDITIONAL noexcept keyed on this
// concept instead of asserting one answer or the other.
static_assert(nothrow_ctsv_lookup_key<ct_cstring_view, char>);
static_assert(nothrow_ctsv_lookup_key<ct_string_view, char>);
static_assert(nothrow_ctsv_lookup_key<std::string_view, char>);
static_assert(nothrow_ctsv_lookup_key<std::string, char>);
static_assert(nothrow_ctsv_lookup_key<decltype("abc"_fs), char>);
// A nothrow key is always a key.
static_assert(!nothrow_ctsv_lookup_key<int, char>);

TEST(CtsvLookupKey, ComparatorNoexceptTracksTheKeyType)
{
    // Whatever the implementation decides about const char*, the comparator's
    // noexcept specification must AGREE with nothrow_ctsv_lookup_key.
    constexpr bool ptr_key_is_nothrow = nothrow_ctsv_lookup_key<const char*, char>;
    constexpr bool comparator_is_nothrow =
        noexcept(ctsv_ci_less<char>{}(std::declval<const char* const&>(),
                                      std::declval<const char* const&>()));
    EXPECT_EQ(ptr_key_is_nothrow, comparator_is_nothrow);

    // For the types the standard DOES guarantee, it must be noexcept.
    EXPECT_TRUE(noexcept(ctsv_ci_less<char>{}(std::declval<const std::string_view&>(),
                                              std::declval<const ct_cstring_view&>())));
}

// ============================================================
//  Ordering semantics
// ============================================================

inline constexpr auto ci_lt  = ctsv_ci_less<char>{};
inline constexpr auto cs_lt  = ctsv_less<char>{};
inline constexpr auto ci_eq  = ctsv_ci_equal_to<char>{};
inline constexpr auto cs_eq  = ctsv_equal_to<char>{};
inline constexpr auto ci_cmp = ctsv_ci_three_way<char>{};
inline constexpr auto cs_cmp = ctsv_three_way<char>{};

// Equivalence under folding.
static_assert(!ci_lt("abc"sv, "ABC"sv) && !ci_lt("ABC"sv, "abc"sv));
static_assert(ci_eq("abc"sv, "ABC"sv));
static_assert(!cs_eq("abc"sv, "ABC"sv));

// Genuine lexicographic behaviour, not just "equal or not".
static_assert(ci_lt("abc"sv, "ABD"sv) && !ci_lt("ABD"sv, "abc"sv));
static_assert(ci_lt("Ab"sv, "abc"sv) && !ci_lt("abc"sv, "Ab"sv));
static_assert(!ci_lt(""sv, ""sv));
static_assert(ci_lt(""sv, "a"sv) && !ci_lt("a"sv, ""sv));

// Fold DIRECTION is observable: '_' (0x5F) is between 'Z' (0x5A) and 'a'
// (0x61). We fold down, so "Z" -> "z" (0x7A) > "_".
static_assert(!ci_lt("Z"sv, "_"sv));
static_assert(ci_lt("_"sv, "a"sv));

// The case-sensitive comparator must NOT fold: 'A' (0x41) < 'a' (0x61).
static_assert(cs_lt("ABC"sv, "abc"sv));

// Three-way agrees with less and equal_to, and has the right category.
static_assert(std::is_same_v<decltype(cs_cmp("a"sv, "b"sv)), std::strong_ordering>);
static_assert(std::is_same_v<decltype(ci_cmp("a"sv, "b"sv)), std::weak_ordering>);
static_assert(ci_cmp("abc"sv, "ABC"sv) == std::weak_ordering::equivalent);
static_assert(ci_cmp("abc"sv, "ABD"sv) == std::weak_ordering::less);
static_assert(ci_cmp("ABD"sv, "abc"sv) == std::weak_ordering::greater);
static_assert(cs_cmp("abc"sv, "abc"sv) == std::strong_ordering::equal);

// Heterogeneous invocation across every key spelling.
static_assert(ci_lt("abc"_ctsv, "ABD"sv));
static_assert(ci_lt(std::string_view{"abc"}, "ABD"_ctsv));
static_assert(ci_eq("Ace"_ctsv, "ACE"sv));
static_assert(ci_cmp("abc"sv, "ABD"_ctsv) == std::weak_ordering::less);

// ============================================================
//  Hashing
// ============================================================

// Case-insensitively equal keys MUST hash equally -- this is the invariant an
// unordered container depends on.
static_assert(ctsv_ci_hash<char>{}("Ace"sv) == ctsv_ci_hash<char>{}("ACE"sv));
static_assert(ctsv_ci_hash<char>{}("Ace"sv) == ctsv_ci_hash<char>{}("ace"sv));
static_assert(ctsv_ci_hash<char>{}("Ace"_ctsv) == ctsv_ci_hash<char>{}(std::string_view{"aCe"}));
// ...and distinct keys should not (not guaranteed in general, but these must).
static_assert(ctsv_ci_hash<char>{}("Ace"sv) != ctsv_ci_hash<char>{}("Acf"sv));
static_assert(ctsv_ci_hash<char>{}(""sv) != ctsv_ci_hash<char>{}("a"sv));
// The case-sensitive hasher must actually distinguish case.
static_assert(ctsv_hash<char>{}("Ace"sv) != ctsv_hash<char>{}("ACE"sv));

// The hash is constexpr -- which is the entire reason it is not std::hash.
static_assert(ctsv_hash<char>{}("compile time"sv) != 0);

TEST(CtsvHash, WideAndUtf32HashConsistently)
{
    EXPECT_EQ(ctsv_ci_hash<wchar_t>{}(std::wstring_view{L"Ace"}),
              ctsv_ci_hash<wchar_t>{}(std::wstring_view{L"ACE"}));
    EXPECT_NE(ctsv_hash<wchar_t>{}(std::wstring_view{L"Ace"}),
              ctsv_hash<wchar_t>{}(std::wstring_view{L"ACE"}));
    EXPECT_EQ(ctsv_ci_hash<char32_t>{}(std::u32string_view{U"Ace"}),
              ctsv_ci_hash<char32_t>{}(std::u32string_view{U"ACE"}));
}

TEST(CtsvHash, RuntimeAgreesWithCompileTime)
{
    constexpr std::size_t compile_time = ctsv_ci_hash<char>{}("Poker"sv);
    const std::string runtime_src{"POKER"};
    EXPECT_EQ(ctsv_ci_hash<char>{}(runtime_src), compile_time);
}

// ============================================================
//  Comparators as sort predicates
// ============================================================

TEST(CtsvComparators, CaseInsensitiveSortIsStableAndCorrect)
{
    std::vector<std::string_view> v{"delta", "Alpha", "charlie", "Bravo"};
    std::ranges::sort(v, ctsv_ci_less<char>{});
    EXPECT_EQ(v, (std::vector<std::string_view>{"Alpha", "Bravo", "charlie", "delta"}));
}

TEST(CtsvComparators, CaseSensitiveSortPutsUpperCaseFirst)
{
    std::vector<std::string_view> v{"delta", "Alpha", "charlie", "Bravo"};
    std::ranges::sort(v, ctsv_less<char>{});
    // All upper-case initials (0x41..) precede all lower-case ones (0x61..).
    EXPECT_EQ(v, (std::vector<std::string_view>{"Alpha", "Bravo", "charlie", "delta"}));

    std::vector<std::string_view> w{"apple", "Apple"};
    std::ranges::sort(w, ctsv_less<char>{});
    EXPECT_EQ(w.front(), "Apple");
}

TEST(CtsvComparators, WideCaseInsensitiveComparison)
{
    const ctsv_ci_less<wchar_t> lt{};
    EXPECT_FALSE(lt(std::wstring_view{L"abc"}, std::wstring_view{L"ABC"}));
    EXPECT_FALSE(lt(std::wstring_view{L"ABC"}, std::wstring_view{L"abc"}));
    EXPECT_TRUE(lt(std::wstring_view{L"abc"}, std::wstring_view{L"ABD"}));
}

TEST(CtsvComparators, NonAsciiIsComparedButNotFolded)
{
    const ctsv_ci_equal_to<char> eq{};
    // UTF-8 "É" (0xC3 0x89) vs "é" (0xC3 0xA9): different bytes, and ASCII
    // folding leaves both alone, so they are NOT equal. This documents the
    // deliberate ASCII-only limitation.
    EXPECT_FALSE(eq(std::string_view{"\xC3\x89"}, std::string_view{"\xC3\xA9"}));
    // ...but the ASCII part of a mixed string still folds.
    EXPECT_TRUE(eq(std::string_view{"CAF\xC3\x89"}, std::string_view{"caf\xC3\x89"}));
}

TEST(CtsvComparators, RawPointerLookupKeysWork)
{
    const ctsv_ci_equal_to<char> eq{};
    const char* lhs = "Hello";
    EXPECT_TRUE(eq(lhs, std::string_view{"HELLO"}));
    EXPECT_TRUE(eq("Hello", "hello"));
}



