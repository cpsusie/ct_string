// ---------------------------------------------------------------------------
// ctsv_containers_tests.cpp -- tests for the basic_ctsv_* container wrappers.
// ---------------------------------------------------------------------------
//
// The central claim of ctsv_containers.hpp is an ASYMMETRY:
//
//     inserting  requires a real basic_ct_string_view
//     looking up accepts anything convertible to std::basic_string_view
//
// Both halves are tested: the positive half with ordinary GTest cases, the
// negative half with static_asserts over requires-expressions (a "this must
// not compile" test that itself compiles).
// ---------------------------------------------------------------------------
#include <gtest/gtest.h>
#include <ct_str/ctsv_containers.hpp>

#include <algorithm>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace cps::ct_string;
using namespace cps::ct_string::literals;
using namespace std::string_view_literals;

// ============================================================
//  Structural guarantees
// ============================================================

// "This must NOT compile" is expressed through NAMED CONCEPT TEMPLATES rather
// than through ad-hoc requires-expressions over concrete types. The difference
// matters: substituting into a concept's template parameters puts the
// requirement in the immediate context, so a failure is an unsatisfied
// constraint. An ad-hoc `requires (ct_cstring_view_map<int> m) { m[sv]; }`
// over already-concrete types is a hard error on both GCC 14 and Clang 19.
namespace
{
    template<typename TMap, typename TKey>
    concept subscriptable_with = requires (TMap& m, const TKey& k) { m[k]; };

    template<typename TContainer, typename TKey>
    concept insertable_with = requires (TContainer& c, const TKey& k) { c.insert(k); };

    template<typename TMap, typename TKey, typename TValue>
    concept emplaceable_with =
        requires (TMap& m, const TKey& k, const TValue& v) { m.emplace(k, v); };

    template<typename TMap, typename TKey>
    concept at_able_with = requires (const TMap& m, const TKey& k) { m.at(k); };

    template<typename TMap, typename TKey>
    concept at_if_able_with = requires (const TMap& m, const TKey& k) { m.at_if(k); };

    template<typename TContainer, typename TKey>
    concept erasable_with = requires (TContainer& c, const TKey& k) { c.erase(k); };
}

// The key type is not constructible from a string_view. THIS is what makes
// every insertion path safe -- not a hand-written check that could be
// forgotten, but the type system.
static_assert(!std::constructible_from<ct_cstring_view_map<int>::key_type, std::string_view>);
static_assert(!std::constructible_from<ct_cstring_view_map<int>::key_type, std::string>);
static_assert(!std::constructible_from<ct_cstring_view_map<int>::key_type, const char*>);

// Dropping the C-string guarantee is safe, forging it is not.
static_assert(std::constructible_from<ct_string_view, ct_cstring_view>);
static_assert(!std::constructible_from<ct_cstring_view, ct_string_view>);

// operator[] must accept a ct view and REJECT every non-ct-view spelling,
// because operator[] INSERTS and an inserted key must carry the lifetime
// guarantee.
static_assert(subscriptable_with<ct_cstring_view_map<int>, ct_cstring_view>);
static_assert(!subscriptable_with<ct_cstring_view_map<int>, std::string_view>);
static_assert(!subscriptable_with<ct_cstring_view_map<int>, std::string>);
static_assert(!subscriptable_with<ct_cstring_view_map<int>, const char*>);
static_assert(subscriptable_with<ct_cstring_view_unordered_map<int>, ct_cstring_view>);
static_assert(!subscriptable_with<ct_cstring_view_unordered_map<int>, std::string_view>);

// The non-C-string flavor's operator[] additionally accepts the other flavor,
// because ct_string_view IS constructible from ct_cstring_view. The
// C-string flavor's does not, because the reverse would forge a guarantee.
static_assert(subscriptable_with<ct_string_view_map<int>, ct_string_view>);
static_assert(subscriptable_with<ct_string_view_map<int>, ct_cstring_view>);
static_assert(!subscriptable_with<ct_cstring_view_map<int>, ct_string_view>);

