// ---------------------------------------------------------------------------
// cxx20_comparison_checks.cpp -- what the two class templates compare with.
// ---------------------------------------------------------------------------
//
// basic_ct_string_view and basic_fixed_string compare with string literals,
// with arrays of and pointers to their own character type, with
// std::basic_string_view, std::basic_string, and with each other (decision
// D-030). For all five character types and both flavors of view, this TU
// checks at compile time that:
//   * every such comparison compiles, in both operand orders, with ==, !=, <
//     and <=>, and gives the right answer wherever the operand can be a
//     constant (std::basic_string cannot be one on GCC 11, so the test suite
//     checks its answers at run time);
//   * every such comparison is noexcept, whatever the standard library;
//   * comparing with another character type, with nullptr, or with an operand
//     whose conversion may throw, does not compile.
//
// It also checks that nullptr, and a literal 0, are rejected by the
// comparators and hashers of ctsv_comparators.hpp, by the std::hash
// specialization, and by every lookup member of the containers in
// ctsv_containers.hpp, while every other key still works (decision D-032).
//
// Like cxx20_header_smoke.cpp, this TU is part of the C++20-pinned smoke
// target, so it needs no test framework and runs wherever that target builds.
// Before T3, the literal, array and pointer checks failed with libc++, whose
// std::basic_string_view(const CharT*) is not noexcept (FINDINGS F-5). C++20
// is also where std::basic_string_view still converts from nullptr.
// ---------------------------------------------------------------------------
#include <ct_str/ct_string_view.hpp>
#include <ct_str/ctsv_comparators.hpp>
#include <ct_str/ctsv_containers.hpp>
#include <ct_str/fixed_string.hpp>

#include <compare>
#include <concepts>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>

// The same ASCII text, as a literal of character type TChar.
#define CPS_CMP_LIT(TChar, text) pick<TChar>(text, L##text, u8##text, u##text, U##text)

namespace
{
    using namespace cps::ct_string;

    /// Returns whichever of its arguments has character type TChar. The result
    /// is an lvalue of type const TChar[N], exactly like a string literal.
    template<std_char TChar, std::size_t N>
    constexpr auto& pick(const char (&c)[N], const wchar_t (&w)[N], const char8_t (&u8)[N],
                         const char16_t (&u16)[N], const char32_t (&u32)[N]) noexcept
    {
        if constexpr (std::same_as<TChar, char>) { return c; }
        else if constexpr (std::same_as<TChar, wchar_t>) { return w; }
        else if constexpr (std::same_as<TChar, char8_t>) { return u8; }
        else if constexpr (std::same_as<TChar, char16_t>) { return u16; }
        else { return u32; }
    }

    /// A copy of a literal in a plain array, so that the array can be const or not.
    template<std_char TChar, std::size_t N>
    struct char_buffer
    {
        TChar chars[N];
    };

    template<std_char TChar, std::size_t N>
    constexpr char_buffer<TChar, N> buffer_of(const TChar (&text)[N]) noexcept
    {
        char_buffer<TChar, N> result{};
        for (std::size_t i = 0; i < N; ++i)
        {
            result.chars[i] = text[i];
        }
        return result;
    }

    /// The two texts every check uses, "abc" < "abd", in character type TChar.
    template<std_char TChar>
    struct texts
    {
        static constexpr auto& lo_lit = CPS_CMP_LIT(TChar, "abc");
        static constexpr auto& hi_lit = CPS_CMP_LIT(TChar, "abd");
        static constexpr auto lo_fs = basic_fixed_string{lo_lit};
        static constexpr auto hi_fs = basic_fixed_string{hi_lit};
        static constexpr basic_ct_string_view<TChar, true> lo_cview = make_ctsv<lo_fs>();
        static constexpr basic_ct_string_view<TChar, true> hi_cview = make_ctsv<hi_fs>();
        static constexpr basic_ct_string_view<TChar, false> lo_view = lo_cview;
        static constexpr basic_ct_string_view<TChar, false> hi_view = hi_cview;
    };

