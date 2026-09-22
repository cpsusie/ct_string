// ---------------------------------------------------------------------------
// ctsv_comparators.hpp -- transparent comparison / hashing function objects for
// basic_ct_string_view keys, and the concepts that describe them.
// ---------------------------------------------------------------------------
//
// THE PROBLEM THIS SOLVES
//
// A basic_ct_string_view<TChar, VALID_CSTR> makes a great associative-container
// key: it is two words wide, trivially copyable, and its storage is guaranteed
// to outlive the container. But it is a *strict* type -- you cannot build one
// from a std::string_view, because doing so would discard the compile-time
// provenance and null-termination guarantees that make it worth having.
//
// That strictness is a problem at LOOKUP time: callers almost always have a
// std::string_view, a std::string, or a string literal in hand -- not a
// basic_ct_string_view. The answer is heterogeneous lookup, which the standard
// gates on the comparator (ordered containers) or on the hasher AND the
// equality predicate (unordered containers) advertising a nested
// `is_transparent` type.
//
// This header supplies four families of such function objects, each available
// in a case-sensitive and a case-insensitive flavor, plus the concepts that
// specify precisely what "a properly transparent comparator for this context"
// means so that misuse is a compile error rather than a silent fallback to
// homogeneous lookup.
//
// WHY A SINGLE CONSTRAINED operator() INSTEAD OF AN OVERLOAD SET
//
// An earlier design enumerated every pairing of {this flavor, other flavor,
// string_view} explicitly -- eight overloads per comparator. That is both
// unmaintainable and subtly incomplete: std::relation (and hence
// std::strict_weak_order) requires the predicate to be invocable in ALL FOUR
// argument orderings of any two types, and a hand-written overload set
// reliably misses some. Every basic_ct_string_view, basic_fixed_string,
// std::basic_string and std::basic_string_view of a given TChar converts
// implicitly (and, except for raw pointers, noexcept-ly) to
// std::basic_string_view<TChar>, so ONE constrained template handles the whole
// matrix, with conditional noexcept falling out automatically.
//
// CASE FOLDING DIRECTION IS OBSERVABLE
//
// The case-insensitive comparators fold DOWN (toward lower case), matching
// char_fold.hpp. This is not arbitrary: '_' is 0x5F, which sits BETWEEN 'Z'
// (0x5A) and 'a' (0x61). Folding down maps 'Z' to 'z' (0x7A), so under
// ctsv_ci_less "Z" sorts AFTER "_"; folding up would have produced the
// opposite. Both are legitimate strict weak orderings; you simply must know
// which one you have. See the static_asserts at the bottom of this header.
//
// CASE-INSENSITIVE ORDERING IS WEAK, NOT STRONG
//
// "abc" and "ABC" are distinct strings that compare EQUIVALENT under the
// case-insensitive comparators. That is the definition of a weak (not strong)
// ordering, so ctsv_ci_three_way returns std::weak_ordering while
// ctsv_three_way returns std::strong_ordering. Do not "simplify" this.
// ---------------------------------------------------------------------------
#ifndef CPS_CT_STR_CTSV_COMPARATORS_HPP
#define CPS_CT_STR_CTSV_COMPARATORS_HPP

#include "char_fold.hpp"
#include "ct_string_view.hpp"
#include <algorithm>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <ranges>
#include <string_view>
#include <type_traits>

namespace cps::ct_string
{
    // =====================================================================
    // Lookup-key concepts
    // =====================================================================

    /// \brief A type usable as a heterogeneous lookup key against a
    /// basic_ct_string_view<TChar, *> key.
    ///
    /// Satisfied by: either flavor of basic_ct_string_view<TChar, *>,
    /// std::basic_string_view<TChar>, std::basic_string<TChar>,
    /// basic_fixed_string<TChar, N>, and a null-terminated const TChar*
    /// (including string literals).
    ///
    /// \remarks Raw pointers are deliberately admitted even though the
    /// standard does NOT mark std::basic_string_view(const CharT*) noexcept
    /// (it calls traits_type::length). Excluding them would defeat the entire
    /// ergonomic point of the wrappers -- `m.at("literal")` has to work. The
    /// comparators' noexcept specification is conditional, so whichever answer
    /// a given standard library gives (libstdc++ and the MS STL both DO mark
    /// that constructor noexcept; the standard does not require it) the
    /// comparators remain correct.
    template<typename T, typename TChar>
    concept ctsv_lookup_key =
        std_char<TChar> && std::convertible_to<const T&, std::basic_string_view<TChar>>;

