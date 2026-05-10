#include <gtest/gtest.h>
#include <ct_str/ct_string_view.hpp>
#include <unordered_set>
#include <unordered_map>
#include <set>
#include <map>
#include <sstream>
#include <ranges>
#include <iterator>
#include <array>
#include <span>
#include <string>
#include <string_view>
#include <functional>
#if defined(__cpp_lib_format) && (__cpp_lib_format >= 201907L)
#  include <format>
#endif

using namespace cjm::ct_string;
using namespace cjm::ct_string::literals;

// ============================================================
//  Concept / type-trait sanity checks
// ============================================================

static_assert(std_char<char>);
static_assert(std_char<wchar_t>);
static_assert(std_char<char8_t>);
static_assert(std_char<char16_t>);
static_assert(std_char<char32_t>);
static_assert(!std_char<int>);
static_assert(!std_char<unsigned char>);
static_assert(!std_char<signed char>);

static_assert(basic_fixed_string_type<basic_fixed_string<char, 6>>);
static_assert(basic_fixed_string_type<const basic_fixed_string<char, 6>>);
static_assert(!basic_fixed_string_type<int>);
static_assert(!basic_fixed_string_type<std::string_view>);

static_assert(basic_ct_string_view_type<ct_cstring_view>);
static_assert(basic_ct_string_view_type<ct_string_view>);
static_assert(basic_ct_string_view_type<const ct_cstring_view>);
static_assert(!basic_ct_string_view_type<std::string_view>);

// ============================================================
//  basic_fixed_string tests
// ============================================================

// --- Construction & size ---

TEST(BasicFixedString, SizeAndLength)
{
    static constexpr auto fs = "hello"_fs;
    static_assert(fs.size() == 5);
    static_assert(fs.length() == 5);
    static_assert(fs.buff_size() == 6);   // includes null terminator
    static_assert(fs.buff_length() == 6);
    EXPECT_EQ(fs.size(), 5u);
    EXPECT_EQ(fs.length(), 5u);
    EXPECT_EQ(fs.buff_size(), 6u);
}

TEST(BasicFixedString, EmptyString)
{
    static constexpr basic_fixed_string<char, 1> fs{};
    static_assert(fs.size() == 0);
    static_assert(fs.empty());
    static_assert(fs.buff_size() == 1);
    EXPECT_TRUE(fs.empty());
    EXPECT_EQ(fs.size(), 0u);
    EXPECT_EQ(fs.data()[0], '\0');
}

TEST(BasicFixedString, ValidCstr)
{
    static constexpr auto fs = "hello"_fs;
    static_assert(fs.valid_cstr());
    EXPECT_TRUE(fs.valid_cstr());
}

TEST(BasicFixedString, NullTerminatorInBuffer)
{
    static constexpr auto fs = "hello"_fs;
    EXPECT_EQ(fs.data()[fs.size()], '\0');
    static_assert(fs.data()[fs.size()] == '\0');
}

TEST(BasicFixedString, DeductionGuideFromArray)
{
    static constexpr basic_fixed_string fs{"deduced"};
    static_assert(std::is_same_v<decltype(fs)::char_type, char>);
    static_assert(fs.size() == 7);
    EXPECT_EQ(fs.size(), 7u);
}

TEST(BasicFixedString, DeductionGuideFromWideArray)
{
    static constexpr basic_fixed_string fs{L"wide"};
    static_assert(std::is_same_v<decltype(fs)::char_type, wchar_t>);
    static_assert(fs.size() == 4);
    EXPECT_EQ(fs.size(), 4u);
}

// --- Element access ---

TEST(BasicFixedString, SubscriptOperator)
{
    static constexpr auto fs = "hello"_fs;
    static_assert(fs[0] == 'h');
    static_assert(fs[4] == 'o');
    EXPECT_EQ(fs[0], 'h');
    EXPECT_EQ(fs[4], 'o');
}

TEST(BasicFixedString, AtInBounds)
{
    static constexpr auto fs = "hello"_fs;
    EXPECT_EQ(fs.at(0), 'h');
    EXPECT_EQ(fs.at(4), 'o');
}

TEST(BasicFixedString, AtOutOfBounds)
{
    static constexpr auto fs = "hello"_fs;
    EXPECT_THROW((void)fs.at(99), std::out_of_range);
}

TEST(BasicFixedString, FrontAndBack)
{
    static constexpr auto fs = "hello"_fs;
    static_assert(fs.front() == 'h');
    static_assert(fs.back() == 'o');
    EXPECT_EQ(fs.front(), 'h');
    EXPECT_EQ(fs.back(), 'o');
}

TEST(BasicFixedString, Data)
{
    static constexpr auto fs = "hello"_fs;
    EXPECT_EQ(std::string_view(fs.data(), fs.size()), "hello");
}

TEST(BasicFixedString, GetStdSv)
{
    static constexpr auto fs = "hello"_fs;
    static constexpr auto sv = fs.get_std_sv();
    static_assert(sv == "hello");
    static_assert(sv.size() == 5);
    EXPECT_EQ(sv, "hello");
}

// --- Iterators ---

TEST(BasicFixedString, ForwardIteration)
{
    static constexpr auto fs = "abc"_fs;
    std::string result;
    for (char c : fs) result += c;
    EXPECT_EQ(result, "abc");
}

TEST(BasicFixedString, ReverseIteration)
{
    static constexpr auto fs = "abc"_fs;
    std::string result;
    for (auto it = fs.rbegin(); it != fs.rend(); ++it) result += *it;
    EXPECT_EQ(result, "cba");
}