    // -- every comparison gives the right answer -------------------------------

    /// True iff \p subject equals \p lo and is less than \p hi, by every operator,
    /// in both operand orders.
    template<typename TSubject, typename TLo, typename THi>
    constexpr bool compares_correctly(const TSubject& subject, TLo& lo, THi& hi) noexcept
    {
        return subject == lo && lo == subject && !(subject != lo) && !(lo != subject)
            && subject != hi && hi != subject && !(subject == hi) && !(hi == subject)
            && subject < hi && hi > subject && !(subject < lo) && !(lo < subject)
            && subject <= lo && lo <= subject && subject >= lo && lo >= subject
            && (subject <=> lo) == 0 && (lo <=> subject) == 0
            && (subject <=> hi) < 0 && (hi <=> subject) > 0;
    }

    /// Checks \p subject, whose text is "abc", against every operand kind that
    /// can be a constant: a literal, a const and a non-const array, a pointer to
    /// const and to non-const, a std::basic_string_view, and both flavors of
    /// view and the fixed string (each compared with the other class template).
    template<std_char TChar, typename TSubject>
    constexpr bool compares_with_every_operand(const TSubject& subject) noexcept
    {
        using t = texts<TChar>;
        const auto lo_const = buffer_of(t::lo_lit);
        const auto hi_const = buffer_of(t::hi_lit);
        auto lo_mutable = buffer_of(t::lo_lit);
        auto hi_mutable = buffer_of(t::hi_lit);
        const TChar* const lo_cptr = lo_const.chars;
        const TChar* const hi_cptr = hi_const.chars;
        TChar* const lo_ptr = lo_mutable.chars;
        TChar* const hi_ptr = hi_mutable.chars;
        const std::basic_string_view<TChar> lo_sv = t::lo_lit;
        const std::basic_string_view<TChar> hi_sv = t::hi_lit;

        return compares_correctly(subject, t::lo_lit, t::hi_lit)
            && compares_correctly(subject, lo_const.chars, hi_const.chars)
            && compares_correctly(subject, lo_mutable.chars, hi_mutable.chars)
            && compares_correctly(subject, lo_cptr, hi_cptr)
            && compares_correctly(subject, lo_ptr, hi_ptr)
            && compares_correctly(subject, lo_sv, hi_sv)
            && compares_correctly(subject, t::lo_fs, t::hi_fs)
            && compares_correctly(subject, t::lo_cview, t::hi_cview)
            && compares_correctly(subject, t::lo_view, t::hi_view);
    }

    template<std_char TChar>
    constexpr bool all_subjects_compare_correctly() noexcept
    {
        using t = texts<TChar>;
        return compares_with_every_operand<TChar>(t::lo_cview)
            && compares_with_every_operand<TChar>(t::lo_view)
            && compares_with_every_operand<TChar>(t::lo_fs);
    }

    static_assert(all_subjects_compare_correctly<char>());
    static_assert(all_subjects_compare_correctly<wchar_t>());
    static_assert(all_subjects_compare_correctly<char8_t>());
    static_assert(all_subjects_compare_correctly<char16_t>());
    static_assert(all_subjects_compare_correctly<char32_t>());

    // -- every comparison is noexcept ------------------------------------------
    // L and R are the operand types, as references where the value category
    // matters (an array must stay an array).

    template<typename L, typename R>
    concept nothrow_comparable_one_way = requires (L l, R r)
    {
        { l == r } noexcept -> std::same_as<bool>;
        { l != r } noexcept -> std::same_as<bool>;
        { l < r } noexcept -> std::same_as<bool>;
        { l <=> r } noexcept -> std::same_as<std::strong_ordering>;
    };

    template<typename L, typename R>
    concept nothrow_comparable = nothrow_comparable_one_way<L, R> && nothrow_comparable_one_way<R, L>;