    /// \brief A ctsv_lookup_key whose conversion to std::basic_string_view is
    /// additionally noexcept. Drives the conditional noexcept on every
    /// comparator in this header.
    template<typename T, typename TChar>
    concept nothrow_ctsv_lookup_key =
        ctsv_lookup_key<T, TChar>
        && nothrow_convertible_to<const T&, std::basic_string_view<TChar>>;

    // =====================================================================
    // Case-folding introspection
    // =====================================================================

    /// \brief Trait reporting whether a comparison/hashing function object
    /// folds case. Defaults to false; a function object opts in by declaring
    /// `using folds_case = std::true_type;`.
    ///
    /// The default of "false" is what makes std::less<>, std::equal_to<> and
    /// std::hash<basic_ct_string_view<...>> -- none of which know anything
    /// about this header -- pair correctly with each other while still being
    /// rejected when paired with a case-insensitive counterpart.
    ///
    /// Users may specialize this for their own function objects instead of
    /// adding a nested type.
    template<typename T>
    struct ctsv_folds_case : std::false_type {};

    /// \brief Picks up an opted-in nested `folds_case` type.
    template<typename T>
        requires requires { typename T::folds_case; }
    struct ctsv_folds_case<T> : T::folds_case {};

    /// \brief Convenience variable template for ctsv_folds_case.
    template<typename T>
    inline constexpr bool ctsv_folds_case_v = ctsv_folds_case<T>::value;

    namespace detail
    {
        /// \brief Detects the nested type the standard library uses to gate
        /// heterogeneous lookup.
        template<typename T>
        concept has_is_transparent = requires { typename T::is_transparent; };

        // -- The pairing matrix -------------------------------------------
        //
        // Heterogeneous lookup exercises a comparator with EVERY combination
        // of {key flavor, other key flavor, std::basic_string_view}. The
        // row/matrix concept pairs below expand a per-pair requirement over
        // that full cartesian product, so a comparator that handles only
        // some orderings cannot satisfy the concepts.

        template<typename F, typename L, typename R>
        concept ctsv_less_pair =
            std::strict_weak_order<const F&, L, R>
            && requires (const F& f, const L& l, const R& r)
            {
                { f(l, r) } noexcept -> nothrow_convertible_to<bool>;
            };

        template<typename F, typename L, typename... Rs>
        concept ctsv_less_row = (ctsv_less_pair<F, L, Rs> && ...);

        template<typename F, typename... Ts>
        concept ctsv_less_matrix = (ctsv_less_row<F, Ts, Ts...> && ...);

        template<typename F, typename L, typename R>
        concept ctsv_equal_pair =
            std::equivalence_relation<const F&, L, R>
            && requires (const F& f, const L& l, const R& r)
            {
                { f(l, r) } noexcept -> nothrow_convertible_to<bool>;
            };

        template<typename F, typename L, typename... Rs>
        concept ctsv_equal_row = (ctsv_equal_pair<F, L, Rs> && ...);

        template<typename F, typename... Ts>
        concept ctsv_equal_matrix = (ctsv_equal_row<F, Ts, Ts...> && ...);

        template<typename F, typename L, typename R>
        concept ctsv_three_way_pair =
            requires (const F& f, const L& l, const R& r)
            {
                { f(l, r) } noexcept -> std::convertible_to<std::weak_ordering>;
            };

        template<typename F, typename L, typename... Rs>
        concept ctsv_three_way_row = (ctsv_three_way_pair<F, L, Rs> && ...);

        template<typename F, typename... Ts>
        concept ctsv_three_way_matrix = (ctsv_three_way_row<F, Ts, Ts...> && ...);

        template<typename F, typename T>
        concept ctsv_hash_one =
            requires (const F& f, const T& t)
            {
                { f(t) } noexcept -> std::same_as<std::size_t>;
            };

        template<typename F, typename... Ts>
        concept ctsv_hash_all = (ctsv_hash_one<F, Ts> && ...);

        /// \brief The three key spellings every transparent function object in
        /// this header must accept, for a given TChar.
        ///
        /// \remarks TChar is deliberately an UNCONSTRAINED template parameter:
        /// the grammar forbids constraining a concept's own template
        /// parameters. The std_char check is therefore the FIRST operand of
        /// each conjunction below. That placement is load bearing -- a
        /// conjunction short-circuits, and the right operand is not even
        /// substituted into when the left is unsatisfied, which is what keeps
        /// `basic_ct_string_view<NotAChar, true>` from becoming a hard error.
        template<typename F, typename TChar>
        concept accepts_all_key_flavors_less = std_char<TChar> && ctsv_less_matrix<F,
            basic_ct_string_view<TChar, true>,
            basic_ct_string_view<TChar, false>,
            std::basic_string_view<TChar>>;