TEST(BasicFixedString, CIterators)
{
    static constexpr auto fs = "abc"_fs;
    EXPECT_EQ(std::distance(fs.cbegin(), fs.cend()), 3);
    EXPECT_EQ(std::distance(fs.crbegin(), fs.crend()), 3);
}

TEST(BasicFixedString, RangesCompatible)
{
    static constexpr auto fs = "hello"_fs;
    auto upper = fs | std::views::transform([](char c){ return static_cast<char>(std::toupper(c)); });
    std::string result(upper.begin(), upper.end());
    EXPECT_EQ(result, "HELLO");
}

// --- Conversions ---

TEST(BasicFixedString, ImplicitConversionToStringView)
{
    static constexpr auto fs = "hello"_fs;
    std::string_view sv = fs;
    EXPECT_EQ(sv, "hello");
    EXPECT_EQ(sv.size(), 5u);
}

TEST(BasicFixedString, ExplicitConversionToString)
{
    static constexpr auto fs = "hello"_fs;
    auto s = static_cast<std::string>(fs);
    EXPECT_EQ(s, "hello");
    EXPECT_EQ(s.size(), 5u);
}

// --- Comparison: same size ---

TEST(BasicFixedString, EqualitySameSize)
{
    static constexpr auto a = "hello"_fs;
    static constexpr auto b = "hello"_fs;
    static constexpr auto c = "world"_fs;
    static_assert(a == b);
    static_assert(a != c);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(BasicFixedString, SpaceshipSameSize)
{
    static constexpr auto x = "abc"_fs;
    static constexpr auto y = "abd"_fs;
    static_assert((x <=> y) < 0);
    static_assert((y <=> x) > 0);
    static_assert((x <=> x) == 0);
    EXPECT_LT(x, y);
    EXPECT_GT(y, x);
}

// --- Comparison: different size ---

TEST(BasicFixedString, EqualityDifferentSize)
{
    static constexpr auto a = "hi"_fs;
    static constexpr auto b = "hello"_fs;
    static_assert(a != b);
    EXPECT_NE(a, b);
    EXPECT_FALSE(a == b);
}

TEST(BasicFixedString, SpaceshipDifferentSize)
{
    static constexpr auto a = "abc"_fs;
    static constexpr auto b = "abcd"_fs;
    static_assert((a <=> b) < 0);
    static_assert((b <=> a) > 0);
    EXPECT_LT(a, b);
    EXPECT_GT(b, a);
}

// --- Comparison: against string_view / const char* ---

TEST(BasicFixedString, EqualityAgainstStringView)
{
    static constexpr auto fs = "hello"_fs;
    EXPECT_EQ(fs, std::string_view{"hello"});
    EXPECT_NE(fs, std::string_view{"world"});
}

TEST(BasicFixedString, SpaceshipAgainstStringView)
{
    static constexpr auto fs = "hello"_fs;
    EXPECT_EQ(fs <=> std::string_view{"hello"}, std::strong_ordering::equal);
    EXPECT_LT(fs, std::string_view{"world"});
}

// --- Concatenation ---

TEST(BasicFixedString, Concatenation)
{
    static constexpr auto a = "hello"_fs;
    static constexpr auto b = " world"_fs;
    static constexpr auto c = a + b;
    static_assert(c.size() == 11);
    static_assert(c.buff_size() == 12);
    static_assert(c == "hello world");
    static_assert(c.valid_cstr());
    EXPECT_EQ(c.size(), 11u);
    EXPECT_EQ(c, std::string_view{"hello world"});
}

TEST(BasicFixedString, ConcatenationPreservesNullTerminator)
{
    static constexpr auto a = "foo"_fs;
    static constexpr auto b = "bar"_fs;
    static constexpr auto c = a + b;
    static_assert(c.data()[c.size()] == '\0');
    EXPECT_EQ(c.data()[c.size()], '\0');
}

TEST(BasicFixedString, ConcatenationWithEmpty)
{
    static constexpr auto a = "hello"_fs;
    static constexpr basic_fixed_string<char, 1> empty{};
    static constexpr auto c = a + empty;
    static_assert(c == "hello");
    static_assert(c.size() == 5);
    EXPECT_EQ(c, std::string_view{"hello"});
}

TEST(BasicFixedString, ConcatenationEmptyWithEmpty)
{
    static constexpr basic_fixed_string<char, 1> e1{};
    static constexpr basic_fixed_string<char, 1> e2{};
    static constexpr auto c = e1 + e2;
    static_assert(c.empty());
    static_assert(c.size() == 0);
    EXPECT_TRUE(c.empty());
}

TEST(BasicFixedString, ConcatenationChained)
{
    static constexpr auto a = "foo"_fs;
    static constexpr auto b = "bar"_fs;
    static constexpr auto c = "baz"_fs;
    static constexpr auto result = a + b + c;
    static_assert(result == "foobarbaz");
    static_assert(result.size() == 9);
    EXPECT_EQ(result, std::string_view{"foobarbaz"});
}

// --- operator<< ---

TEST(BasicFixedString, StreamOutputNarrow)
{
    static constexpr auto fs = "hello"_fs;
    std::ostringstream oss;
    oss << fs;
    EXPECT_EQ(oss.str(), "hello");
}

TEST(BasicFixedString, StreamOutputWide)
{
    static constexpr basic_fixed_string fs{L"wide"};
    std::wostringstream oss;
    oss << fs;
    EXPECT_EQ(oss.str(), L"wide");
}

// --- Other char types ---

TEST(BasicFixedString, Char8Type)
{
    static constexpr basic_fixed_string fs{u8"utf8"};
    static_assert(std::is_same_v<decltype(fs)::char_type, char8_t>);
    static_assert(fs.size() == 4);
    EXPECT_EQ(fs.get_std_sv(), std::u8string_view{u8"utf8"});
}

TEST(BasicFixedString, Char16Type)
{
    static constexpr basic_fixed_string fs{u"utf16"};
    static_assert(std::is_same_v<decltype(fs)::char_type, char16_t>);
    static_assert(fs.size() == 5);
    EXPECT_EQ(fs.get_std_sv(), std::u16string_view{u"utf16"});
}

TEST(BasicFixedString, Char32Type)
{
    static constexpr basic_fixed_string fs{U"utf32"};
    static_assert(std::is_same_v<decltype(fs)::char_type, char32_t>);
    static_assert(fs.size() == 5);
    EXPECT_EQ(fs.get_std_sv(), std::u32string_view{U"utf32"});
}

// ============================================================
//  basic_ct_string_view tests
// ============================================================

// --- Type aliases sanity ---

static_assert(std::is_same_v<ct_cstring_view::char_type, char>);
static_assert(std::is_same_v<ct_wcstring_view::char_type, wchar_t>);
static_assert(std::is_same_v<ct_u8cstring_view::char_type, char8_t>);
static_assert(std::is_same_v<ct_u16cstring_view::char_type, char16_t>);
static_assert(std::is_same_v<ct_u32cstring_view::char_type, char32_t>);
static_assert(ct_cstring_view::known_cstr == true);
static_assert(ct_string_view::known_cstr == false);

// --- Construction & known_cstr ---

TEST(CtStringView, KnownCstrFlagTrue)
{
    static constexpr auto sv = "hello"_ctsv;
    static_assert(sv.known_cstr == true);
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(sv)>, ct_cstring_view>);
    EXPECT_TRUE(sv.known_cstr);
}