    template<typename TSubject, typename TChar>
    concept nothrow_comparable_with_every_operand =
        nothrow_comparable<const TSubject&, const TChar (&)[4]>         // a literal, or a const array
        && nothrow_comparable<const TSubject&, TChar (&)[4]>            // a non-const array
        && nothrow_comparable<const TSubject&, TChar (&)[]>             // an array of unknown bound
        && nothrow_comparable<const TSubject&, const TChar*>
        && nothrow_comparable<const TSubject&, TChar*>
        && nothrow_comparable<const TSubject&, const std::basic_string_view<TChar>&>
        && nothrow_comparable<const TSubject&, const std::basic_string<TChar>&>
        && nothrow_comparable<const TSubject&, std::basic_string<TChar>&>
        && nothrow_comparable<const TSubject&, const basic_fixed_string<TChar, 4>&>
        && nothrow_comparable<const TSubject&, const basic_fixed_string<TChar, 9>&>
        && nothrow_comparable<const TSubject&, const basic_ct_string_view<TChar, true>&>
        && nothrow_comparable<const TSubject&, const basic_ct_string_view<TChar, false>&>;

    template<typename TChar>
    concept every_subject_nothrow_comparable =
        nothrow_comparable_with_every_operand<basic_ct_string_view<TChar, true>, TChar>
        && nothrow_comparable_with_every_operand<basic_ct_string_view<TChar, false>, TChar>
        && nothrow_comparable_with_every_operand<basic_fixed_string<TChar, 4>, TChar>;

    static_assert(every_subject_nothrow_comparable<char>);
    static_assert(every_subject_nothrow_comparable<wchar_t>);
    static_assert(every_subject_nothrow_comparable<char8_t>);
    static_assert(every_subject_nothrow_comparable<char16_t>);
    static_assert(every_subject_nothrow_comparable<char32_t>);

    // -- what must not compile -------------------------------------------------

    template<typename L, typename R>
    concept comparable_one_way_at_all = requires (L l, R r) { l == r; }
        || requires (L l, R r) { l != r; }
        || requires (L l, R r) { l < r; }
        || requires (L l, R r) { l <=> r; };

    template<typename L, typename R>
    concept comparable_at_all = comparable_one_way_at_all<L, R> || comparable_one_way_at_all<R, L>;

    /// No array of, or pointer to, TOtherChar compares with TSubject.
    template<typename TSubject, typename TOtherChar>
    concept compares_with_no_array_or_pointer_of =
        !comparable_at_all<const TSubject&, const TOtherChar (&)[4]>
        && !comparable_at_all<const TSubject&, TOtherChar (&)[4]>
        && !comparable_at_all<const TSubject&, const TOtherChar*>
        && !comparable_at_all<const TSubject&, TOtherChar*>;

    /// Nothing of character type TOtherChar compares with TSubject.
    template<typename TSubject, typename TOtherChar>
    concept compares_with_nothing_of = compares_with_no_array_or_pointer_of<TSubject, TOtherChar>
        && !comparable_at_all<const TSubject&, const std::basic_string_view<TOtherChar>&>
        && !comparable_at_all<const TSubject&, const std::basic_string<TOtherChar>&>
        && !comparable_at_all<const TSubject&, const basic_fixed_string<TOtherChar, 4>&>
        && !comparable_at_all<const TSubject&, const basic_ct_string_view<TOtherChar, true>&>
        && !comparable_at_all<const TSubject&, const basic_ct_string_view<TOtherChar, false>&>;