        template<typename F, typename TChar>
        concept accepts_all_key_flavors_equal = std_char<TChar> && ctsv_equal_matrix<F,
            basic_ct_string_view<TChar, true>,
            basic_ct_string_view<TChar, false>,
            std::basic_string_view<TChar>>;

        template<typename F, typename TChar>
        concept accepts_all_key_flavors_three_way = std_char<TChar> && ctsv_three_way_matrix<F,
            basic_ct_string_view<TChar, true>,
            basic_ct_string_view<TChar, false>,
            std::basic_string_view<TChar>>;

        template<typename F, typename TChar>
        concept accepts_all_key_flavors_hash = std_char<TChar> && ctsv_hash_all<F,
            basic_ct_string_view<TChar, true>,
            basic_ct_string_view<TChar, false>,
            std::basic_string_view<TChar>>;
    }

    // =====================================================================
    // Comparator concepts
    // =====================================================================

    /// \brief A comparator suitable as the Compare parameter of an ordered
    /// associative container keyed on basic_ct_string_view<TChar, *>.
    ///
    /// Requires:
    ///   * nothrow default-constructibility (containers default-construct it,
    ///     and the constexpr containers in this library require it);
    ///   * a nested `is_transparent` type -- WITHOUT this the standard library
    ///     silently falls back to homogeneous lookup, converting every lookup
    ///     key to the key type, which for basic_ct_string_view does not even
    ///     compile;
    ///   * that it be a strict weak ordering over every pairing of
    ///     {ct view (cstr flavor), ct view (non-cstr flavor), string_view},
    ///     in both argument orders;
    ///   * that all of those invocations be noexcept and yield something
    ///     nothrow-convertible to bool.
    ///
    /// \remarks std::less<> satisfies this for every TChar, because
    /// basic_ct_string_view provides heterogeneous operator<=>.
    template<typename F, typename TChar>
    concept transparent_ctsv_less =
        std_char<TChar>
        && std::is_nothrow_default_constructible_v<F>
        && detail::has_is_transparent<F>
        && detail::accepts_all_key_flavors_less<F, TChar>;

    /// \brief An equality predicate suitable as the KeyEqual parameter of an
    /// unordered associative container keyed on basic_ct_string_view<TChar, *>.
    /// \see transparent_ctsv_less for the rationale behind each requirement;
    /// this concept requires an equivalence relation rather than an ordering.
    /// \remarks std::equal_to<> satisfies this for every TChar.
    template<typename F, typename TChar>
    concept transparent_ctsv_equal_to =
        std_char<TChar>
        && std::is_nothrow_default_constructible_v<F>
        && detail::has_is_transparent<F>
        && detail::accepts_all_key_flavors_equal<F, TChar>;

    /// \brief A three-way comparison function object over the key flavors.
    ///
    /// Not consumed by any standard container -- provided because a
    /// three-way comparison is strictly more informative than a `less`, is the
    /// natural spelling for a user-defined operator<=>, and lets a single call
    /// answer "less, equal, or greater" during a binary search.
    ///
    /// The result is required only to be convertible to std::weak_ordering:
    /// case-sensitive comparators legitimately return std::strong_ordering
    /// (which converts), and case-insensitive ones must return
    /// std::weak_ordering because distinct strings can be equivalent.
    template<typename F, typename TChar>
    concept transparent_ctsv_three_way =
        std_char<TChar>
        && std::is_nothrow_default_constructible_v<F>
        && detail::has_is_transparent<F>
        && detail::accepts_all_key_flavors_three_way<F, TChar>;

    /// \brief A hasher suitable as the Hash parameter of an unordered
    /// associative container keyed on basic_ct_string_view<TChar, *>.
    ///
    /// The result type must be EXACTLY std::size_t (not merely convertible):
    /// a narrowing or widening conversion in the hash path is always a bug.
    /// All invocations must be noexcept -- an unordered container that throws
    /// from its hasher mid-rehash is left in an unhappy state.
    template<typename F, typename TChar>
    concept transparent_ctsv_hash =
        std_char<TChar>
        && std::is_nothrow_default_constructible_v<F>
        && detail::has_is_transparent<F>
        && detail::accepts_all_key_flavors_hash<F, TChar>;