TEST(CtStringView, DefaultConstructKnownCstr)
{
    static constexpr ct_cstring_view sv;
    static_assert(sv.empty());
    static_assert(sv.size() == 0);
    EXPECT_EQ(sv.size(), 0u);
    EXPECT_TRUE(sv.empty());
    EXPECT_EQ(sv.c_str()[0], '\0');
}

TEST(CtStringView, DefaultConstructNonCstr)
{
    static constexpr ct_string_view sv;
    static_assert(sv.empty());
    EXPECT_TRUE(sv.empty());
    EXPECT_EQ(sv.size(), 0u);
}

TEST(CtStringView, MakeCtsv)
{
    static constexpr auto sv = make_ctsv<"hello">();
    static_assert(sv.size() == 5);
    static_assert(sv == "hello");
    static_assert(sv.known_cstr);
    EXPECT_EQ(sv.size(), 5u);
    EXPECT_EQ(sv, "hello");
}

TEST(CtStringView, LiteralAndMakeCtsvAgree)
{
    static constexpr auto a = "hello"_ctsv;
    static constexpr auto b = make_ctsv<"hello">();
    static_assert(a == b);
    EXPECT_EQ(a, b);
}

// --- Size / capacity ---

TEST(CtStringView, SizeAndLength)
{
    static constexpr auto sv = "hello"_ctsv;
    static_assert(sv.size() == 5);
    static_assert(sv.length() == 5);
    static_assert(!sv.empty());
    EXPECT_EQ(sv.size(), 5u);
    EXPECT_EQ(sv.length(), 5u);
    EXPECT_FALSE(sv.empty());
}

TEST(CtStringView, MaxSizeMatchesStringView)
{
    static constexpr auto sv = "hello"_ctsv;
    EXPECT_EQ(sv.max_size(), std::string_view{}.max_size());
}

// --- Element access ---

TEST(CtStringView, SubscriptOperator)
{
    static constexpr auto sv = "hello"_ctsv;
    static_assert(sv[0] == 'h');
    static_assert(sv[4] == 'o');
    EXPECT_EQ(sv[0], 'h');
    EXPECT_EQ(sv[4], 'o');
}

TEST(CtStringView, AtInBounds)
{
    static constexpr auto sv = "hello"_ctsv;
    EXPECT_EQ(sv.at(0), 'h');
    EXPECT_EQ(sv.at(4), 'o');
}

TEST(CtStringView, AtOutOfBounds)
{
    static constexpr auto sv = "hello"_ctsv;
    EXPECT_THROW((void)sv.at(5), std::out_of_range);
    EXPECT_THROW((void)sv.at(99), std::out_of_range);
}

TEST(CtStringView, FrontAndBack)
{
    static constexpr auto sv = "hello"_ctsv;
    static_assert(sv.front() == 'h');
    static_assert(sv.back() == 'o');
    EXPECT_EQ(sv.front(), 'h');
    EXPECT_EQ(sv.back(), 'o');
}

TEST(CtStringView, CStr)
{
    static constexpr auto sv = "hello"_ctsv;
    EXPECT_STREQ(sv.c_str(), "hello");
    EXPECT_EQ(sv.c_str()[sv.size()], '\0');
}

TEST(CtStringView, DataMatchesCStr)
{
    static constexpr auto sv = "hello"_ctsv;
    EXPECT_EQ(sv.data(), sv.c_str());
}

// --- Conversions ---

TEST(CtStringView, ImplicitConversionToStringView)
{
    static constexpr auto sv = "hello"_ctsv;
    std::string_view stdSv = sv;
    EXPECT_EQ(stdSv, "hello");
    EXPECT_EQ(stdSv.size(), 5u);
}