    /// Nothing of another std_char type compares with TSubject; neither do
    /// arrays of, or pointers to, signed char and unsigned char; nor does
    /// nullptr. (std::basic_string_view<unsigned char> is not used: the
    /// standard provides no std::char_traits<unsigned char>.)
    template<typename TSubject, typename... TOtherChars>
    concept rejects_foreign_operands = (compares_with_nothing_of<TSubject, TOtherChars> && ...)
        && compares_with_no_array_or_pointer_of<TSubject, signed char>
        && compares_with_no_array_or_pointer_of<TSubject, unsigned char>
        && !comparable_at_all<const TSubject&, std::nullptr_t>;

    template<typename TChar, typename... TOtherChars>
    concept every_subject_rejects_foreign_operands =
        rejects_foreign_operands<basic_ct_string_view<TChar, true>, TOtherChars...>
        && rejects_foreign_operands<basic_ct_string_view<TChar, false>, TOtherChars...>
        && rejects_foreign_operands<basic_fixed_string<TChar, 4>, TOtherChars...>;

    static_assert(every_subject_rejects_foreign_operands<char, wchar_t, char8_t, char16_t, char32_t>);
    static_assert(every_subject_rejects_foreign_operands<wchar_t, char, char8_t, char16_t, char32_t>);
    static_assert(every_subject_rejects_foreign_operands<char8_t, char, wchar_t, char16_t, char32_t>);
    static_assert(every_subject_rejects_foreign_operands<char16_t, char, wchar_t, char8_t, char32_t>);
    static_assert(every_subject_rejects_foreign_operands<char32_t, char, wchar_t, char8_t, char16_t>);

    // -- the public convertibility concepts (Q-23) -----------------------------

    struct throwing_to_sv
    {
        operator std::string_view() const { return {}; } // NOLINT(*-explicit-constructor)
    };

    struct nothrow_to_sv
    {
        operator std::string_view() const noexcept { return {}; } // NOLINT(*-explicit-constructor)
    };

    static_assert(nothrow_convertible_to<nothrow_to_sv, std::string_view>);
    static_assert(!nothrow_convertible_to<throwing_to_sv, std::string_view>);
    static_assert(not_exact_same_convertible_to<throwing_to_sv, std::string_view>);
    static_assert(not_exact_same_nothrow_convertible_to<nothrow_to_sv, std::string_view>);
    static_assert(!not_exact_same_nothrow_convertible_to<throwing_to_sv, std::string_view>,
        "the name promises noexcept, so a conversion that may throw must not satisfy it");
    static_assert(!not_exact_same_nothrow_convertible_to<std::string_view, std::string_view>);
    static_assert(not_same_unqual_type_nothrow_convertible_to<nothrow_to_sv, std::string_view>);
    static_assert(!not_same_unqual_type_nothrow_convertible_to<throwing_to_sv, std::string_view>);

    // -- operands whose conversion may throw -----------------------------------
    // The operators are noexcept, and they convert a const lvalue of the other
    // operand. An operand whose conversion from a const lvalue may throw must
    // therefore not compare, even when its conversion from an rvalue cannot
    // throw (D-030). Before T3 such an operand compared, and a throw would have
    // called std::terminate.

    struct throws_only_from_const_lvalue
    {
        operator std::string_view() const& { return {}; } // NOLINT(*-explicit-constructor)
        operator std::string_view() && noexcept { return {}; } // NOLINT(*-explicit-constructor)
    };

    template<typename TSubject>
    concept rejects_throwing_conversions =
        !comparable_at_all<const TSubject&, const throwing_to_sv&>
        && !comparable_at_all<const TSubject&, const throws_only_from_const_lvalue&>;

    static_assert(rejects_throwing_conversions<basic_ct_string_view<char, true>>);
    static_assert(rejects_throwing_conversions<basic_ct_string_view<char, false>>);
    static_assert(rejects_throwing_conversions<basic_fixed_string<char, 4>>);

    // Conversely, an operand whose conversion from a const lvalue cannot throw
    // compares, even when its conversion from an rvalue is deleted: that one is
    // never used.