    /// \brief The pairing constraint for unordered containers: the hasher and
    /// the equality predicate must agree about case.
    ///
    /// The unordered-container invariant is that equal keys hash equally.
    /// Pairing a case-insensitive equality predicate with a case-sensitive
    /// hasher violates it catastrophically and SILENTLY -- "Ace" and "ACE"
    /// would be "equal" yet land in different buckets, so the container would
    /// happily hold both. That class of bug is essentially undebuggable, so it
    /// is rejected at compile time here.
    ///
    /// \remarks This cannot verify that two independently written function
    /// objects genuinely agree; it verifies the declared intent via
    /// ctsv_folds_case. That is enough to catch every realistic mistake,
    /// because the function objects in this header declare it correctly and
    /// anything that does not declare it defaults to "case-sensitive".
    template<typename THash, typename TEq, typename TChar>
    concept consistent_ctsv_hash_equal =
        transparent_ctsv_hash<THash, TChar>
        && transparent_ctsv_equal_to<TEq, TChar>
        && (ctsv_folds_case_v<THash> == ctsv_folds_case_v<TEq>);

    // =====================================================================
    // Implementation helpers
    // =====================================================================

    namespace detail
    {
        /// \brief FNV-1a parameters selected by the width of std::size_t, so
        /// the hash is well-defined on 32-bit targets rather than silently
        /// truncating the 64-bit constants.
        template<std::size_t SIZE_T_WIDTH>
        struct fnv1a_params;

        template<>
        struct fnv1a_params<8>
        {
            static constexpr std::size_t offset_basis = 14695981039346656037ULL;
            static constexpr std::size_t prime        = 1099511628211ULL;
        };

        template<>
        struct fnv1a_params<4>
        {
            static constexpr std::size_t offset_basis = 2166136261U;
            static constexpr std::size_t prime        = 16777619U;
        };

        using fnv1a = fnv1a_params<sizeof(std::size_t)>;

        /// \brief The unsigned integer type with the same width as TChar.
        /// \remarks Used instead of std::make_unsigned_t both because
        /// make_unsigned's applicability to char8_t has been a moving target
        /// across implementations and because being explicit about width makes
        /// the hash's value stable across platforms with the same size_t.
        template<std_char TChar>
        using ctsv_uchar_t =
            std::conditional_t<sizeof(TChar) == 1, std::uint8_t,
            std::conditional_t<sizeof(TChar) == 2, std::uint16_t, std::uint32_t>>;

        /// \brief constexpr FNV-1a over the CODE UNITS of \p sv, optionally
        /// ASCII-case-folded.
        ///
        /// \tparam FOLD_CASE when true, each code unit is passed through
        /// ascii_to_lower_fn before being mixed in. Because the folding is the
        /// same per-code-unit map used by ctsv_ci_equal_to, the
        /// "equal keys hash equally" invariant holds by construction.
        ///
        /// \remarks Iterates code units, NOT bytes: a reinterpret_cast to
        /// unsigned char would be both non-constexpr and endian-dependent.
        /// The consequence is that the hash of a wide string differs from the
        /// hash of its UTF-8 transcoding, which is fine -- they are different
        /// key types and never share a container.
        ///
        /// \remarks This is deliberately NOT std::hash. std::hash is not
        /// constexpr, and a constexpr hash is required for compile-time
        /// container construction and for static_assert-based testing.
        template<std_char TChar, bool FOLD_CASE>
        [[nodiscard]] constexpr std::size_t fnv1a_fold(
            const std::basic_string_view<TChar> sv) noexcept
        {
            constexpr auto folder = ascii_to_lower_fn<TChar>{};
            std::size_t hash = fnv1a::offset_basis;
            for (const TChar raw : sv)
            {
                const TChar unit = FOLD_CASE ? folder(raw) : raw;
                hash ^= static_cast<std::size_t>(static_cast<ctsv_uchar_t<TChar>>(unit));
                hash *= fnv1a::prime;
            }
            return hash;
        }
    }

    // =====================================================================
    // The function objects
    // =====================================================================
    //
    // Every one of them follows the same shape:
    //   * templated on TChar (the fold and the string_view type both depend
    //     on it, so there is no useful `void` "deduce everything" flavor);
    //   * a single constrained operator() covering the whole pairing matrix;
    //   * conditional noexcept driven by nothrow_ctsv_lookup_key;
    //   * `is_transparent` (gating heterogeneous lookup) and `folds_case`
    //     (gating hash/equality pairing) tags.

    /// \brief Case-sensitive, transparent `less` over basic_ct_string_view keys.
    ///
    /// Ordering is ordinary lexicographic comparison by code unit, identical
    /// to std::basic_string_view's own operator<=>.
    ///
    /// \code
    ///     ct_cstring_view_map<int> m{ {"beta"_ctsv, 2}, {"alpha"_ctsv, 1} };
    ///     auto it = m.find(std::string_view{"alpha"});   // heterogeneous
    /// \endcode
    template<std_char TChar>
    struct ctsv_less final
    {
        using char_type   = TChar;
        using std_sv_type = std::basic_string_view<TChar>;
        /// \brief Gates C++14 heterogeneous lookup in ordered containers.
        using is_transparent = void;
        /// \brief Declares that this comparator does NOT fold case.
        using folds_case = std::false_type;