TEST(CtStringView, ToStringMethod)
{
    static constexpr auto sv = "hello"_ctsv;
    auto s = sv.to_string();
    EXPECT_EQ(s, "hello");
    static_assert(std::is_same_v<decltype(s), std::string>);
}

// --- Iterators ---

TEST(CtStringView, ForwardIteration)
{
    static constexpr auto sv = "abc"_ctsv;
    std::string result;
    for (char c : sv) result += c;
    EXPECT_EQ(result, "abc");
}

TEST(CtStringView, ReverseIteration)
{
    static constexpr auto sv = "abc"_ctsv;
    std::string result;
    for (auto it = sv.rbegin(); it != sv.rend(); ++it) result += *it;
    EXPECT_EQ(result, "cba");
}

TEST(CtStringView, RangesView)
{
    static_assert(std::ranges::view<ct_cstring_view>);
    static_assert(std::ranges::view<ct_string_view>);
    static_assert(std::ranges::borrowed_range<ct_cstring_view>);
    static_assert(std::ranges::borrowed_range<ct_string_view>);
    static_assert(std::ranges::random_access_range<ct_cstring_view>);
    static_assert(std::ranges::contiguous_range<ct_cstring_view>);

    static constexpr auto sv = "hello"_ctsv;
    auto upper = sv | std::views::transform([](char c){ return static_cast<char>(std::toupper(c)); });
    std::string result(upper.begin(), upper.end());
    EXPECT_EQ(result, "HELLO");
}

// --- Comparison: same type ---