    struct nothrow_only_from_const_lvalue
    {
        operator std::string_view() const& noexcept { return {}; } // NOLINT(*-explicit-constructor)
        operator std::string_view() && = delete; // NOLINT(*-explicit-constructor)
    };

    static_assert(nothrow_comparable<const basic_ct_string_view<char, true>&,
        const nothrow_only_from_const_lvalue&>);
    static_assert(nothrow_comparable<const basic_ct_string_view<char, false>&,
        const nothrow_only_from_const_lvalue&>);
    static_assert(nothrow_comparable<const basic_fixed_string<char, 4>&,
        const nothrow_only_from_const_lvalue&>);

    // -- comparators, hashers and container lookups never take nullptr ---------
    // They convert their keys to std::basic_string_view, which in C++20 still
    // converts from nullptr (P2166 removed that in C++23); the call then has
    // undefined behaviour. None of them accepts nullptr, or a literal 0 (a
    // common spelling of NULL), on any standard library or language version
    // (Q-24, D-032). Each check also confirms that every other key still
    // works, so the rejection is specific to nullptr. For the containers, that
    // includes empty braces and a key that also converts to nullptr, which the
    // first version of D-032 rejected by mistake (Codex's review of PR #17).

    static_assert(!ctsv_lookup_key<std::nullptr_t, char>);
    static_assert(!ctsv_lookup_key<const std::nullptr_t, char>);
    static_assert(!ctsv_lookup_key<std::nullptr_t, char32_t>);
    static_assert(ctsv_lookup_key<const char*, char>);
    static_assert(ctsv_lookup_key<const char32_t*, char32_t>);

    /// \p F takes a \p TOther and a \p TKey, in at least one order.
    template<typename F, typename TOther, typename TKey>
    concept invocable_either_way = std::invocable<const F&, const TOther&, TKey>
        || std::invocable<const F&, TKey, const TOther&>;

    /// \p F takes a \p TOther and a \p TKey, in both orders.
    template<typename F, typename TOther, typename TKey>
    concept invocable_both_ways = std::invocable<const F&, const TOther&, TKey>
        && std::invocable<const F&, TKey, const TOther&>;

    /// The comparator \p F compares each key flavor with literals, arrays,
    /// pointers and strings, but never with nullptr or a literal 0.
    template<typename F, typename TChar>
    concept compares_keys_but_not_null =
        invocable_both_ways<F, basic_ct_string_view<TChar, true>, const TChar (&)[4]>
        && invocable_both_ways<F, basic_ct_string_view<TChar, true>, const TChar*>
        && invocable_both_ways<F, basic_ct_string_view<TChar, false>, TChar*>
        && invocable_both_ways<F, basic_ct_string_view<TChar, false>, const std::basic_string<TChar>&>
        && invocable_both_ways<F, std::basic_string_view<TChar>, const TChar*>
        && !invocable_either_way<F, basic_ct_string_view<TChar, true>, std::nullptr_t>
        && !invocable_either_way<F, basic_ct_string_view<TChar, false>, std::nullptr_t>
        && !requires (const F& f, const basic_ct_string_view<TChar, true>& key) { f(key, 0); }
        && !requires (const F& f, const basic_ct_string_view<TChar, true>& key) { f(0, key); };

    /// As compares_keys_but_not_null, and \p F does not compare nullptr with a
    /// std::basic_string_view either. This does not hold for std::less<> and
    /// std::equal_to<>: for them that pairing is std::basic_string_view's own
    /// comparison, which in C++20 still accepts nullptr. The containers never
    /// pass a comparator nullptr, whatever it is: their lookups reject it.
    template<typename F, typename TChar>
    concept never_compares_null = compares_keys_but_not_null<F, TChar>
        && !invocable_either_way<F, std::basic_string_view<TChar>, std::nullptr_t>;