        /// \brief Orders \p lhs before \p rhs lexicographically by code unit.
        template<ctsv_lookup_key<TChar> L, ctsv_lookup_key<TChar> R>
        [[nodiscard]] constexpr bool operator()(const L& lhs, const R& rhs) const
            noexcept(nothrow_ctsv_lookup_key<L, TChar> && nothrow_ctsv_lookup_key<R, TChar>)
        {
            const std_sv_type lv = lhs;
            const std_sv_type rv = rhs;
            return lv < rv;
        }
    };

    /// \brief ASCII-case-insensitive, transparent `less`.
    ///
    /// Folds DOWN, so "Z" orders AFTER "_" (see the header comment). Uses a
    /// std::ranges projection rather than a transform_view so that no
    /// intermediate view object is formed on either side; nothing is allocated.
    ///
    /// \code
    ///     ct_cstring_view_ci_map<int> m;
    ///     m["Ace"_ctsv] = 14;
    ///     m.at("ACE");                       // 14
    ///     m.at(std::string{"ace"});          // 14
    /// \endcode
    template<std_char TChar>
    struct ctsv_ci_less final
    {
        using char_type   = TChar;
        using std_sv_type = std::basic_string_view<TChar>;
        using is_transparent = void;
        /// \brief Declares that this comparator folds case; pairs only with
        /// other case-folding function objects.
        using folds_case = std::true_type;

        /// \brief Orders \p lhs before \p rhs lexicographically, comparing
        /// ASCII-lower-folded code units.
        template<ctsv_lookup_key<TChar> L, ctsv_lookup_key<TChar> R>
        [[nodiscard]] constexpr bool operator()(const L& lhs, const R& rhs) const
            noexcept(nothrow_ctsv_lookup_key<L, TChar> && nothrow_ctsv_lookup_key<R, TChar>)
        {
            const std_sv_type lv = lhs;
            const std_sv_type rv = rhs;
            // std::ranges::less compares two ELEMENTS (it requires
            // totally_ordered_with); it has no idea how to order two RANGES.
            // Ordering ranges element-by-element is
            // std::ranges::lexicographical_compare's job, and the case folding
            // is applied as a PROJECTION -- same result as piping both sides
            // through views::ascii_lower, but no view object is materialized.
            return std::ranges::lexicographical_compare(lv, rv,
                std::ranges::less{},
                ascii_to_lower_fn<TChar>{}, ascii_to_lower_fn<TChar>{});
        }
    };

    /// \brief Case-sensitive, transparent `equal_to` over basic_ct_string_view
    /// keys. Pairs with ctsv_hash.
    template<std_char TChar>
    struct ctsv_equal_to final
    {
        using char_type   = TChar;
        using std_sv_type = std::basic_string_view<TChar>;
        /// \brief Gates C++20 heterogeneous lookup in unordered containers
        /// (which additionally require a transparent hasher).
        using is_transparent = void;
        using folds_case = std::false_type;

        /// \brief Tests \p lhs and \p rhs for exact code-unit equality.
        template<ctsv_lookup_key<TChar> L, ctsv_lookup_key<TChar> R>
        [[nodiscard]] constexpr bool operator()(const L& lhs, const R& rhs) const
            noexcept(nothrow_ctsv_lookup_key<L, TChar> && nothrow_ctsv_lookup_key<R, TChar>)
        {
            const std_sv_type lv = lhs;
            const std_sv_type rv = rhs;
            return lv == rv;
        }
    };

    /// \brief ASCII-case-insensitive, transparent `equal_to`. Pairs with
    /// ctsv_ci_hash (and is rejected at compile time if paired with ctsv_hash).
    template<std_char TChar>
    struct ctsv_ci_equal_to final
    {
        using char_type   = TChar;
        using std_sv_type = std::basic_string_view<TChar>;
        using is_transparent = void;
        using folds_case = std::true_type;

        /// \brief Tests \p lhs and \p rhs for equality of ASCII-lower-folded
        /// code units.
        template<ctsv_lookup_key<TChar> L, ctsv_lookup_key<TChar> R>
        [[nodiscard]] constexpr bool operator()(const L& lhs, const R& rhs) const
            noexcept(nothrow_ctsv_lookup_key<L, TChar> && nothrow_ctsv_lookup_key<R, TChar>)
        {
            const std_sv_type lv = lhs;
            const std_sv_type rv = rhs;
            // ASCII folding is a per-code-unit map, so it cannot change the
            // length; an early size check is therefore both correct and the
            // cheapest possible rejection.
            return lv.size() == rv.size()
                && std::ranges::equal(lv, rv, std::ranges::equal_to{},
                    ascii_to_lower_fn<TChar>{}, ascii_to_lower_fn<TChar>{});
        }
    };