TEST(CtStringView, EqualitySameType)
{
    static constexpr auto a = "hello"_ctsv;
    static constexpr auto b = "hello"_ctsv;
    static constexpr auto c = "world"_ctsv;
    static_assert(a == b);
    static_assert(a != c);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(CtStringView, SpaceshipSameType)
{
    static constexpr auto a = "apple"_ctsv;
    static constexpr auto b = "banana"_ctsv;
    EXPECT_LT(a, b);
    EXPECT_GT(b, a);
    EXPECT_EQ(a <=> a, std::strong_ordering::equal);
}

// --- Comparison: against string_view / const char* ---

TEST(CtStringView, EqualityAgainstStringView)
{
    static constexpr auto sv = "hello"_ctsv;
    EXPECT_EQ(sv, std::string_view{"hello"});
    EXPECT_NE(sv, std::string_view{"world"});
}

TEST(CtStringView, EqualityAgainstCharPtrLiteral)
{
    static constexpr auto sv = "hello"_ctsv;
    EXPECT_EQ(sv, "hello");
    EXPECT_NE(sv, "world");
}

TEST(CtStringView, SpaceshipAgainstStringView)
{
    static constexpr auto sv = "hello"_ctsv;
    EXPECT_LT(sv, std::string_view{"world"});
    EXPECT_EQ(sv <=> std::string_view{"hello"}, std::strong_ordering::equal);
}

// --- Comparison: cross-flavor (known_cstr vs non) ---

TEST(CtStringView, CrossFlavorEquality)
{
    static constexpr auto cstr_sv = "hello"_ctsv;          // known_cstr == true
    static constexpr ct_string_view non_cstr_sv = cstr_sv;  // converting ctor
    EXPECT_EQ(cstr_sv, non_cstr_sv);
    EXPECT_EQ(non_cstr_sv, cstr_sv);
}

TEST(CtStringView, CrossFlavorSpaceship)
{
    static constexpr auto cstr_sv = "apple"_ctsv;
    static constexpr ct_string_view non_cstr_sv = "banana"_ctsv;
    EXPECT_EQ(cstr_sv <=> non_cstr_sv, std::strong_ordering::less);
    EXPECT_EQ(non_cstr_sv <=> cstr_sv, std::strong_ordering::greater);
}

// --- Converting construction/assignment (true -> false) ---

TEST(CtStringView, ConvertingConstructorCstrToNonCstr)
{
    static constexpr auto cstr_sv = "hello"_ctsv;
    static constexpr ct_string_view non_cstr{cstr_sv};
    static_assert(non_cstr == "hello");
    EXPECT_EQ(non_cstr, "hello");
}

TEST(CtStringView, ConvertingAssignmentCstrToNonCstr)
{
    static constexpr auto cstr_sv = "hello"_ctsv;
    ct_string_view non_cstr;
    non_cstr = cstr_sv;
    EXPECT_EQ(non_cstr, "hello");
    EXPECT_EQ(non_cstr.size(), 5u);
}

// Converse direction (non_cstr -> cstr) must NOT be constructible.
static_assert(!std::is_constructible_v<ct_cstring_view, ct_string_view>);
// Same-flavor copy/move are OK.
static_assert(std::is_copy_constructible_v<ct_cstring_view>);
static_assert(std::is_copy_assignable_v<ct_cstring_view>);
static_assert(std::is_nothrow_copy_constructible_v<ct_cstring_view>);
static_assert(std::is_nothrow_move_constructible_v<ct_cstring_view>);

// remove_suffix available on the non-cstr flavor.
static_assert(requires (ct_string_view v) { v.remove_suffix(1); });

// c_str available on the known_cstr flavor.
static_assert(requires (ct_cstring_view v) { v.c_str(); });

// (Negative `requires`-expression checks for the inverse cases trigger an MSVC
//  bug where a constrained-member failure is treated as a hard error rather
//  than SFINAE, so they're omitted here.)

// --- remove_prefix ---

TEST(CtStringView, RemovePrefixKnownCstr)
{
    auto sv = "hello"_ctsv;
    sv.remove_prefix(2);
    EXPECT_EQ(sv, "llo");
    EXPECT_EQ(sv.size(), 3u);
    // null-termination preserved
    EXPECT_EQ(sv.c_str()[sv.size()], '\0');
    EXPECT_STREQ(sv.c_str(), "llo");
}

TEST(CtStringView, RemovePrefixZero)
{
    auto sv = "hello"_ctsv;
    sv.remove_prefix(0);
    EXPECT_EQ(sv, "hello");
}

TEST(CtStringView, RemovePrefixAll)
{
    auto sv = "hello"_ctsv;
    sv.remove_prefix(sv.size());
    EXPECT_TRUE(sv.empty());
    EXPECT_EQ(sv.c_str()[0], '\0');
}

TEST(CtStringView, RemovePrefixNonCstr)
{
    ct_string_view sv = "hello"_ctsv;
    sv.remove_prefix(2);
    EXPECT_EQ(sv, "llo");
}

// --- remove_suffix (only on non-cstr flavor) ---

TEST(CtStringView, RemoveSuffixNonCstr)
{
    ct_string_view sv = "hello"_ctsv;
    sv.remove_suffix(2);
    EXPECT_EQ(sv, "hel");
    EXPECT_EQ(sv.size(), 3u);
}

// --- swap ---

TEST(CtStringView, Swap)
{
    auto a = "hello"_ctsv;
    auto b = "world"_ctsv;
    a.swap(b);
    EXPECT_EQ(a, "world");
    EXPECT_EQ(b, "hello");
}

TEST(CtStringView, StdSwap)
{
    auto a = "hello"_ctsv;
    auto b = "world"_ctsv;
    using std::swap;
    swap(a, b);
    EXPECT_EQ(a, "world");
    EXPECT_EQ(b, "hello");
}

// --- substr (tail only, preserves known_cstr) ---

TEST(CtStringView, SubstrTail)
{
    static constexpr auto sv = "hello"_ctsv;
    auto tail = sv.substr(2);
    static_assert(std::is_same_v<decltype(tail), ct_cstring_view>);
    EXPECT_EQ(tail, "llo");
    EXPECT_EQ(tail.c_str()[tail.size()], '\0');
    EXPECT_STREQ(tail.c_str(), "llo");
}

TEST(CtStringView, SubstrTailDefaultPos)
{
    static constexpr auto sv = "hello"_ctsv;
    auto full = sv.substr();
    EXPECT_EQ(full, "hello");
}

TEST(CtStringView, SubstrTailFromEnd)
{
    static constexpr auto sv = "hello"_ctsv;
    auto empty = sv.substr(sv.size());
    EXPECT_TRUE(empty.empty());
    EXPECT_EQ(empty.c_str()[0], '\0');
}

TEST(CtStringView, SubstrResultNotCstring)
{
    static constexpr auto sv = "hello"_ctsv;
    static_assert(sv.known_cstr);
    static constexpr auto part = sv.substr(1U, 3U);
    using part_type = std::remove_cvref_t<decltype(part)>;
    static_assert(!part.known_cstr);
    static_assert(std::same_as<ct_string_view, part_type>);
    static_assert(part == "ell");
    EXPECT_EQ("ell", part);
    ct_string_view part_copy = part;
    part_copy.remove_suffix(1);
    EXPECT_EQ("el", part_copy);

}

// --- copy ---

TEST(CtStringView, Copy)
{
    static constexpr auto sv = "hello"_ctsv;
    char buf[6]{};
    auto n = sv.copy(buf, 3, 1);  // copies "ell"
    EXPECT_EQ(n, 3u);
    EXPECT_EQ(std::string_view(buf, 3), "ell");
}

TEST(CtStringView, CopyTruncated)
{
    static constexpr auto sv = "hi"_ctsv;
    char buf[8]{};
    auto n = sv.copy(buf, 100);
    EXPECT_EQ(n, 2u);
    EXPECT_EQ(std::string_view(buf, n), "hi");
}

// --- starts_with / ends_with / contains ---

TEST(CtStringView, StartsWith)
{
    static constexpr auto sv = "hello world"_ctsv;
    EXPECT_TRUE(sv.starts_with(std::string_view{"hello"}));
    EXPECT_TRUE(sv.starts_with('h'));
    EXPECT_TRUE(sv.starts_with("hello"));
    EXPECT_FALSE(sv.starts_with(std::string_view{"world"}));
    EXPECT_FALSE(sv.starts_with('x'));
}

TEST(CtStringView, EndsWith)
{
    static constexpr auto sv = "hello world"_ctsv;
    EXPECT_TRUE(sv.ends_with(std::string_view{"world"}));
    EXPECT_TRUE(sv.ends_with('d'));
    EXPECT_TRUE(sv.ends_with("world"));
    EXPECT_FALSE(sv.ends_with(std::string_view{"hello"}));
}

TEST(CtStringView, Contains)
{
    static constexpr auto sv = "hello world"_ctsv;
    EXPECT_TRUE(sv.contains(std::string_view{"lo wo"}));
    EXPECT_TRUE(sv.contains('w'));
    EXPECT_TRUE(sv.contains("lo wo"));
    EXPECT_FALSE(sv.contains(std::string_view{"xyz"}));
    EXPECT_FALSE(sv.contains('z'));
}

// --- find / rfind ---

TEST(CtStringView, Find)
{
    static constexpr auto sv = "hello world"_ctsv;
    EXPECT_EQ(sv.find(std::string_view{"world"}), 6u);
    EXPECT_EQ(sv.find('o'), 4u);
    EXPECT_EQ(sv.find('o', 5), 7u);
    EXPECT_EQ(sv.find("xyz"), ct_cstring_view::npos);
    EXPECT_EQ(sv.find("ell", 0, 3), 1u);
}

TEST(CtStringView, Rfind)
{
    static constexpr auto sv = "hello world"_ctsv;
    EXPECT_EQ(sv.rfind('o'), 7u);
    EXPECT_EQ(sv.rfind(std::string_view{"lo"}), 3u);
    EXPECT_EQ(sv.rfind("xyz"), ct_cstring_view::npos);
}

// --- find_first_of / find_last_of ---

TEST(CtStringView, FindFirstOf)
{
    static constexpr auto sv = "hello world"_ctsv;
    EXPECT_EQ(sv.find_first_of(std::string_view{"aeiou"}), 1u);  // 'e'
    EXPECT_EQ(sv.find_first_of('o'), 4u);
    EXPECT_EQ(sv.find_first_of("xyz"), ct_cstring_view::npos);
}

TEST(CtStringView, FindLastOf)
{
    static constexpr auto sv = "hello world"_ctsv;
    EXPECT_EQ(sv.find_last_of(std::string_view{"aeiou"}), 7u);  // 'o' in "world"
    EXPECT_EQ(sv.find_last_of('h'), 0u);
}

// --- find_first_not_of / find_last_not_of ---

TEST(CtStringView, FindFirstNotOf)
{
    static constexpr auto sv = "aaabbb"_ctsv;
    EXPECT_EQ(sv.find_first_not_of('a'), 3u);
    EXPECT_EQ(sv.find_first_not_of(std::string_view{"ab"}), ct_cstring_view::npos);
}

TEST(CtStringView, FindLastNotOf)
{
    static constexpr auto sv = "aaabbb"_ctsv;
    EXPECT_EQ(sv.find_last_not_of('b'), 2u);
    EXPECT_EQ(sv.find_last_not_of(std::string_view{"ab"}), ct_cstring_view::npos);
}

// --- compare ---

TEST(CtStringView, CompareWithStringView)
{
    static constexpr auto sv = "hello"_ctsv;
    EXPECT_EQ(sv.compare(std::string_view{"hello"}), 0);
    EXPECT_LT(sv.compare(std::string_view{"world"}), 0);
    EXPECT_GT(sv.compare(std::string_view{"apple"}), 0);
}

TEST(CtStringView, CompareWithCharPtr)
{
    static constexpr auto sv = "hello"_ctsv;
    EXPECT_EQ(sv.compare("hello"), 0);
    EXPECT_LT(sv.compare("world"), 0);
    EXPECT_GT(sv.compare("apple"), 0);
}

TEST(CtStringView, CompareSubstring)
{
    static constexpr auto sv = "hello world"_ctsv;
    EXPECT_EQ(sv.compare(0, 5, std::string_view{"hello"}), 0);
    EXPECT_EQ(sv.compare(6, 5, std::string_view{"world"}), 0);
    EXPECT_NE(sv.compare(0, 5, std::string_view{"world"}), 0);
}

TEST(CtStringView, CompareSubstringAgainstSubstring)
{
    static constexpr auto sv = "hello world"_ctsv;
    EXPECT_EQ(sv.compare(0, 5, std::string_view{"xxhelloxx"}, 2, 5), 0);
}

TEST(CtStringView, CompareSubstringWithCharPtr)
{
    static constexpr auto sv = "hello world"_ctsv;
    EXPECT_EQ(sv.compare(0, 5, "hello"), 0);
    EXPECT_EQ(sv.compare(6, 5, "worldXX", 5), 0);
}

// --- Hash ---

TEST(CtStringView, HashMatchesStringViewHash)
{
    static constexpr auto sv = "hello"_ctsv;
    const std::string_view stdSv{"hello"};
    EXPECT_EQ(std::hash<ct_cstring_view>{}(sv), std::hash<std::string_view>{}(stdSv));
}

TEST(CtStringView, HashIsTransparent)
{
    // The hash specialization declares `is_transparent`.
    static_assert(requires { typename std::hash<ct_cstring_view>::is_transparent; });
}

TEST(CtStringView, UsableInUnorderedSet)
{
    std::unordered_set<ct_cstring_view> s;
    static constexpr auto a = "hello"_ctsv;
    static constexpr auto b = "world"_ctsv;
    s.insert(a);
    s.insert(b);
    s.insert(a);  // duplicate
    EXPECT_EQ(s.size(), 2u);
    EXPECT_TRUE(s.contains(a));
    EXPECT_TRUE(s.contains(b));
}

TEST(CtStringView, UsableAsUnorderedMapKey)
{
    std::unordered_map<ct_cstring_view, int> m;
    static constexpr auto k1 = "one"_ctsv;
    static constexpr auto k2 = "two"_ctsv;
    m[k1] = 1;
    m[k2] = 2;
    EXPECT_EQ(m.at(k1), 1);
    EXPECT_EQ(m.at(k2), 2);
}

// --- Other char types ---

TEST(CtStringView, WideCharLiteral)
{
    static constexpr auto wv = make_ctsv<L"wide">();
    static_assert(wv.size() == 4);
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(wv)>, ct_wcstring_view>);
    EXPECT_EQ(wv.size(), 4u);
    EXPECT_EQ(static_cast<std::wstring_view>(wv), std::wstring_view{L"wide"});
}