    /// The hasher \p H hashes literals, arrays, pointers and string views, but
    /// never nullptr or a literal 0.
    template<typename H, typename TChar>
    concept hashes_keys_but_not_null =
        std::invocable<const H&, const TChar (&)[4]>
        && std::invocable<const H&, const TChar*>
        && std::invocable<const H&, TChar*>
        && std::invocable<const H&, const std::basic_string_view<TChar>&>
        && !std::invocable<const H&, std::nullptr_t>
        && !requires (const H& h) { h(0); };

    template<typename TChar>
    concept every_function_object_rejects_null =
        never_compares_null<ctsv_less<TChar>, TChar>
        && never_compares_null<ctsv_ci_less<TChar>, TChar>
        && never_compares_null<ctsv_equal_to<TChar>, TChar>
        && never_compares_null<ctsv_ci_equal_to<TChar>, TChar>
        && never_compares_null<ctsv_three_way<TChar>, TChar>
        && never_compares_null<ctsv_ci_three_way<TChar>, TChar>
        && compares_keys_but_not_null<std::less<>, TChar>
        && compares_keys_but_not_null<std::equal_to<>, TChar>
        && hashes_keys_but_not_null<ctsv_hash<TChar>, TChar>
        && hashes_keys_but_not_null<ctsv_ci_hash<TChar>, TChar>
        && hashes_keys_but_not_null<std::hash<basic_ct_string_view<TChar, true>>, TChar>
        && hashes_keys_but_not_null<std::hash<basic_ct_string_view<TChar, false>>, TChar>;

    static_assert(every_function_object_rejects_null<char>);
    static_assert(every_function_object_rejects_null<wchar_t>);
    static_assert(every_function_object_rejects_null<char8_t>);
    static_assert(every_function_object_rejects_null<char16_t>);
    static_assert(every_function_object_rejects_null<char32_t>);

    /// Every lookup member of the container \p C takes a const \p TKey lvalue.
    /// Only the ordered containers have lower_bound and upper_bound, only the
    /// maps have at and at_if, and erase needs a non-const container.
    template<typename C, typename TKey, bool ORDERED, bool MAP>
    concept looks_up_with = requires (C& c, const TKey& key)
        {
            c.find(key);
            c.contains(key);
            c.count(key);
            c.equal_range(key);
        }
        && (!ORDERED || requires (C& c, const TKey& key) { c.lower_bound(key); c.upper_bound(key); })
        && (!MAP || requires (C& c, const TKey& key) { c.at(key); c.at_if(key); })
        && (std::is_const_v<C> || requires (C& c, const TKey& key) { c.erase(key); });

    /// Every lookup member of the container \p C takes empty braces, which make
    /// an empty std::basic_string_view. erase does not: there, {} could equally
    /// be an iterator, as it can for the standard containers.
    template<typename C, bool ORDERED, bool MAP>
    concept looks_up_with_empty_braces = requires (C& c)
        {
            c.find({});
            c.contains({});
            c.count({});
            c.equal_range({});
        }
        && (!ORDERED || requires (C& c) { c.lower_bound({}); c.upper_bound({}); })
        && (!MAP || requires (C& c) { c.at({}); c.at_if({}); });

    /// A key that converts both to a std::basic_string_view and to nullptr. Only
    /// the first conversion leads to a key, so a lookup must use it.
    template<typename TChar>
    struct converts_to_view_and_to_null
    {
        // NOLINTNEXTLINE(*-explicit-constructor)
        operator std::basic_string_view<TChar>() const noexcept { return {}; }
        // NOLINTNEXTLINE(*-explicit-constructor)
        operator std::nullptr_t() const noexcept { return nullptr; }
    };