    /// \brief Case-sensitive, transparent three-way comparison. Yields
    /// std::strong_ordering: equivalent implies genuinely equal.
    template<std_char TChar>
    struct ctsv_three_way final
    {
        using char_type   = TChar;
        using std_sv_type = std::basic_string_view<TChar>;
        using is_transparent = void;
        using folds_case = std::false_type;

        /// \brief Three-way compares \p lhs and \p rhs by code unit.
        template<ctsv_lookup_key<TChar> L, ctsv_lookup_key<TChar> R>
        [[nodiscard]] constexpr std::strong_ordering operator()(const L& lhs, const R& rhs) const
            noexcept(nothrow_ctsv_lookup_key<L, TChar> && nothrow_ctsv_lookup_key<R, TChar>)
        {
            const std_sv_type lv = lhs;
            const std_sv_type rv = rhs;
            return lv <=> rv;
        }
    };

    /// \brief ASCII-case-insensitive, transparent three-way comparison.
    ///
    /// Yields std::weak_ordering, NOT std::strong_ordering: "abc" and "ABC"
    /// are equivalent without being equal, which is exactly what distinguishes
    /// a weak ordering from a strong one.
    ///
    /// \remarks std::lexicographical_compare_three_way takes no projection
    /// parameter, so this one genuinely does form two lazy transform_views via
    /// views::ascii_lower. They are still allocation-free.
    template<std_char TChar>
    struct ctsv_ci_three_way final
    {
        using char_type   = TChar;
        using std_sv_type = std::basic_string_view<TChar>;
        using is_transparent = void;
        using folds_case = std::true_type;

        /// \brief Three-way compares \p lhs and \p rhs by ASCII-lower-folded
        /// code unit.
        template<ctsv_lookup_key<TChar> L, ctsv_lookup_key<TChar> R>
        [[nodiscard]] constexpr std::weak_ordering operator()(const L& lhs, const R& rhs) const
            noexcept(nothrow_ctsv_lookup_key<L, TChar> && nothrow_ctsv_lookup_key<R, TChar>)
        {
            const std_sv_type lv = lhs;
            const std_sv_type rv = rhs;
            auto lower_lhs = views::ascii_lower(lv);
            auto lower_rhs = views::ascii_lower(rv);
            const auto result = std::lexicographical_compare_three_way(
                lower_lhs.begin(), lower_lhs.end(),
                lower_rhs.begin(), lower_rhs.end());
            return static_cast<std::weak_ordering>(result);
        }
    };

    /// \brief Case-sensitive, transparent, CONSTEXPR hasher (FNV-1a over code
    /// units). Pairs with ctsv_equal_to.
    ///
    /// \remarks This is intentionally not std::hash. std::hash is not
    /// constexpr, which rules out compile-time container construction and
    /// static_assert-based testing. The trade-off is that a value hashed here
    /// does not agree with std::hash<std::basic_string_view<TChar>> -- which is
    /// harmless, because a container uses exactly one hasher throughout. If you
    /// want the standard library's hash (e.g. to match an existing
    /// std::unordered_map<std::string, V>), pass
    /// std::hash<basic_ct_string_view<TChar, B>> explicitly: it is transparent,
    /// and ctsv_folds_case defaults it to "case-sensitive", so it pairs
    /// correctly with std::equal_to<> and with ctsv_equal_to.
    template<std_char TChar>
    struct ctsv_hash final
    {
        using char_type   = TChar;
        using std_sv_type = std::basic_string_view<TChar>;
        /// \brief Gates C++20 heterogeneous lookup in unordered containers.
        using is_transparent = void;
        using folds_case = std::false_type;

        /// \brief Hashes \p key by code unit.
        template<ctsv_lookup_key<TChar> K>
        [[nodiscard]] constexpr std::size_t operator()(const K& key) const
            noexcept(nothrow_ctsv_lookup_key<K, TChar>)
        {
            const std_sv_type kv = key;
            return detail::fnv1a_fold<TChar, false>(kv);
        }
    };