// insert / emplace are gated the same way.
static_assert(insertable_with<ct_cstring_view_set, ct_cstring_view>);
static_assert(!insertable_with<ct_cstring_view_set, std::string_view>);
static_assert(!insertable_with<ct_cstring_view_set, std::string>);
static_assert(insertable_with<ct_cstring_view_unordered_set, ct_cstring_view>);
static_assert(!insertable_with<ct_cstring_view_unordered_set, std::string_view>);
static_assert(!emplaceable_with<ct_cstring_view_map<int>, std::string_view, int>);

// ...while at / at_if / erase accept every lookup spelling.
static_assert(at_able_with<ct_cstring_view_map<int>, std::string_view>);
static_assert(at_able_with<ct_cstring_view_map<int>, std::string>);
static_assert(at_able_with<ct_cstring_view_map<int>, const char*>);
static_assert(at_able_with<ct_cstring_view_map<int>, ct_cstring_view>);
static_assert(at_able_with<ct_cstring_view_map<int>, ct_string_view>);
static_assert(at_able_with<ct_cstring_view_map<int>, decltype("k"_fs)>);
static_assert(at_if_able_with<ct_cstring_view_map<int>, std::string_view>);
static_assert(at_if_able_with<ct_cstring_view_unordered_map<int>, const char*>);
static_assert(erasable_with<ct_cstring_view_map<int>, std::string_view>);
static_assert(erasable_with<ct_cstring_view_set, std::string>);
static_assert(erasable_with<ct_cstring_view_unordered_set, const char*>);

// A set has no mapped value, so it has no at/at_if at all.
static_assert(!at_able_with<ct_cstring_view_set, std::string_view>);

// at_if is the non-throwing at: pointer in, pointer out, const-correct.
static_assert(std::is_same_v<decltype(std::declval<ct_cstring_view_map<int>&>().at_if("k")), int*>);
static_assert(std::is_same_v<
    decltype(std::declval<const ct_cstring_view_map<int>&>().at_if("k")), const int*>);

// Range concepts: ordered containers are bidirectional, unordered are forward.
// The unordered containers expose their underlying std::unordered_* iterators
// verbatim, and the standard only mandates *forward* iterators there. Whether
// they are additionally bidirectional is unspecified and does vary: libstdc++
// models forward only, while the MSVC STL's bucket-list iterators are
// bidirectional. So only the guaranteed (forward) half is asserted here.
static_assert(std::ranges::bidirectional_range<ct_cstring_view_set>);
static_assert(std::ranges::bidirectional_range<ct_cstring_view_map<int>>);
static_assert(std::ranges::forward_range<ct_cstring_view_unordered_set>);
static_assert(std::ranges::forward_range<ct_cstring_view_unordered_map<int>>);

// Container-level introspection of the case policy.
static_assert(!ct_cstring_view_unordered_map<int>::folds_case);
static_assert(ct_cstring_view_ci_unordered_map<int>::folds_case);
static_assert(ct_cstring_view_map<int>::is_cstring);
static_assert(!ct_string_view_map<int>::is_cstring);

// A hash/equality case mismatch must be rejected. The container's
// requires-clause is spelled in terms of this concept, so asserting the
// concept asserts the container. (Naming the ill-formed specialization
// directly would be a hard error, not a substitution failure.)
static_assert(!consistent_ctsv_hash_equal<ctsv_ci_hash<char>, ctsv_equal_to<char>, char>);
static_assert(!consistent_ctsv_hash_equal<ctsv_hash<char>, ctsv_ci_equal_to<char>, char>);
static_assert(consistent_ctsv_hash_equal<ctsv_ci_hash<char>, ctsv_ci_equal_to<char>, char>);

// ============================================================
//  Ordered map
// ============================================================

TEST(CtsvMap, HeterogeneousAtAndAtIf)
{
    ct_cstring_view_map<int> m;
    m["alpha"_ctsv] = 1;
    m["beta"_ctsv]  = 2;

    EXPECT_EQ(m.at(std::string_view{"alpha"}), 1);
    EXPECT_EQ(m.at(std::string{"beta"}), 2);
    EXPECT_EQ(m.at("alpha"), 1);
    EXPECT_EQ(m.at("beta"_ctsv), 2);

    ASSERT_NE(m.at_if("alpha"), nullptr);
    EXPECT_EQ(*m.at_if("alpha"), 1);
    EXPECT_EQ(m.at_if("gamma"), nullptr);
}