    /// Some member of the container \p C takes nullptr or a literal 0 as a key.
    /// A member that \p C does not have takes nothing.
    /// This checks only that such a call does not compile. A requires-expression
    /// cannot tell a deleted overload from an ambiguous call, so it does not check
    /// that the deleted overloads are the ones chosen (D-032). Braced lists, such as
    /// {nullptr}, are not rejected in C++20: they build the string view in place.
    template<typename C>
    concept some_lookup_takes_null =
        requires (C& c) { c.find(nullptr); } || requires (C& c) { c.find(0); }
        || requires (C& c) { c.contains(nullptr); } || requires (C& c) { c.contains(0); }
        || requires (C& c) { c.count(nullptr); } || requires (C& c) { c.count(0); }
        || requires (C& c) { c.equal_range(nullptr); } || requires (C& c) { c.equal_range(0); }
        || requires (C& c) { c.lower_bound(nullptr); } || requires (C& c) { c.lower_bound(0); }
        || requires (C& c) { c.upper_bound(nullptr); } || requires (C& c) { c.upper_bound(0); }
        || requires (C& c) { c.at(nullptr); } || requires (C& c) { c.at(0); }
        || requires (C& c) { c.at_if(nullptr); } || requires (C& c) { c.at_if(0); }
        || requires (C& c) { c.erase(nullptr); } || requires (C& c) { c.erase(0); };

    /// The container \p C, const or not, looks up every key kind, including empty
    /// braces and a key that also converts to nullptr, and never nullptr or a
    /// literal 0.
    template<typename C, typename TChar, bool ORDERED, bool MAP>
    concept looks_up_every_key_but_null =
        looks_up_with<C, TChar[4], ORDERED, MAP>
        && looks_up_with<C, const TChar*, ORDERED, MAP>
        && looks_up_with<C, TChar*, ORDERED, MAP>
        && looks_up_with<C, std::basic_string_view<TChar>, ORDERED, MAP>
        && looks_up_with<C, std::basic_string<TChar>, ORDERED, MAP>
        && looks_up_with<C, basic_fixed_string<TChar, 4>, ORDERED, MAP>
        && looks_up_with<C, basic_ct_string_view<TChar, true>, ORDERED, MAP>
        && looks_up_with<C, basic_ct_string_view<TChar, false>, ORDERED, MAP>
        && looks_up_with<C, converts_to_view_and_to_null<TChar>, ORDERED, MAP>
        && looks_up_with<const C, const TChar*, ORDERED, MAP>
        && looks_up_with<const C, converts_to_view_and_to_null<TChar>, ORDERED, MAP>
        && looks_up_with_empty_braces<C, ORDERED, MAP>
        && looks_up_with_empty_braces<const C, ORDERED, MAP>
        && !some_lookup_takes_null<C>
        && !some_lookup_takes_null<const C>;

    template<typename TChar, bool VALID_CSTR>
    concept every_container_rejects_null =
        looks_up_every_key_but_null<basic_ctsv_map<TChar, VALID_CSTR, int>, TChar, true, true>
        && looks_up_every_key_but_null<basic_ctsv_set<TChar, VALID_CSTR>, TChar, true, false>
        && looks_up_every_key_but_null<basic_ctsv_unordered_map<TChar, VALID_CSTR, int>, TChar, false, true>
        && looks_up_every_key_but_null<basic_ctsv_unordered_set<TChar, VALID_CSTR>, TChar, false, false>;

    static_assert(every_container_rejects_null<char, true>);
    static_assert(every_container_rejects_null<char, false>);
    static_assert(every_container_rejects_null<wchar_t, true>);
    static_assert(every_container_rejects_null<char8_t, false>);
    static_assert(every_container_rejects_null<char16_t, true>);
    static_assert(every_container_rejects_null<char32_t, false>);

    // The case-insensitive containers, and an ordered map using std::less<>.
    static_assert(looks_up_every_key_but_null<ct_cstring_view_ci_map<int>, char, true, true>);
    static_assert(looks_up_every_key_but_null<ct_cstring_view_ci_unordered_set, char, false, false>);
    static_assert(looks_up_every_key_but_null<basic_ctsv_map<char, true, int, std::less<>>, char, true, true>);
}

#undef CPS_CMP_LIT
