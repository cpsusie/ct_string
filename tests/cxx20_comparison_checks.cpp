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
//   * comparing with another character type, or with nullptr, does not
//     compile.
//
// Like cxx20_header_smoke.cpp, this TU is part of the C++20-pinned smoke
// target, so it needs no test framework and runs wherever that target builds.
// Before T3, the literal, array and pointer checks failed with libc++, whose
// std::basic_string_view(const CharT*) is not noexcept (FINDINGS F-5).
// ---------------------------------------------------------------------------
#include <ct_str/ct_string_view.hpp>
#include <ct_str/fixed_string.hpp>

#include <compare>
#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>

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
}

#undef CPS_CMP_LIT