TEST(CtsvMap, AtThrowsOutOfRangeWithTheKeyInTheMessage)
{
    ct_cstring_view_map<int> m;
    m["alpha"_ctsv] = 1;

    try
    {
        (void)m.at("gamma");
        FAIL() << "at() must throw for an absent key";
    }
    catch (const std::out_of_range& e)
    {
        EXPECT_NE(std::string_view{e.what()}.find("gamma"), std::string_view::npos);
    }
}

TEST(CtsvMap, FindContainsCountAndErase)
{
    ct_cstring_view_map<int> m;
    m["alpha"_ctsv] = 1;
    m["beta"_ctsv]  = 2;

    EXPECT_NE(m.find(std::string_view{"beta"}), m.end());
    EXPECT_EQ(m.find("missing"), m.end());
    EXPECT_TRUE(m.contains(std::string{"alpha"}));
    EXPECT_FALSE(m.contains("gamma"));
    EXPECT_EQ(m.count("alpha"), 1u);
    EXPECT_EQ(m.count("gamma"), 0u);

    EXPECT_EQ(m.erase(std::string_view{"alpha"}), 1u);
    EXPECT_EQ(m.erase("alpha"), 0u);
    EXPECT_EQ(m.size(), 1u);
}

TEST(CtsvMap, MutationThroughAtAndAtIf)
{
    ct_cstring_view_map<int> m;
    m["k"_ctsv] = 1;
    m.at("k") = 42;
    EXPECT_EQ(m.at("k"), 42);
    *m.at_if("k") = 7;
    EXPECT_EQ(m.at("k"), 7);
}

TEST(CtsvMap, BoundsAndEqualRange)
{
    ct_cstring_view_map<int> m;
    m["a"_ctsv] = 1;
    m["c"_ctsv] = 3;
    m["e"_ctsv] = 5;

    EXPECT_EQ(m.lower_bound("c")->second, 3);
    EXPECT_EQ(m.upper_bound("c")->second, 5);
    EXPECT_EQ(m.lower_bound("b")->second, 3);
    const auto [lo, hi] = m.equal_range("c");
    EXPECT_EQ(std::distance(lo, hi), 1);
}

TEST(CtsvMap, CaseInsensitiveLookupAndOrdering)
{
    ct_cstring_view_ci_map<int> m;
    m["Delta"_ctsv]   = 4;
    m["alpha"_ctsv]   = 1;
    m["Charlie"_ctsv] = 3;
    m["bravo"_ctsv]   = 2;

    EXPECT_EQ(m.at("ALPHA"), 1);
    EXPECT_EQ(m.at("BrAvO"), 2);
    EXPECT_EQ(m.at(std::string{"charlie"}), 3);
    EXPECT_TRUE(m.contains("DELTA"));

    // Iteration order is the case-insensitive ordering.
    std::vector<int> order;
    for (const auto& [k, v] : m) { order.push_back(v); (void)k; }
    EXPECT_EQ(order, (std::vector<int>{1, 2, 3, 4}));
}

TEST(CtsvMap, CaseInsensitiveTreatsDifferentCasesAsOneKey)
{
    ct_cstring_view_ci_map<int> m;
    m["Ace"_ctsv] = 1;
    m["ACE"_ctsv] = 2;   // same key under ci ordering -- overwrites
    EXPECT_EQ(m.size(), 1u);
    EXPECT_EQ(m.at("ace"), 2);
}

TEST(CtsvMap, CaseSensitiveKeepsThemDistinct)
{
    ct_cstring_view_map<int> m;
    m["Ace"_ctsv] = 1;
    m["ACE"_ctsv] = 2;
    EXPECT_EQ(m.size(), 2u);
    EXPECT_EQ(m.at("Ace"), 1);
    EXPECT_EQ(m.at("ACE"), 2);
    EXPECT_EQ(m.at_if("ace"), nullptr);
}

TEST(CtsvMap, InitializerListConstructionAndEquality)
{
    const ct_cstring_view_map<int> a{{"x"_ctsv, 1}, {"y"_ctsv, 2}};
    const ct_cstring_view_map<int> b{{"y"_ctsv, 2}, {"x"_ctsv, 1}};
    EXPECT_EQ(a, b);
    EXPECT_EQ(a.size(), 2u);
    EXPECT_EQ(a.at("x"), 1);
}