TEST(CtStringView, Char8Literal)
{
    static constexpr auto v = make_ctsv<u8"utf8">();
    static_assert(v.size() == 4);
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(v)>, ct_u8cstring_view>);
    EXPECT_EQ(v.size(), 4u);
}

TEST(CtStringView, Char16Literal)
{
    static constexpr auto v = make_ctsv<u"utf16">();
    static_assert(v.size() == 5);
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(v)>, ct_u16cstring_view>);
    EXPECT_EQ(v.size(), 5u);
}

TEST(CtStringView, Char32Literal)
{
    static constexpr auto v = make_ctsv<U"utf32">();
    static_assert(v.size() == 5);
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(v)>, ct_u32cstring_view>);
    EXPECT_EQ(v.size(), 5u);
}

// --- Borrowed range usage (return iterator from temporary) ---

TEST(CtStringView, BorrowedRangeFindIf)
{
    auto make_sv = []() { return "hello"_ctsv; };
    // Because the type is enable_borrowed_range, dangling iterators do NOT result.
    auto it = std::ranges::find(make_sv(), 'l');
    static_assert(!std::is_same_v<decltype(it), std::ranges::dangling>);
    EXPECT_NE(it, std::ranges::end(make_sv()));
}

// ============================================================
//  Heterogeneous-lookup-friendly container aliases
// ============================================================