    /// \brief ASCII-case-insensitive, transparent, CONSTEXPR hasher.
    ///
    /// Folds each code unit with exactly the same map ctsv_ci_equal_to uses,
    /// so the "equal keys hash equally" invariant holds by construction rather
    /// than by convention.
    ///
    /// \code
    ///     static_assert(ctsv_ci_hash<char>{}("Ace") == ctsv_ci_hash<char>{}("ACE"));
    /// \endcode
    template<std_char TChar>
    struct ctsv_ci_hash final
    {
        using char_type   = TChar;
        using std_sv_type = std::basic_string_view<TChar>;
        using is_transparent = void;
        using folds_case = std::true_type;

        /// \brief Hashes \p key by ASCII-lower-folded code unit.
        template<ctsv_lookup_key<TChar> K>
        [[nodiscard]] constexpr std::size_t operator()(const K& key) const
            noexcept(nothrow_ctsv_lookup_key<K, TChar>)
        {
            const std_sv_type kv = key;
            return detail::fnv1a_fold<TChar, true>(kv);
        }
    };

    // =====================================================================
    // Conformance self-checks
    // =====================================================================
    //
    // Each family is checked against its concept for EVERY std_char type. If
    // one of these ever fails, the comparator and the concept have drifted
    // apart -- which is precisely the failure mode the concepts exist to
    // prevent.

#define CPS_CTSV_CHECK_COMPARATORS_FOR(CHARTYPE)                                             \
    static_assert(transparent_ctsv_less<ctsv_less<CHARTYPE>, CHARTYPE>);                     \
    static_assert(transparent_ctsv_less<ctsv_ci_less<CHARTYPE>, CHARTYPE>);                  \
    static_assert(transparent_ctsv_equal_to<ctsv_equal_to<CHARTYPE>, CHARTYPE>);             \
    static_assert(transparent_ctsv_equal_to<ctsv_ci_equal_to<CHARTYPE>, CHARTYPE>);          \
    static_assert(transparent_ctsv_three_way<ctsv_three_way<CHARTYPE>, CHARTYPE>);           \
    static_assert(transparent_ctsv_three_way<ctsv_ci_three_way<CHARTYPE>, CHARTYPE>);        \
    static_assert(transparent_ctsv_hash<ctsv_hash<CHARTYPE>, CHARTYPE>);                     \
    static_assert(transparent_ctsv_hash<ctsv_ci_hash<CHARTYPE>, CHARTYPE>);                  \
    static_assert(consistent_ctsv_hash_equal<ctsv_hash<CHARTYPE>,                            \
        ctsv_equal_to<CHARTYPE>, CHARTYPE>);                                                 \
    static_assert(consistent_ctsv_hash_equal<ctsv_ci_hash<CHARTYPE>,                         \
        ctsv_ci_equal_to<CHARTYPE>, CHARTYPE>);                                              \
    static_assert(!consistent_ctsv_hash_equal<ctsv_ci_hash<CHARTYPE>,                        \
        ctsv_equal_to<CHARTYPE>, CHARTYPE>, "a ci hasher must not pair with a cs equality"); \
    static_assert(!consistent_ctsv_hash_equal<ctsv_hash<CHARTYPE>,                           \
        ctsv_ci_equal_to<CHARTYPE>, CHARTYPE>, "a cs hasher must not pair with a ci equality")

    CPS_CTSV_CHECK_COMPARATORS_FOR(char);
    CPS_CTSV_CHECK_COMPARATORS_FOR(wchar_t);
    CPS_CTSV_CHECK_COMPARATORS_FOR(char8_t);
    CPS_CTSV_CHECK_COMPARATORS_FOR(char16_t);
    CPS_CTSV_CHECK_COMPARATORS_FOR(char32_t);

#undef CPS_CTSV_CHECK_COMPARATORS_FOR

    // The standard's own transparent function objects must also qualify --
    // basic_ct_string_view supplies heterogeneous operator== / operator<=>,
    // so std::less<> and std::equal_to<> work out of the box. If either of
    // these ever fails, the comparison-operator templates in ct_string_view.hpp
    // have regressed.
    static_assert(transparent_ctsv_less<std::less<>, char>);
    static_assert(transparent_ctsv_less<std::less<>, wchar_t>);
    static_assert(transparent_ctsv_equal_to<std::equal_to<>, char>);
    static_assert(transparent_ctsv_equal_to<std::equal_to<>, char32_t>);

    // ...and the NON-transparent ones must not.
    static_assert(!transparent_ctsv_less<std::less<std::string_view>, char>,
        "std::less<T> is not transparent and must be rejected");
    static_assert(!transparent_ctsv_equal_to<std::equal_to<std::string_view>, char>,
        "std::equal_to<T> is not transparent and must be rejected");