TEST(CtsvMap, BaseEscapeHatchExposesTheStdContainer)
{
    ct_cstring_view_map<int> m;
    m["k"_ctsv] = 1;
    const std::map<ct_cstring_view, int, ctsv_less<char>>& raw = m.base();
    EXPECT_EQ(raw.size(), 1u);
}

// ============================================================
//  Ordered set
// ============================================================

TEST(CtsvSet, HeterogeneousLookupAcrossEveryKeySpelling)
{
    ct_cstring_view_set s;
    s.insert("hello"_ctsv);
    s.insert("world"_ctsv);

    EXPECT_NE(s.find(std::string_view{"hello"}), s.end());
    EXPECT_NE(s.find(std::string{"hello"}), s.end());
    EXPECT_NE(s.find("hello"), s.end());
    static constexpr auto fs = "hello"_fs;
    EXPECT_NE(s.find(fs), s.end());
    // The opposite (non-known_cstr) flavor of the same characters.
    const ct_string_view other_flavor = "hello"_ctsv;
    EXPECT_NE(s.find(other_flavor), s.end());

    EXPECT_EQ(s.find("missing"), s.end());
    EXPECT_TRUE(s.contains("world"));
    EXPECT_EQ(s.count("hello"), 1u);
}

TEST(CtsvSet, EraseByEveryKeySpelling)
{
    ct_cstring_view_set s;
    s.insert("a"_ctsv);
    s.insert("b"_ctsv);
    s.insert("c"_ctsv);

    EXPECT_EQ(s.erase(std::string_view{"a"}), 1u);
    EXPECT_EQ(s.erase(std::string{"b"}), 1u);
    EXPECT_EQ(s.erase("c"), 1u);
    EXPECT_EQ(s.erase("c"), 0u);
    EXPECT_TRUE(s.empty());
}

TEST(CtsvSet, CaseInsensitiveSet)
{
    ct_cstring_view_ci_set s;
    s.insert("Alice"_ctsv);
    s.insert("BOB"_ctsv);

    EXPECT_TRUE(s.contains("ALICE"));
    EXPECT_TRUE(s.contains(std::string{"bob"}));
    EXPECT_EQ(s.erase("alice"), 1u);
    EXPECT_EQ(s.size(), 1u);

    // Re-inserting a different casing is a no-op: it is the same key.
    s.insert("bob"_ctsv);
    EXPECT_EQ(s.size(), 1u);
}

TEST(CtsvSet, ReverseIterationIsAvailableOnOrderedContainers)
{
    ct_cstring_view_set s;
    s.insert("a"_ctsv);
    s.insert("b"_ctsv);
    s.insert("c"_ctsv);

    std::vector<std::string> back;
    for (auto it = s.rbegin(); it != s.rend(); ++it) { back.emplace_back(std::string_view{*it}); }
    EXPECT_EQ(back, (std::vector<std::string>{"c", "b", "a"}));
}

TEST(CtsvSet, NonCstrFlavorWorks)
{
    ct_string_view_set s;
    s.insert(ct_string_view{"hello"_ctsv});
    EXPECT_TRUE(s.contains(std::string_view{"hello"}));
    EXPECT_EQ(s.erase("hello"), 1u);
}

// ============================================================
//  Unordered map
// ============================================================

TEST(CtsvUnorderedMap, HeterogeneousAtAndAtIf)
{
    ct_cstring_view_unordered_map<int> m;
    m["alpha"_ctsv] = 1;
    m["beta"_ctsv]  = 2;

    EXPECT_EQ(m.at(std::string_view{"alpha"}), 1);
    EXPECT_EQ(m.at(std::string{"beta"}), 2);
    EXPECT_EQ(m.at("alpha"), 1);
    ASSERT_NE(m.at_if("beta"), nullptr);
    EXPECT_EQ(*m.at_if("beta"), 2);
    EXPECT_EQ(m.at_if("gamma"), nullptr);
    EXPECT_THROW((void)m.at("gamma"), std::out_of_range);
}

