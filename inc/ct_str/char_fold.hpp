// ---------------------------------------------------------------------------
// char_fold.hpp -- ASCII case-folding function objects and lazy range adaptors.
// ---------------------------------------------------------------------------
//
// This header provides the lowest layer of the case-insensitive comparison
// facilities: constexpr, noexcept, allocation-free case folding of individual
// code units, plus lazy views that apply that folding across a range.
//
// SCOPE: ASCII ONLY. Only the 26 code units 'A'..'Z' (and 'a'..'z' for the
// upper-casing direction) are transformed; every other code unit -- including
// every code unit >= 0x80 -- passes through completely unchanged.
//
// Why that is a safe (if limited) choice:
//
//   * For UTF-8 (char8_t, and char when it holds UTF-8) every byte of a
//     multi-byte sequence is >= 0x80, so no byte of a non-ASCII character can
//     ever be mistaken for an ASCII letter. Folding is therefore never
//     *destructive*; it simply does nothing outside the ASCII range.
//   * For UTF-16 / UTF-32 the same holds: only the ASCII sub-range is touched,
//     and the ASCII sub-range has identical meaning in every Unicode encoding.
//   * The transformation is a pure per-code-unit map, so it composes with
//     std::ranges projections without materializing anything.
//
// \todo Proper Unicode case folding (full, simple, or Turkic-aware) requires
// the Unicode CaseFolding.txt tables plus multi-code-point expansion (e.g.
// U+00DF LATIN SMALL LETTER SHARP S folds to "ss", changing the *length* of
// the range, which defeats the per-code-unit projection model used here). That
// is a project unto itself -- likely an ICU dependency or a generated table of
// a few thousand entries -- and is deliberately out of scope. Do NOT attempt a
// partial "Latin-1 also works" extension: any folding that is applied
// inconsistently between the hasher and the equality predicate silently breaks
// the unordered-container invariant that equal keys hash equally.
// ---------------------------------------------------------------------------
#ifndef CPS_CT_STR_CHAR_FOLD_HPP
#define CPS_CT_STR_CHAR_FOLD_HPP

#include "fixed_string.hpp"
#include <ranges>
#include <type_traits>

namespace cps::ct_string
{
    namespace detail
    {
        /// \brief The ASCII code-unit constants needed by the folders,
        /// materialized in the target character type.
        /// \remarks static_cast (rather than brace-init) is used deliberately:
        /// although 'A' etc. are constant expressions that fit every std_char
        /// type, an explicit cast documents intent and avoids any dependence on
        /// the narrowing-conversion rules for braced initialization.
        template<std_char TChar>
        struct ascii_case_constants
        {
            static constexpr TChar upper_a = static_cast<TChar>('A');
            static constexpr TChar upper_z = static_cast<TChar>('Z');
            static constexpr TChar lower_a = static_cast<TChar>('a');
            static constexpr TChar lower_z = static_cast<TChar>('z');
            /// \brief 'a' - 'A' == 32. Held as int to keep the arithmetic in
            /// the promoted type; the result is cast back to TChar.
            static constexpr int case_delta = static_cast<int>('a') - static_cast<int>('A');
        };
    }

    /// \brief Function object mapping an ASCII upper-case code unit to its
    /// lower-case counterpart; every other code unit is returned unchanged.
    /// \tparam TChar the character type operated upon.
    /// \remarks Comparisons are performed in TChar, never after narrowing to
    /// char. Narrowing a wide code unit to char before testing would alias
    /// unrelated code points onto 'A'..'Z' (e.g. U+0141 truncates to 0x41 ==
    /// 'A') and corrupt them.
    template<std_char TChar>
    struct ascii_to_lower_fn final
    {
        using char_type = TChar;

        /// \brief Folds \p c to lower case if it is an ASCII upper-case letter.
        /// \param[in] c the code unit to fold.
        /// \returns the folded code unit, or \p c unchanged.
        [[nodiscard]] constexpr char_type operator()(const char_type c) const noexcept
        {
            using k = detail::ascii_case_constants<char_type>;
            return (c >= k::upper_a && c <= k::upper_z)
                ? static_cast<char_type>(static_cast<int>(c) + k::case_delta)
                : c;
        }
    };

    /// \brief Function object mapping an ASCII lower-case code unit to its
    /// upper-case counterpart; every other code unit is returned unchanged.
    /// \tparam TChar the character type operated upon.
    /// \remarks See ascii_to_lower_fn for the rationale behind comparing in
    /// TChar rather than char.
    template<std_char TChar>
    struct ascii_to_upper_fn final
    {
        using char_type = TChar;

        /// \brief Folds \p c to upper case if it is an ASCII lower-case letter.
        /// \param[in] c the code unit to fold.
        /// \returns the folded code unit, or \p c unchanged.
        [[nodiscard]] constexpr char_type operator()(const char_type c) const noexcept
        {
            using k = detail::ascii_case_constants<char_type>;
            return (c >= k::lower_a && c <= k::lower_z)
                ? static_cast<char_type>(static_cast<int>(c) - k::case_delta)
                : c;
        }
    };