    // Case-folding tags: ours opt in explicitly, the standard's default to
    // case-sensitive, which is correct.
    static_assert(!ctsv_folds_case_v<std::less<>>);
    static_assert(!ctsv_folds_case_v<std::equal_to<>>);
    static_assert(!ctsv_folds_case_v<ctsv_less<char>>);
    static_assert(ctsv_folds_case_v<ctsv_ci_less<char>>);

    // -- Behavioural checks -----------------------------------------------
    namespace detail
    {
        inline constexpr auto ci_lt  = ctsv_ci_less<char>{};
        inline constexpr auto ci_cmp = ctsv_ci_three_way<char>{};
        inline constexpr auto ci_eq  = ctsv_ci_equal_to<char>{};
        inline constexpr auto cs_lt  = ctsv_less<char>{};

        // ci_less: case-insensitive, and genuinely lexicographic.
        static_assert(!ci_lt(std::string_view{"abc"}, std::string_view{"ABC"})
                   && !ci_lt(std::string_view{"ABC"}, std::string_view{"abc"}),
            "case must be irrelevant (equivalent => neither is less)");
        static_assert(ci_lt(std::string_view{"abc"}, std::string_view{"ABD"})
                  && !ci_lt(std::string_view{"ABD"}, std::string_view{"abc"}));
        static_assert(ci_lt(std::string_view{"Ab"}, std::string_view{"abc"})
                  && !ci_lt(std::string_view{"abc"}, std::string_view{"Ab"}),
            "a proper prefix orders before the longer range");
        static_assert(!ci_lt(std::string_view{""}, std::string_view{""}));
        static_assert(ci_lt(std::string_view{""}, std::string_view{"a"})
                  && !ci_lt(std::string_view{"a"}, std::string_view{""}));
        // Non-alpha code units pass through untouched. '_' is 0x5F: it sits
        // between 'Z' (0x5A) and 'a' (0x61), so the fold DIRECTION is
        // OBSERVABLE. We fold DOWN, hence "Z" -> "z" (0x7A) and "z" > "_".
        static_assert(!ci_lt(std::string_view{"Z"}, std::string_view{"_"})
                   && ci_lt(std::string_view{"_"}, std::string_view{"a"}));

        // The case-sensitive comparator must NOT fold.
        static_assert(cs_lt(std::string_view{"ABC"}, std::string_view{"abc"}),
            "'A' (0x41) < 'a' (0x61) when case is respected");

        // ci_three_way must agree with ci_less.
        static_assert(ci_cmp(std::string_view{"abc"}, std::string_view{"ABC"})
            == std::weak_ordering::equivalent);
        static_assert(ci_cmp(std::string_view{"abc"}, std::string_view{"ABD"})
            == std::weak_ordering::less);
        static_assert(ci_cmp(std::string_view{"ABD"}, std::string_view{"abc"})
            == std::weak_ordering::greater);
        static_assert(ci_cmp(std::string_view{"Ab"}, std::string_view{"abc"})
            == std::weak_ordering::less);
        static_assert(ci_cmp(std::string_view{""}, std::string_view{""})
            == std::weak_ordering::equivalent);

        [[nodiscard]] constexpr bool ctsv_comparators_agree(
            const std::string_view l, const std::string_view r) noexcept
        {
            return (ci_lt(l, r) == (ci_cmp(l, r) == std::weak_ordering::less))
                && (ci_eq(l, r) == (ci_cmp(l, r) == std::weak_ordering::equivalent));
        }

        static_assert(ctsv_comparators_agree("abc", "ABC"));
        static_assert(ctsv_comparators_agree("abc", "ABD"));
        static_assert(ctsv_comparators_agree("ABD", "abc"));
        static_assert(ctsv_comparators_agree("Ab", "abc"));
        static_assert(ctsv_comparators_agree("Z", "_"));
        static_assert(ctsv_comparators_agree("_", "a"));

        // Hashing: case-insensitively equal keys MUST hash equally, and the
        // case-sensitive hasher must actually distinguish them.
        static_assert(ctsv_ci_hash<char>{}(std::string_view{"Ace"})
                   == ctsv_ci_hash<char>{}(std::string_view{"ACE"}));
        static_assert(ctsv_ci_hash<char>{}(std::string_view{"Ace"})
                   == ctsv_ci_hash<char>{}(std::string_view{"ace"}));
        static_assert(ctsv_hash<char>{}(std::string_view{"Ace"})
                   != ctsv_hash<char>{}(std::string_view{"ACE"}));
        static_assert(ctsv_ci_hash<char>{}(std::string_view{"Ace"})
                   != ctsv_ci_hash<char>{}(std::string_view{"Acf"}));
    }
}

#endif // CPS_CT_STR_CTSV_COMPARATORS_HPP