TEST(CtsvUnorderedMap, FindContainsCountAndErase)
{
    ct_cstring_view_unordered_map<int> m;
    m["alpha"_ctsv] = 1;

    EXPECT_NE(m.find(std::string_view{"alpha"}), m.end());
    EXPECT_EQ(m.find("missing"), m.end());
    EXPECT_TRUE(m.contains(std::string{"alpha"}));
    EXPECT_EQ(m.count("alpha"), 1u);
    EXPECT_EQ(m.erase("alpha"), 1u);
    EXPECT_EQ(m.erase("alpha"), 0u);
    EXPECT_TRUE(m.empty());
}

TEST(CtsvUnorderedMap, CaseInsensitiveLookup)
{
    ct_cstring_view_ci_unordered_map<int> m;
    m["Ace"_ctsv]  = 14;
    m["King"_ctsv] = 13;

    EXPECT_EQ(m.at("ACE"), 14);
    EXPECT_EQ(m.at(std::string{"ace"}), 14);
    EXPECT_EQ(m.at(std::string_view{"aCe"}), 14);
    EXPECT_EQ(*m.at_if("kInG"), 13);
    EXPECT_TRUE(m.contains("KING"));
    EXPECT_EQ(m.erase("AcE"), 1u);
    EXPECT_EQ(m.size(), 1u);
}

TEST(CtsvUnorderedMap, CaseInsensitiveCollapsesCasings)
{
    ct_cstring_view_ci_unordered_map<int> m;
    m["Ace"_ctsv] = 1;
    m["ACE"_ctsv] = 2;
    EXPECT_EQ(m.size(), 1u);
    EXPECT_EQ(m.at("ace"), 2);
}

TEST(CtsvUnorderedMap, HashAndEqualityAgreeAcrossLookupKeyTypes)
{
    // Different lookup-key spellings of the same characters must land in the
    // same bucket AND compare equal. If the hasher and the comparator ever
    // disagree, this is the test that catches it.
    ct_cstring_view_ci_unordered_map<int> m;
    m["Hello"_ctsv] = 1;

    EXPECT_NE(m.find(std::string_view{"HELLO"}), m.end());
    EXPECT_NE(m.find(std::string{"hello"}), m.end());
    EXPECT_NE(m.find("hElLo"), m.end());
    static constexpr auto fs = "HELLO"_fs;
    EXPECT_NE(m.find(fs), m.end());
    const ct_string_view other_flavor = "hello"_ctsv;
    EXPECT_NE(m.find(other_flavor), m.end());
}

TEST(CtsvUnorderedMap, BucketInterfaceIsForwarded)
{
    ct_cstring_view_unordered_map<int> m;
    m.reserve(64);
    m["k"_ctsv] = 1;
    EXPECT_GE(m.bucket_count(), 1u);
    EXPECT_LE(m.load_factor(), m.max_load_factor());
    EXPECT_EQ(m.bucket_size(m.bucket("k"_ctsv)), 1u);
}

TEST(CtsvUnorderedMap, SurvivesRehashWithManyCasings)
{
    // Grow past the initial bucket count so a rehash definitely happens, then
    // confirm every key is still reachable by a differently-cased lookup.
    ct_cstring_view_ci_unordered_map<int> m;
    m["Alpha"_ctsv]   = 1;
    m["Bravo"_ctsv]   = 2;
    m["Charlie"_ctsv] = 3;
    m["Delta"_ctsv]   = 4;
    m["Echo"_ctsv]    = 5;
    m.rehash(256);

    EXPECT_EQ(m.at("ALPHA"), 1);
    EXPECT_EQ(m.at("bravo"), 2);
    EXPECT_EQ(m.at("ChArLiE"), 3);
    EXPECT_EQ(m.at("DELTA"), 4);
    EXPECT_EQ(m.at("echo"), 5);
    EXPECT_EQ(m.size(), 5u);
}

// ============================================================
//  Unordered set
// ============================================================