// -- Sanity: the alias templates produce containers parameterized with
// transparent function objects.

static_assert(std::is_same_v<
    ct_cstring_view_set::key_compare, std::less<>>);
static_assert(std::is_same_v<
    ct_cstring_view_map<int>::key_compare, std::less<>>);
static_assert(std::is_same_v<
    ct_cstring_view_unordered_set::key_equal, std::equal_to<>>);
static_assert(std::is_same_v<
    ct_cstring_view_unordered_map<int>::key_equal, std::equal_to<>>);

// std::less<> and std::equal_to<> are themselves transparent.
static_assert(requires { typename std::less<>::is_transparent; });
static_assert(requires { typename std::equal_to<>::is_transparent; });

// -- std::set / std::map heterogeneous lookup via std::less<> --

TEST(CtSvContainerAliases, OrderedSetHeterogeneousLookupWithStringView)
{
    ct_cstring_view_set s;
    s.insert("hello"_ctsv);
    s.insert("world"_ctsv);

    // find() / contains() / count() with std::string_view (a different type)
    EXPECT_NE(s.find(std::string_view{"hello"}), s.end());
    EXPECT_TRUE(s.contains(std::string_view{"world"}));
    EXPECT_EQ(s.count(std::string_view{"hello"}), 1u);
    EXPECT_EQ(s.find(std::string_view{"missing"}), s.end());
}

TEST(CtSvContainerAliases, OrderedSetHeterogeneousLookupWithString)
{
    ct_cstring_view_set s;
    s.insert("hello"_ctsv);

    const std::string key{"hello"};
    EXPECT_NE(s.find(key), s.end());
    EXPECT_TRUE(s.contains(key));
}

TEST(CtSvContainerAliases, OrderedSetHeterogeneousLookupWithFixedString)
{
    ct_cstring_view_set s;
    s.insert("hello"_ctsv);

    static constexpr auto fs = "hello"_fs;  // basic_fixed_string
    EXPECT_NE(s.find(fs), s.end());
    EXPECT_TRUE(s.contains(fs));
}

TEST(CtSvContainerAliases, OrderedSetHeterogeneousLookupCrossFlavor)
{
    ct_cstring_view_set s;
    s.insert("hello"_ctsv);

    // Look up using the OPPOSITE flavor (non-known_cstr) of the same chars.
    ct_string_view non_cstr_key = "hello"_ctsv;
    EXPECT_NE(s.find(non_cstr_key), s.end());
    EXPECT_TRUE(s.contains(non_cstr_key));
}

TEST(CtSvContainerAliases, OrderedMapHeterogeneousLookup)
{
    ct_cstring_view_map<int> m;
    m.emplace("alpha"_ctsv, 1);
    m.emplace("beta"_ctsv, 2);

    auto it = m.find(std::string_view{"beta"});
    ASSERT_NE(it, m.end());
    EXPECT_EQ(it->second, 2);

    EXPECT_TRUE(m.contains(std::string{"alpha"}));
    EXPECT_FALSE(m.contains(std::string_view{"gamma"}));
}

// -- std::unordered_set / std::unordered_map heterogeneous lookup --

TEST(CtSvContainerAliases, UnorderedSetHeterogeneousLookupWithStringView)
{
    ct_cstring_view_unordered_set s;
    s.insert("hello"_ctsv);
    s.insert("world"_ctsv);

    auto it = s.find(std::string_view{"hello"});
    EXPECT_NE(it, s.end());
    EXPECT_TRUE(s.contains(std::string_view{"world"}));
    EXPECT_EQ(s.find(std::string_view{"missing"}), s.end());
}

TEST(CtSvContainerAliases, UnorderedSetHeterogeneousLookupWithString)
{
    ct_cstring_view_unordered_set s;
    s.insert("hello"_ctsv);
    const std::string key{"hello"};
    EXPECT_NE(s.find(key), s.end());
    EXPECT_TRUE(s.contains(key));
}

TEST(CtSvContainerAliases, UnorderedSetHeterogeneousLookupWithFixedString)
{
    ct_cstring_view_unordered_set s;
    s.insert("hello"_ctsv);
    static constexpr auto fs = "hello"_fs;
    EXPECT_NE(s.find(fs), s.end());
    EXPECT_TRUE(s.contains(fs));
}