    /// \brief Ready-made ascii_to_lower_fn instance for \p TChar. Intended for
    /// use as a std::ranges projection argument.
    template<std_char TChar>
    inline constexpr ascii_to_lower_fn<TChar> ascii_to_lower{};

    /// \brief Ready-made ascii_to_upper_fn instance for \p TChar. Intended for
    /// use as a std::ranges projection argument.
    template<std_char TChar>
    inline constexpr ascii_to_upper_fn<TChar> ascii_to_upper{};

    namespace detail
    {
        /// \brief The (cvref-stripped) element type of a range, for ranges
        /// whose elements are std_char.
        template<typename R>
        using range_char_t = std::remove_cvref_t<std::ranges::range_value_t<R>>;

        /// \brief A viewable range of std_char code units.
        template<typename R>
        concept viewable_char_range =
            std::ranges::viewable_range<R> && std_char<range_char_t<R>>;
    }

    namespace views
    {
        /// \brief Range adaptor yielding a lazy, allocation-free view of a
        /// character range with every ASCII upper-case code unit folded down.
        ///
        /// The result is a std::ranges::transform_view: nothing is copied and
        /// no storage is acquired. The source range must outlive the view
        /// unless it is a borrowed range (std::basic_string_view and
        /// basic_ct_string_view both are).
        ///
        /// Example:
        /// \code
        ///     constexpr std::string_view s = "Hello";
        ///     for (const char c : cps::ct_string::views::ascii_lower(s))
        ///     {
        ///         // 'h', 'e', 'l', 'l', 'o'
        ///     }
        /// \endcode
        struct ascii_lower_fn final
        {
            /// \brief Adapts \p r into a lower-folded transform_view.
            template<detail::viewable_char_range R>
            [[nodiscard]] constexpr auto operator()(R&& r) const
            {
                using char_type = detail::range_char_t<R>;
                return std::views::transform(std::forward<R>(r),
                    ascii_to_lower_fn<char_type>{});
            }
        };

        /// \brief Range adaptor yielding a lazy, allocation-free view of a
        /// character range with every ASCII lower-case code unit folded up.
        /// \see ascii_lower_fn for the lifetime remarks.
        struct ascii_upper_fn final
        {
            /// \brief Adapts \p r into an upper-folded transform_view.
            template<detail::viewable_char_range R>
            [[nodiscard]] constexpr auto operator()(R&& r) const
            {
                using char_type = detail::range_char_t<R>;
                return std::views::transform(std::forward<R>(r),
                    ascii_to_upper_fn<char_type>{});
            }
        };

        /// \brief Adaptor instance: views::ascii_lower(range).
        inline constexpr ascii_lower_fn ascii_lower{};
        /// \brief Adaptor instance: views::ascii_upper(range).
        inline constexpr ascii_upper_fn ascii_upper{};
    }

    // -- self-checks -------------------------------------------------------
    // The fold must be identity outside 'A'..'Z' / 'a'..'z'. The '_' case is
    // load bearing: 0x5F sits BETWEEN 'Z' (0x5A) and 'a' (0x61), so the
    // direction in which we fold is observable in any ordering built on top of
    // this. We fold DOWN, hence 'Z' becomes 'z' (0x7A), which is GREATER than
    // '_'. See ctsv_comparators.hpp.
    static_assert(ascii_to_lower<char>('A') == 'a');
    static_assert(ascii_to_lower<char>('Z') == 'z');
    static_assert(ascii_to_lower<char>('a') == 'a');
    static_assert(ascii_to_lower<char>('_') == '_');
    static_assert(ascii_to_lower<char>('0') == '0');
    static_assert(ascii_to_upper<char>('a') == 'A');
    static_assert(ascii_to_upper<char>('Z') == 'Z');
    static_assert(ascii_to_upper<char>('_') == '_');

    static_assert(ascii_to_lower<wchar_t>(L'A') == L'a');
    static_assert(ascii_to_lower<wchar_t>(L'\u00C0') == L'\u00C0', "non-ASCII must pass through");
    static_assert(ascii_to_lower<char8_t>(u8'A') == u8'a');
    static_assert(ascii_to_lower<char8_t>(static_cast<char8_t>(0xC3)) == static_cast<char8_t>(0xC3),
        "UTF-8 lead/continuation bytes must pass through");
    static_assert(ascii_to_lower<char16_t>(u'A') == u'a');
    static_assert(ascii_to_lower<char16_t>(u'\u0141') == u'\u0141',
        "a code unit whose low byte aliases 'A' must NOT be folded");
    static_assert(ascii_to_lower<char32_t>(U'A') == U'a');
    static_assert(ascii_to_lower<char32_t>(U'\U0001F600') == U'\U0001F600');
}

#endif // CPS_CT_STR_CHAR_FOLD_HPP