TEST(CtsvUnorderedSet, HeterogeneousLookupAcrossEveryKeySpelling)
{
    ct_cstring_view_unordered_set s;
    s.insert("hello"_ctsv);
    s.insert("world"_ctsv);

    EXPECT_NE(s.find(std::string_view{"hello"}), s.end());
    EXPECT_NE(s.find(std::string{"hello"}), s.end());
    EXPECT_NE(s.find("hello"), s.end());
    static constexpr auto fs = "hello"_fs;
    EXPECT_NE(s.find(fs), s.end());
    const ct_string_view other_flavor = "hello"_ctsv;
    EXPECT_NE(s.find(other_flavor), s.end());
    EXPECT_EQ(s.find("missing"), s.end());
}

TEST(CtsvUnorderedSet, CaseSensitiveKeepsCasingsDistinct)
{
    ct_cstring_view_unordered_set s;
    s.insert("World"_ctsv);
    EXPECT_TRUE(s.contains("World"));
    EXPECT_FALSE(s.contains("world"));
    s.insert("world"_ctsv);
    EXPECT_EQ(s.size(), 2u);
}

TEST(CtsvUnorderedSet, CaseInsensitiveCollapsesCasings)
{
    ct_cstring_view_ci_unordered_set s;
    s.insert("World"_ctsv);
    s.insert("WORLD"_ctsv);
    EXPECT_EQ(s.size(), 1u);
    EXPECT_TRUE(s.contains("world"));
    EXPECT_EQ(s.erase(std::string{"wOrLd"}), 1u);
    EXPECT_TRUE(s.empty());
}

TEST(CtsvUnorderedSet, NonCstrFlavorWorks)
{
    ct_string_view_unordered_set s;
    s.insert(ct_string_view{"hello"_ctsv});
    EXPECT_TRUE(s.contains(std::string_view{"hello"}));
}

// ============================================================
//  Wide-character instantiations
// ============================================================

TEST(CtsvWideContainers, OrderedAndUnorderedWideLookup)
{
    ct_wcstring_view_set ws;
    ws.insert(make_ctsv<L"wide">());
    EXPECT_TRUE(ws.contains(std::wstring_view{L"wide"}));
    EXPECT_TRUE(ws.contains(std::wstring{L"wide"}));
    EXPECT_TRUE(ws.contains(L"wide"));

    ct_wcstring_view_unordered_set wus;
    wus.insert(make_ctsv<L"wide">());
    EXPECT_TRUE(wus.contains(std::wstring_view{L"wide"}));
}

TEST(CtsvWideContainers, CaseInsensitiveWideMap)
{
    ct_wcstring_view_ci_map<int> m;
    m[make_ctsv<L"Ace">()] = 14;
    EXPECT_EQ(m.at(std::wstring_view{L"ACE"}), 14);
    EXPECT_EQ(m.at(L"ace"), 14);

    ct_wcstring_view_ci_unordered_map<int> um;
    um[make_ctsv<L"King">()] = 13;
    EXPECT_EQ(um.at(std::wstring_view{L"KING"}), 13);
}

// ============================================================
//  Explicit instantiation of every wrapper
// ============================================================
//
// Forwarded members of a class template are only checked when instantiated.
// Explicit instantiation checks all of them at once, which is how the draft
// version of this wrapper's attempt to forward std::unordered_map::rbegin was
// found.

template class cps::ct_string::basic_ctsv_map<char, true, int>;
template class cps::ct_string::basic_ctsv_map<char, false, int, ctsv_ci_less<char>>;
template class cps::ct_string::basic_ctsv_map<char8_t, true, int>;
template class cps::ct_string::basic_ctsv_set<char, true>;
template class cps::ct_string::basic_ctsv_set<wchar_t, false, ctsv_ci_less<wchar_t>>;
template class cps::ct_string::basic_ctsv_set<char16_t, true, ctsv_ci_less<char16_t>>;
template class cps::ct_string::basic_ctsv_unordered_map<char, true, int>;
template class cps::ct_string::basic_ctsv_unordered_map<char, true, int,
    ctsv_ci_hash<char>, ctsv_ci_equal_to<char>>;
template class cps::ct_string::basic_ctsv_unordered_map<char, true, int,
    std::hash<ct_cstring_view>, std::equal_to<>>;
template class cps::ct_string::basic_ctsv_unordered_set<char, false>;
template class cps::ct_string::basic_ctsv_unordered_set<char32_t, true,
    ctsv_ci_hash<char32_t>, ctsv_ci_equal_to<char32_t>>;