TEST(CtSvContainerAliases, UnorderedSetHeterogeneousLookupCrossFlavor)
{
    ct_cstring_view_unordered_set s;
    s.insert("hello"_ctsv);

    ct_string_view non_cstr_key = "hello"_ctsv;
    EXPECT_NE(s.find(non_cstr_key), s.end());
    EXPECT_TRUE(s.contains(non_cstr_key));
}

TEST(CtSvContainerAliases, UnorderedMapHeterogeneousLookup)
{
    ct_cstring_view_unordered_map<int> m;
    m.emplace("alpha"_ctsv, 1);
    m.emplace("beta"_ctsv, 2);

    auto it = m.find(std::string_view{"beta"});
    ASSERT_NE(it, m.end());
    EXPECT_EQ(it->second, 2);

    EXPECT_TRUE(m.contains(std::string{"alpha"}));
    EXPECT_FALSE(m.contains(std::string_view{"gamma"}));
}

// -- A heterogeneous lookup must hash-compare-equal: different lookup-key
//    types that represent the same string must locate the same bucket.

TEST(CtSvContainerAliases, UnorderedSetHashConsistencyAcrossKeyTypes)
{
    ct_cstring_view_unordered_set s;
    s.insert("hello"_ctsv);

    EXPECT_NE(s.find(std::string_view{"hello"}), s.end());
    EXPECT_NE(s.find(std::string{"hello"}),     s.end());
    static constexpr auto fs = "hello"_fs;
    EXPECT_NE(s.find(fs),                        s.end());
}

// -- Non-known_cstr flavor variants compile and lookup the same way.

TEST(CtSvContainerAliases, NonCstrFlavorOrderedAndUnorderedSetLookup)
{
    ct_string_view_set os;
    os.insert(ct_string_view{"hello"_ctsv});
    EXPECT_TRUE(os.contains(std::string_view{"hello"}));

    ct_string_view_unordered_set us;
    us.insert(ct_string_view{"hello"_ctsv});
    EXPECT_TRUE(us.contains(std::string_view{"hello"}));
}

// -- Wide-char alias variants compile.

TEST(CtSvContainerAliases, WideAliasesCompileAndLookup)
{
    ct_wcstring_view_set ws;
    ws.insert(make_ctsv<L"wide">());
    EXPECT_TRUE(ws.contains(std::wstring_view{L"wide"}));

    ct_wcstring_view_unordered_set wus;
    wus.insert(make_ctsv<L"wide">());
    EXPECT_TRUE(wus.contains(std::wstring_view{L"wide"}));
}

// ============================================================
//  std::formatter
// ============================================================

#if defined(__cpp_lib_format) && (__cpp_lib_format >= 201907L)

TEST(CtSvFormatter, BasicNarrow)
{
    static constexpr auto sv = "hello"_ctsv;
    EXPECT_EQ(std::format("{}", sv), "hello");
}

TEST(CtSvFormatter, NarrowWidthAndAlign)
{
    static constexpr auto sv = "hi"_ctsv;
    EXPECT_EQ(std::format("[{:>5}]", sv), "[   hi]");
    EXPECT_EQ(std::format("[{:<5}]", sv), "[hi   ]");
    EXPECT_EQ(std::format("[{:^6}]", sv), "[  hi  ]");
}

TEST(CtSvFormatter, NarrowPrecisionTruncates)
{
    static constexpr auto sv = "hello"_ctsv;
    EXPECT_EQ(std::format("{:.3}", sv), "hel");
}

TEST(CtSvFormatter, NarrowFillCharacter)
{
    static constexpr auto sv = "hi"_ctsv;
    EXPECT_EQ(std::format("[{:*>5}]", sv), "[***hi]");
}

TEST(CtSvFormatter, NarrowNonCstrFlavor)
{
    static constexpr auto src = "hello world"_ctsv;
    static constexpr auto part = src.substr(0, 5);  // ct_string_view
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(part)>, ct_string_view>);
    EXPECT_EQ(std::format("{}", part), "hello");
}

TEST(CtSvFormatter, WideBasic)
{
    static constexpr auto wv = make_ctsv<L"wide">();
    EXPECT_EQ(std::format(L"{}", wv), std::wstring{L"wide"});
}

TEST(CtSvFormatter, WideWidthAlignPrecision)
{
    static constexpr auto wv = make_ctsv<L"hello">();
    EXPECT_EQ(std::format(L"[{:>8}]", wv), std::wstring{L"[   hello]"});
    EXPECT_EQ(std::format(L"{:.3}",   wv), std::wstring{L"hel"});
}

#endif // __cpp_lib_format

// ============================================================
//  fmt::formatter (only if {fmt} is available)
// ============================================================

#if defined(CJM_CT_STRING_VIEW_HAS_FMT)

TEST(CtSvFmtFormatter, BasicNarrow)
{
    static constexpr auto sv = "hello"_ctsv;
    EXPECT_EQ(fmt::format("{}", sv), "hello");
}

TEST(CtSvFmtFormatter, NarrowWidthAlignPrecision)
{
    static constexpr auto sv = "hi"_ctsv;
    EXPECT_EQ(fmt::format("[{:>5}]", sv), "[   hi]");
    EXPECT_EQ(fmt::format("{:.3}",   "hello"_ctsv), "hel");
}

TEST(CtSvFmtFormatter, NarrowNonCstrFlavor)
{
    static constexpr auto src  = "hello world"_ctsv;
    static constexpr auto part = src.substr(0, 5);  // ct_string_view
    EXPECT_EQ(fmt::format("{}", part), "hello");
}

#endif // CJM_CT_STRING_VIEW_HAS_FMT


