// ---------------------------------------------------------------------------
// ctsv_containers.hpp -- associative container wrappers keyed on
// basic_ct_string_view, with genuinely convenient heterogeneous lookup.
// ---------------------------------------------------------------------------
//
// WHY WRAPPERS RATHER THAN ALIASES
//
// This header replaces an earlier family of plain alias templates
// (basic_ct_string_view_map, ..._unordered_set, and their per-flavor
// spellings). Those aliases pre-wired the right comparator and hasher, which
// made std::map::find and std::unordered_map::find accept a std::string_view
// -- and then stopped, because the standard's heterogeneous-lookup support is
// conspicuously incomplete:
//
//   * std::map::at / std::unordered_map::at have NO heterogeneous overload at
//     all. They take const key_type&. Since a basic_ct_string_view cannot be
//     constructed from a std::string_view (by design -- that would forge the
//     compile-time provenance guarantee), `m.at(some_string_view)` simply does
//     not compile.
//   * operator[] likewise takes only the key type, but there the restriction
//     is CORRECT: operator[] inserts, and an inserted key must carry the
//     lifetime guarantee.
//   * contains/count/find are heterogeneous, but erase only became so in
//     C++23 and under awkward constraints.
//
// The net effect was that the single most-reached-for operation -- "look this
// string up and give me the value" -- was the one that didn't work. These
// wrappers fix that asymmetry, and encode the insert/lookup distinction in the
// type system:
//
//   INSERTING  (operator[], insert, emplace, try_emplace, ...)
//       requires a real basic_ct_string_view of a flavor from which key_type
//       is constructible. Nothing else can produce a key, so the container can
//       never come to hold a key whose storage it does not outlive.
//
//   LOOKING UP (at, at_if, find, contains, count, erase, lower_bound, ...)
//       takes std_sv_type BY VALUE. Every basic_ct_string_view (either
//       flavor), std::basic_string, basic_fixed_string and string literal
//       converts implicitly, so all of them just work at the call site.
//
// That asymmetry is the whole point of the design. It is enforced
// structurally, not by convention: key_type is simply not constructible from
// a string_view, so every insertion path fails to compile on its own.
//
// AT_IF
//
// at_if is the non-throwing sibling of at: it returns a pointer to the mapped
// value, or nullptr. It is the operation you actually want nine times out of
// ten (`if (const auto* p = m.at_if(name))`), and it avoids both the
// exception of at and the iterator/end() dance of find. The name matches the
// at_if on this project's static_flat_map.
//
// NOT PROVIDED: multimap / multiset wrappers. The superseded alias family had
// them and nothing ever used them. Adding them is mechanical -- the only real
// differences are that erase(key) may remove more than one element and that
// equal_range becomes interesting -- but they are omitted rather than shipped
// untested.
// ---------------------------------------------------------------------------
#ifndef CPS_CT_STR_CTSV_CONTAINERS_HPP
#define CPS_CT_STR_CTSV_CONTAINERS_HPP

#include "ctsv_comparators.hpp"
#include <concepts>
#include <cstddef>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace cps::ct_string
{
    namespace detail
    {
        /// \brief Builds the std::out_of_range thrown by the at() members.
        ///
        /// The key is interpolated into the message only for char, because
        /// there are no std::formatter / std::string conversions for
        /// char8_t, char16_t or char32_t views, and transcoding inside an
        /// error path is not worth the dependency.
        template<std_char TChar>
        [[noreturn]] void throw_ctsv_key_not_found(
            const char* const what, const std::basic_string_view<TChar> key)
        {
            if constexpr (std::same_as<TChar, char>)
            {
                std::string msg{what};
                msg.append(": key not found: \"").append(key).append("\"");
                throw std::out_of_range{msg};
            }
            else
            {
                (void)key;
                throw std::out_of_range{std::string{what} + ": key not found"};
            }
        }
    }

    // =====================================================================
    // Ordered map
    // =====================================================================

    /// \brief A std::map keyed on basic_ct_string_view<TChar, VALID_CSTR> with
    /// heterogeneous lookup extended to at / at_if / erase and every other
    /// read-only operation.
    ///
    /// \tparam TChar the character type.
    /// \tparam VALID_CSTR the key flavor: true for the null-termination-
    /// guaranteeing ct_cstring_view, false for ct_string_view.
    /// \tparam TValue the mapped type.
    /// \tparam TLess the ordering. Must satisfy transparent_ctsv_less, which
    /// rejects non-transparent comparators (they would silently disable
    /// heterogeneous lookup) and comparators that cannot handle every pairing
    /// of the key flavors with std::basic_string_view.
    ///
    /// \code
    ///     using namespace cps::ct_string::literals;
    ///
    ///     ct_cstring_view_ci_map<int> ranks;
    ///     ranks["Ace"_ctsv]   = 14;          // insert: needs a real ct view
    ///     ranks["King"_ctsv]  = 13;
    ///
    ///     ranks.at("ACE");                   // 14 -- from a string literal
    ///     ranks.at(std::string{"ace"});      // 14 -- from a std::string
    ///     if (const int* p = ranks.at_if("kInG")) { /* *p == 13 */ }
    ///     ranks.contains(std::string_view{"queen"});   // false
    ///
    ///     // ranks[std::string_view{"Ace"}] = 1;   // ILL-FORMED, by design:
    ///     // a string_view cannot supply the lifetime guarantee an inserted
    ///     // key must carry.
    /// \endcode
    template<std_char TChar, bool VALID_CSTR, typename TValue,
             transparent_ctsv_less<TChar> TLess = ctsv_less<TChar>>
    class basic_ctsv_map
        : private std::map<basic_ct_string_view<TChar, VALID_CSTR>, TValue, TLess>
    {
        using base_type = std::map<basic_ct_string_view<TChar, VALID_CSTR>, TValue, TLess>;

    public:
        /// \brief Whether the key flavor carries the C-string guarantee.
        static constexpr bool is_cstring = VALID_CSTR;

        using char_type       = TChar;
        using key_type        = basic_ct_string_view<TChar, VALID_CSTR>;
        /// \brief The opposite key flavor. Accepted by the inserting members
        /// only when key_type is constructible from it (i.e. only when doing
        /// so cannot forge a guarantee).
        using other_key_type  = basic_ct_string_view<TChar, !VALID_CSTR>;
        /// \brief The type every LOOKUP member accepts, by value.
        using std_sv_type     = std::basic_string_view<TChar>;
        using mapped_type     = TValue;
        using value_type      = typename base_type::value_type;
        using size_type       = typename base_type::size_type;
        using difference_type = typename base_type::difference_type;
        using key_compare     = TLess;
        using value_compare   = typename base_type::value_compare;
        using allocator_type  = typename base_type::allocator_type;
        using reference       = typename base_type::reference;
        using const_reference = typename base_type::const_reference;
        using pointer         = typename base_type::pointer;
        using const_pointer   = typename base_type::const_pointer;
        using iterator        = typename base_type::iterator;
        using const_iterator  = typename base_type::const_iterator;
        using reverse_iterator       = typename base_type::reverse_iterator;
        using const_reverse_iterator = typename base_type::const_reverse_iterator;
        using node_type       = typename base_type::node_type;
        using insert_return_type = typename base_type::insert_return_type;

        using base_type::base_type;

        // -- capacity / iteration (ordered containers are bidirectional) ---
        using base_type::empty;
        using base_type::size;
        using base_type::max_size;
        using base_type::clear;
        using base_type::begin;
        using base_type::end;
        using base_type::cbegin;
        using base_type::cend;
        using base_type::rbegin;
        using base_type::rend;
        using base_type::crbegin;
        using base_type::crend;
        using base_type::get_allocator;
        using base_type::key_comp;
        using base_type::value_comp;

        // -- insertion ------------------------------------------------------
        // insert/try_emplace/insert_or_assign are forwarded unchanged: each
        // either takes key_type outright or (for insert's converting overload)
        // is already constrained by the standard on
        // is_constructible_v<value_type, P&&>. Since key_type is not
        // constructible from a string_view, the "only a real ct view may be
        // inserted" policy enforces itself there.
        //
        // emplace/emplace_hint are the exception and are re-declared below.
        using base_type::insert;
        using base_type::insert_or_assign;
        using base_type::try_emplace;
        using base_type::extract;
        using base_type::merge;
        using base_type::swap;

        /// \brief Constructs an element in place from \p args.
        ///
        /// \remarks Constrained on std::constructible_from<value_type,
        /// TArgs...>, which the standard containers' emplace is NOT. That
        /// matters: without the constraint `m.emplace(std::string_view{"k"}, 1)`
        /// is a perfectly well-formed CALL that then fails several layers deep
        /// inside the standard library with an unreadable diagnostic. With it,
        /// the call simply does not participate in overload resolution -- which
        /// is both a far better error message and, unlike the deep failure, a
        /// statement of the insertion policy that a requires-expression can
        /// actually observe.
        template<typename... TArgs>
            requires std::constructible_from<value_type, TArgs...>
        std::pair<iterator, bool> emplace(TArgs&&... args)
        {
            return base_type::emplace(std::forward<TArgs>(args)...);
        }

        /// \brief Constructs an element in place near \p hint.
        /// \see emplace for the rationale behind the constraint.
        template<typename... TArgs>
            requires std::constructible_from<value_type, TArgs...>
        iterator emplace_hint(const_iterator hint, TArgs&&... args)
        {
            return base_type::emplace_hint(hint, std::forward<TArgs>(args)...);
        }

        /// \brief Inserts a default-constructed value if \p key is absent and
        /// returns a reference to the mapped value.
        /// \remarks Takes key_type, NOT std_sv_type: this member inserts.
        [[nodiscard]] mapped_type& operator[](const key_type key)
        {
            return base_type::operator[](key);
        }

        /// \brief operator[] accepting the opposite key flavor, available only
        /// when converting to key_type cannot forge a guarantee (i.e. when
        /// going from the C-string flavor to the non-C-string one).
        [[nodiscard]] mapped_type& operator[](const other_key_type key)
            requires std::constructible_from<key_type, other_key_type>
        {
            return base_type::operator[](key_type{key});
        }

        // -- lookup ----------------------------------------------------------
        //
        // These are declared here INSTEAD OF being pulled in with
        // `using base_type::find;` and friends. Forwarding the base versions
        // as well would make `m.find(std::string_view{...})` ambiguous: the
        // base's transparent template and the std_sv_type overload below would
        // both be exact matches. Declaring only these keeps overload
        // resolution trivial, and key_type converts implicitly to std_sv_type
        // so passing a ct view still works.
        //
        // They are noexcept because transparent_ctsv_less guarantees the
        // comparator is noexcept for every key flavor, and a lookup in a
        // std::map neither allocates nor copies a mapped value.

        /// \brief Finds \p key. \returns an iterator, or end() if absent.
        [[nodiscard]] iterator find(const std_sv_type key) noexcept
        {
            return base_type::find(key);
        }

        /// \copydoc find
        [[nodiscard]] const_iterator find(const std_sv_type key) const noexcept
        {
            return base_type::find(key);
        }

        /// \brief \returns true iff \p key is present.
        [[nodiscard]] bool contains(const std_sv_type key) const noexcept
        {
            return base_type::contains(key);
        }

        /// \brief \returns 1 if \p key is present, otherwise 0.
        [[nodiscard]] size_type count(const std_sv_type key) const noexcept
        {
            return base_type::count(key);
        }

        /// \brief \returns an iterator to the first element not ordered before
        /// \p key.
        [[nodiscard]] iterator lower_bound(const std_sv_type key) noexcept
        {
            return base_type::lower_bound(key);
        }

        /// \copydoc lower_bound
        [[nodiscard]] const_iterator lower_bound(const std_sv_type key) const noexcept
        {
            return base_type::lower_bound(key);
        }

        /// \brief \returns an iterator to the first element ordered after \p key.
        [[nodiscard]] iterator upper_bound(const std_sv_type key) noexcept
        {
            return base_type::upper_bound(key);
        }

        /// \copydoc upper_bound
        [[nodiscard]] const_iterator upper_bound(const std_sv_type key) const noexcept
        {
            return base_type::upper_bound(key);
        }

        /// \brief \returns the pair {lower_bound(key), upper_bound(key)}.
        [[nodiscard]] std::pair<iterator, iterator> equal_range(const std_sv_type key) noexcept
        {
            return base_type::equal_range(key);
        }

        /// \copydoc equal_range
        [[nodiscard]] std::pair<const_iterator, const_iterator>
            equal_range(const std_sv_type key) const noexcept
        {
            return base_type::equal_range(key);
        }

        /// \brief \returns a reference to the value mapped to \p key.
        /// \throws std::out_of_range if \p key is absent.
        /// \remarks This is the member the superseded alias family could not
        /// offer: std::map::at has no heterogeneous overload.
        [[nodiscard]] mapped_type& at(const std_sv_type key)
        {
            if (const auto it = base_type::find(key); it != base_type::end())
            {
                return it->second;
            }
            detail::throw_ctsv_key_not_found<TChar>("basic_ctsv_map::at", key);
        }

        /// \copydoc at
        [[nodiscard]] const mapped_type& at(const std_sv_type key) const
        {
            if (const auto it = base_type::find(key); it != base_type::cend())
            {
                return it->second;
            }
            detail::throw_ctsv_key_not_found<TChar>("basic_ctsv_map::at", key);
        }

        /// \brief The non-throwing at: \returns a pointer to the value mapped
        /// to \p key, or nullptr if \p key is absent.
        [[nodiscard]] mapped_type* at_if(const std_sv_type key) noexcept
        {
            const auto it = base_type::find(key);
            return it != base_type::end() ? std::addressof(it->second) : nullptr;
        }

        /// \copydoc at_if
        [[nodiscard]] const mapped_type* at_if(const std_sv_type key) const noexcept
        {
            const auto it = base_type::find(key);
            return it != base_type::cend() ? std::addressof(it->second) : nullptr;
        }

        // -- erasure ---------------------------------------------------------

        /// \brief Erases the element at \p pos. \returns the following iterator.
        iterator erase(const_iterator pos) { return base_type::erase(pos); }

        /// \brief Erases [first, last). \returns the following iterator.
        iterator erase(const_iterator first, const_iterator last)
        {
            return base_type::erase(first, last);
        }

        /// \brief Erases the element matching \p key, if any.
        /// \returns the number erased (0 or 1).
        /// \remarks Implemented as find-then-erase rather than by forwarding
        /// to the base's erase(K&&), whose heterogeneous form is C++23-only
        /// and differently constrained across implementations.
        size_type erase(const std_sv_type key) noexcept
        {
            const auto it = base_type::find(key);
            if (it == base_type::end()) { return 0; }
            base_type::erase(it);
            return 1;
        }

        // -- interop ---------------------------------------------------------

        /// \brief Escape hatch to the wrapped std::map.
        /// \remarks Use when a standard algorithm or API genuinely needs the
        /// concrete container type. Mutating through it bypasses nothing --
        /// the insertion policy is enforced by key_type itself.
        [[nodiscard]] base_type& base() noexcept { return *this; }
        /// \copydoc base
        [[nodiscard]] const base_type& base() const noexcept { return *this; }

        [[nodiscard]] bool operator==(const basic_ctsv_map& other) const
            requires std::equality_comparable<mapped_type>
        {
            return base() == other.base();
        }
    };

    // =====================================================================
    // Ordered set
    // =====================================================================

    /// \brief A std::set of basic_ct_string_view<TChar, VALID_CSTR> whose
    /// lookup members accept anything convertible to std::basic_string_view.
    /// \see basic_ctsv_map for the insert/lookup asymmetry rationale.
    ///
    /// \code
    ///     ct_cstring_view_ci_set names;
    ///     names.insert("Alice"_ctsv);
    ///     names.contains("ALICE");                    // true
    ///     names.erase(std::string{"alice"});          // 1
    /// \endcode
    template<std_char TChar, bool VALID_CSTR,
             transparent_ctsv_less<TChar> TLess = ctsv_less<TChar>>
    class basic_ctsv_set
        : private std::set<basic_ct_string_view<TChar, VALID_CSTR>, TLess>
    {
        using base_type = std::set<basic_ct_string_view<TChar, VALID_CSTR>, TLess>;

    public:
        static constexpr bool is_cstring = VALID_CSTR;

        using char_type       = TChar;
        using key_type        = basic_ct_string_view<TChar, VALID_CSTR>;
        using other_key_type  = basic_ct_string_view<TChar, !VALID_CSTR>;
        using std_sv_type     = std::basic_string_view<TChar>;
        using value_type      = typename base_type::value_type;
        using size_type       = typename base_type::size_type;
        using difference_type = typename base_type::difference_type;
        using key_compare     = TLess;
        using value_compare   = typename base_type::value_compare;
        using allocator_type  = typename base_type::allocator_type;
        using reference       = typename base_type::reference;
        using const_reference = typename base_type::const_reference;
        using pointer         = typename base_type::pointer;
        using const_pointer   = typename base_type::const_pointer;
        using iterator        = typename base_type::iterator;
        using const_iterator  = typename base_type::const_iterator;
        using reverse_iterator       = typename base_type::reverse_iterator;
        using const_reverse_iterator = typename base_type::const_reverse_iterator;
        using node_type       = typename base_type::node_type;
        using insert_return_type = typename base_type::insert_return_type;

        using base_type::base_type;

        using base_type::empty;
        using base_type::size;
        using base_type::max_size;
        using base_type::clear;
        using base_type::begin;
        using base_type::end;
        using base_type::cbegin;
        using base_type::cend;
        using base_type::rbegin;
        using base_type::rend;
        using base_type::crbegin;
        using base_type::crend;
        using base_type::get_allocator;
        using base_type::key_comp;
        using base_type::value_comp;

        using base_type::insert;
        using base_type::extract;
        using base_type::merge;
        using base_type::swap;

        /// \brief Constructs an element in place from \p args.
        /// \see basic_ctsv_map::emplace for the rationale behind the
        /// constraint the standard containers' emplace lacks.
        template<typename... TArgs>
            requires std::constructible_from<value_type, TArgs...>
        std::pair<iterator, bool> emplace(TArgs&&... args)
        {
            return base_type::emplace(std::forward<TArgs>(args)...);
        }

        /// \brief Constructs an element in place near \p hint.
        template<typename... TArgs>
            requires std::constructible_from<value_type, TArgs...>
        iterator emplace_hint(const_iterator hint, TArgs&&... args)
        {
            return base_type::emplace_hint(hint, std::forward<TArgs>(args)...);
        }

        /// \brief Finds \p key. \returns an iterator, or end() if absent.
        [[nodiscard]] iterator find(const std_sv_type key) const noexcept
        {
            return base_type::find(key);
        }

        /// \brief \returns true iff \p key is present.
        [[nodiscard]] bool contains(const std_sv_type key) const noexcept
        {
            return base_type::contains(key);
        }

        /// \brief \returns 1 if \p key is present, otherwise 0.
        [[nodiscard]] size_type count(const std_sv_type key) const noexcept
        {
            return base_type::count(key);
        }

        /// \brief \returns an iterator to the first element not ordered before \p key.
        [[nodiscard]] iterator lower_bound(const std_sv_type key) const noexcept
        {
            return base_type::lower_bound(key);
        }

        /// \brief \returns an iterator to the first element ordered after \p key.
        [[nodiscard]] iterator upper_bound(const std_sv_type key) const noexcept
        {
            return base_type::upper_bound(key);
        }

        /// \brief \returns the pair {lower_bound(key), upper_bound(key)}.
        [[nodiscard]] std::pair<iterator, iterator>
            equal_range(const std_sv_type key) const noexcept
        {
            return base_type::equal_range(key);
        }

        /// \brief Erases the element at \p pos. \returns the following iterator.
        iterator erase(const_iterator pos) { return base_type::erase(pos); }

        /// \brief Erases [first, last). \returns the following iterator.
        iterator erase(const_iterator first, const_iterator last)
        {
            return base_type::erase(first, last);
        }

        /// \brief Erases the element matching \p key, if any.
        /// \returns the number erased (0 or 1).
        size_type erase(const std_sv_type key) noexcept
        {
            const auto it = base_type::find(key);
            if (it == base_type::end()) { return 0; }
            base_type::erase(it);
            return 1;
        }

        /// \brief Escape hatch to the wrapped std::set.
        [[nodiscard]] base_type& base() noexcept { return *this; }
        /// \copydoc base
        [[nodiscard]] const base_type& base() const noexcept { return *this; }

        [[nodiscard]] bool operator==(const basic_ctsv_set& other) const
        {
            return base() == other.base();
        }
    };

    // =====================================================================
    // Unordered map
    // =====================================================================

    /// \brief A std::unordered_map keyed on basic_ct_string_view<TChar,
    /// VALID_CSTR> with heterogeneous lookup extended to at / at_if / erase.
    ///
    /// \tparam THash the hasher; must satisfy transparent_ctsv_hash.
    /// \tparam TEq the equality predicate; must satisfy
    /// transparent_ctsv_equal_to.
    ///
    /// The requires-clause additionally enforces consistent_ctsv_hash_equal:
    /// the hasher and the equality predicate must agree about case. Mixing
    /// them is the single nastiest bug available in this area -- a
    /// case-insensitive equality with a case-sensitive hash produces a
    /// container that silently holds both "Ace" and "ACE" while reporting
    /// them equal -- so it is rejected at compile time.
    ///
    /// \code
    ///     ct_cstring_view_ci_unordered_map<int> m;
    ///     m["Ace"_ctsv] = 14;
    ///     m.at("ACE");                                  // 14
    ///     m.at_if(std::string_view{"ace"});             // -> &14
    /// \endcode
    template<std_char TChar, bool VALID_CSTR, typename TValue,
             transparent_ctsv_hash<TChar>     THash = ctsv_hash<TChar>,
             transparent_ctsv_equal_to<TChar> TEq   = ctsv_equal_to<TChar>>
        requires consistent_ctsv_hash_equal<THash, TEq, TChar>
    class basic_ctsv_unordered_map
        : private std::unordered_map<basic_ct_string_view<TChar, VALID_CSTR>, TValue, THash, TEq>
    {
        using base_type =
            std::unordered_map<basic_ct_string_view<TChar, VALID_CSTR>, TValue, THash, TEq>;

    public:
        static constexpr bool is_cstring = VALID_CSTR;
        /// \brief Whether this container's hasher and equality fold ASCII case.
        static constexpr bool folds_case = ctsv_folds_case_v<THash>;

        using char_type       = TChar;
        using key_type        = basic_ct_string_view<TChar, VALID_CSTR>;
        using other_key_type  = basic_ct_string_view<TChar, !VALID_CSTR>;
        using std_sv_type     = std::basic_string_view<TChar>;
        using mapped_type     = TValue;
        using value_type      = typename base_type::value_type;
        using size_type       = typename base_type::size_type;
        using difference_type = typename base_type::difference_type;
        using hasher          = THash;
        using key_equal       = TEq;
        using allocator_type  = typename base_type::allocator_type;
        using reference       = typename base_type::reference;
        using const_reference = typename base_type::const_reference;
        using pointer         = typename base_type::pointer;
        using const_pointer   = typename base_type::const_pointer;
        using iterator        = typename base_type::iterator;
        using const_iterator  = typename base_type::const_iterator;
        using local_iterator  = typename base_type::local_iterator;
        using const_local_iterator = typename base_type::const_local_iterator;
        using node_type       = typename base_type::node_type;
        using insert_return_type = typename base_type::insert_return_type;

        using base_type::base_type;

        // NOTE: no rbegin/rend/crbegin/crend. std::unordered_map is a forward
        // range and has no reverse iterators; the superseded draft of this
        // wrapper tried to forward them and did not compile.
        using base_type::empty;
        using base_type::size;
        using base_type::max_size;
        using base_type::clear;
        using base_type::begin;
        using base_type::end;
        using base_type::cbegin;
        using base_type::cend;
        using base_type::get_allocator;
        using base_type::hash_function;
        using base_type::key_eq;

        // -- bucket interface -------------------------------------------------
        using base_type::bucket_count;
        using base_type::max_bucket_count;
        using base_type::bucket_size;
        using base_type::bucket;
        using base_type::load_factor;
        using base_type::max_load_factor;
        using base_type::rehash;
        using base_type::reserve;

        using base_type::insert;
        using base_type::insert_or_assign;
        using base_type::try_emplace;
        using base_type::extract;
        using base_type::merge;
        using base_type::swap;

        /// \brief Constructs an element in place from \p args.
        /// \see basic_ctsv_map::emplace for the rationale behind the
        /// constraint the standard containers' emplace lacks.
        template<typename... TArgs>
            requires std::constructible_from<value_type, TArgs...>
        std::pair<iterator, bool> emplace(TArgs&&... args)
        {
            return base_type::emplace(std::forward<TArgs>(args)...);
        }

        /// \brief Constructs an element in place near \p hint.
        template<typename... TArgs>
            requires std::constructible_from<value_type, TArgs...>
        iterator emplace_hint(const_iterator hint, TArgs&&... args)
        {
            return base_type::emplace_hint(hint, std::forward<TArgs>(args)...);
        }

        /// \brief Inserts a default-constructed value if \p key is absent and
        /// returns a reference to the mapped value.
        /// \remarks Takes key_type, NOT std_sv_type: this member inserts.
        [[nodiscard]] mapped_type& operator[](const key_type key)
        {
            return base_type::operator[](key);
        }

        /// \brief operator[] accepting the opposite key flavor, available only
        /// when converting to key_type cannot forge a guarantee.
        [[nodiscard]] mapped_type& operator[](const other_key_type key)
            requires std::constructible_from<key_type, other_key_type>
        {
            return base_type::operator[](key_type{key});
        }

        // -- lookup ------------------------------------------------------------
        // noexcept because transparent_ctsv_hash and transparent_ctsv_equal_to
        // both require noexcept invocation for every key flavor, and an
        // unordered lookup neither allocates nor rehashes.

        /// \brief Finds \p key. \returns an iterator, or end() if absent.
        [[nodiscard]] iterator find(const std_sv_type key) noexcept
        {
            return base_type::find(key);
        }

        /// \copydoc find
        [[nodiscard]] const_iterator find(const std_sv_type key) const noexcept
        {
            return base_type::find(key);
        }

        /// \brief \returns true iff \p key is present.
        [[nodiscard]] bool contains(const std_sv_type key) const noexcept
        {
            return base_type::contains(key);
        }

        /// \brief \returns 1 if \p key is present, otherwise 0.
        [[nodiscard]] size_type count(const std_sv_type key) const noexcept
        {
            return base_type::count(key);
        }

        /// \brief \returns the range of elements matching \p key.
        [[nodiscard]] std::pair<iterator, iterator> equal_range(const std_sv_type key) noexcept
        {
            return base_type::equal_range(key);
        }

        /// \copydoc equal_range
        [[nodiscard]] std::pair<const_iterator, const_iterator>
            equal_range(const std_sv_type key) const noexcept
        {
            return base_type::equal_range(key);
        }

        /// \brief \returns a reference to the value mapped to \p key.
        /// \throws std::out_of_range if \p key is absent.
        [[nodiscard]] mapped_type& at(const std_sv_type key)
        {
            if (const auto it = base_type::find(key); it != base_type::end())
            {
                return it->second;
            }
            detail::throw_ctsv_key_not_found<TChar>("basic_ctsv_unordered_map::at", key);
        }

        /// \copydoc at
        [[nodiscard]] const mapped_type& at(const std_sv_type key) const
        {
            if (const auto it = base_type::find(key); it != base_type::cend())
            {
                return it->second;
            }
            detail::throw_ctsv_key_not_found<TChar>("basic_ctsv_unordered_map::at", key);
        }

        /// \brief The non-throwing at: \returns a pointer to the value mapped
        /// to \p key, or nullptr if \p key is absent.
        [[nodiscard]] mapped_type* at_if(const std_sv_type key) noexcept
        {
            const auto it = base_type::find(key);
            return it != base_type::end() ? std::addressof(it->second) : nullptr;
        }

        /// \copydoc at_if
        [[nodiscard]] const mapped_type* at_if(const std_sv_type key) const noexcept
        {
            const auto it = base_type::find(key);
            return it != base_type::cend() ? std::addressof(it->second) : nullptr;
        }

        /// \brief Erases the element at \p pos. \returns the following iterator.
        iterator erase(const_iterator pos) { return base_type::erase(pos); }

        /// \brief Erases [first, last). \returns the following iterator.
        iterator erase(const_iterator first, const_iterator last)
        {
            return base_type::erase(first, last);
        }

        /// \brief Erases the element matching \p key, if any.
        /// \returns the number erased (0 or 1).
        size_type erase(const std_sv_type key) noexcept
        {
            const auto it = base_type::find(key);
            if (it == base_type::end()) { return 0; }
            base_type::erase(it);
            return 1;
        }

        /// \brief Escape hatch to the wrapped std::unordered_map.
        [[nodiscard]] base_type& base() noexcept { return *this; }
        /// \copydoc base
        [[nodiscard]] const base_type& base() const noexcept { return *this; }

        [[nodiscard]] bool operator==(const basic_ctsv_unordered_map& other) const
            requires std::equality_comparable<mapped_type>
        {
            return base() == other.base();
        }
    };

    // =====================================================================
    // Unordered set
    // =====================================================================

    /// \brief A std::unordered_set of basic_ct_string_view<TChar, VALID_CSTR>
    /// whose lookup members accept anything convertible to
    /// std::basic_string_view.
    /// \see basic_ctsv_unordered_map for the hash/equality pairing rationale.
    template<std_char TChar, bool VALID_CSTR,
             transparent_ctsv_hash<TChar>     THash = ctsv_hash<TChar>,
             transparent_ctsv_equal_to<TChar> TEq   = ctsv_equal_to<TChar>>
        requires consistent_ctsv_hash_equal<THash, TEq, TChar>
    class basic_ctsv_unordered_set
        : private std::unordered_set<basic_ct_string_view<TChar, VALID_CSTR>, THash, TEq>
    {
        using base_type = std::unordered_set<basic_ct_string_view<TChar, VALID_CSTR>, THash, TEq>;

    public:
        static constexpr bool is_cstring = VALID_CSTR;
        static constexpr bool folds_case = ctsv_folds_case_v<THash>;

        using char_type       = TChar;
        using key_type        = basic_ct_string_view<TChar, VALID_CSTR>;
        using other_key_type  = basic_ct_string_view<TChar, !VALID_CSTR>;
        using std_sv_type     = std::basic_string_view<TChar>;
        using value_type      = typename base_type::value_type;
        using size_type       = typename base_type::size_type;
        using difference_type = typename base_type::difference_type;
        using hasher          = THash;
        using key_equal       = TEq;
        using allocator_type  = typename base_type::allocator_type;
        using reference       = typename base_type::reference;
        using const_reference = typename base_type::const_reference;
        using pointer         = typename base_type::pointer;
        using const_pointer   = typename base_type::const_pointer;
        using iterator        = typename base_type::iterator;
        using const_iterator  = typename base_type::const_iterator;
        using local_iterator  = typename base_type::local_iterator;
        using const_local_iterator = typename base_type::const_local_iterator;
        using node_type       = typename base_type::node_type;
        using insert_return_type = typename base_type::insert_return_type;

        using base_type::base_type;

        using base_type::empty;
        using base_type::size;
        using base_type::max_size;
        using base_type::clear;
        using base_type::begin;
        using base_type::end;
        using base_type::cbegin;
        using base_type::cend;
        using base_type::get_allocator;
        using base_type::hash_function;
        using base_type::key_eq;

        using base_type::bucket_count;
        using base_type::max_bucket_count;
        using base_type::bucket_size;
        using base_type::bucket;
        using base_type::load_factor;
        using base_type::max_load_factor;
        using base_type::rehash;
        using base_type::reserve;

        using base_type::insert;
        using base_type::extract;
        using base_type::merge;
        using base_type::swap;

        /// \brief Constructs an element in place from \p args.
        /// \see basic_ctsv_map::emplace for the rationale behind the
        /// constraint the standard containers' emplace lacks.
        template<typename... TArgs>
            requires std::constructible_from<value_type, TArgs...>
        std::pair<iterator, bool> emplace(TArgs&&... args)
        {
            return base_type::emplace(std::forward<TArgs>(args)...);
        }

        /// \brief Constructs an element in place near \p hint.
        template<typename... TArgs>
            requires std::constructible_from<value_type, TArgs...>
        iterator emplace_hint(const_iterator hint, TArgs&&... args)
        {
            return base_type::emplace_hint(hint, std::forward<TArgs>(args)...);
        }

        /// \brief Finds \p key. \returns an iterator, or end() if absent.
        [[nodiscard]] iterator find(const std_sv_type key) noexcept
        {
            return base_type::find(key);
        }

        /// \copydoc find
        [[nodiscard]] const_iterator find(const std_sv_type key) const noexcept
        {
            return base_type::find(key);
        }

        /// \brief \returns true iff \p key is present.
        [[nodiscard]] bool contains(const std_sv_type key) const noexcept
        {
            return base_type::contains(key);
        }

        /// \brief \returns 1 if \p key is present, otherwise 0.
        [[nodiscard]] size_type count(const std_sv_type key) const noexcept
        {
            return base_type::count(key);
        }

        /// \brief \returns the range of elements matching \p key.
        [[nodiscard]] std::pair<iterator, iterator> equal_range(const std_sv_type key) noexcept
        {
            return base_type::equal_range(key);
        }

        /// \copydoc equal_range
        [[nodiscard]] std::pair<const_iterator, const_iterator>
            equal_range(const std_sv_type key) const noexcept
        {
            return base_type::equal_range(key);
        }

        /// \brief Erases the element at \p pos. \returns the following iterator.
        iterator erase(const_iterator pos) { return base_type::erase(pos); }

        /// \brief Erases [first, last). \returns the following iterator.
        iterator erase(const_iterator first, const_iterator last)
        {
            return base_type::erase(first, last);
        }

        /// \brief Erases the element matching \p key, if any.
        /// \returns the number erased (0 or 1).
        size_type erase(const std_sv_type key) noexcept
        {
            const auto it = base_type::find(key);
            if (it == base_type::end()) { return 0; }
            base_type::erase(it);
            return 1;
        }

        /// \brief Escape hatch to the wrapped std::unordered_set.
        [[nodiscard]] base_type& base() noexcept { return *this; }
        /// \copydoc base
        [[nodiscard]] const base_type& base() const noexcept { return *this; }

        [[nodiscard]] bool operator==(const basic_ctsv_unordered_set& other) const
        {
            return base() == other.base();
        }
    };

    // =====================================================================
    // Convenience aliases
    // =====================================================================
    //
    // Spelled out for char and wchar_t in both key flavors, case-sensitive
    // ("..._map") and case-insensitive ("..._ci_map"). For char8_t, char16_t
    // and char32_t, instantiate basic_ctsv_map / _set / _unordered_map /
    // _unordered_set directly -- every one of them is supported, the alias
    // surface is simply kept to the two character types the standard library
    // can actually format and stream.

    // -- ordered set -------------------------------------------------------
    /// \brief Ordered, case-sensitive set of ct_cstring_view.
    using ct_cstring_view_set     = basic_ctsv_set<char, true>;
    /// \brief Ordered, case-sensitive set of ct_string_view.
    using ct_string_view_set      = basic_ctsv_set<char, false>;
    /// \brief Ordered, case-sensitive set of ct_wcstring_view.
    using ct_wcstring_view_set    = basic_ctsv_set<wchar_t, true>;
    /// \brief Ordered, case-sensitive set of ct_wstring_view.
    using ct_wstring_view_set     = basic_ctsv_set<wchar_t, false>;

    /// \brief Ordered, ASCII-case-insensitive set of ct_cstring_view.
    using ct_cstring_view_ci_set  = basic_ctsv_set<char, true, ctsv_ci_less<char>>;
    /// \brief Ordered, ASCII-case-insensitive set of ct_string_view.
    using ct_string_view_ci_set   = basic_ctsv_set<char, false, ctsv_ci_less<char>>;
    /// \brief Ordered, ASCII-case-insensitive set of ct_wcstring_view.
    using ct_wcstring_view_ci_set = basic_ctsv_set<wchar_t, true, ctsv_ci_less<wchar_t>>;
    /// \brief Ordered, ASCII-case-insensitive set of ct_wstring_view.
    using ct_wstring_view_ci_set  = basic_ctsv_set<wchar_t, false, ctsv_ci_less<wchar_t>>;

    // -- ordered map -------------------------------------------------------
    /// \brief Ordered, case-sensitive map keyed on ct_cstring_view.
    template<typename TValue> using ct_cstring_view_map  = basic_ctsv_map<char, true, TValue>;
    /// \brief Ordered, case-sensitive map keyed on ct_string_view.
    template<typename TValue> using ct_string_view_map   = basic_ctsv_map<char, false, TValue>;
    /// \brief Ordered, case-sensitive map keyed on ct_wcstring_view.
    template<typename TValue> using ct_wcstring_view_map = basic_ctsv_map<wchar_t, true, TValue>;
    /// \brief Ordered, case-sensitive map keyed on ct_wstring_view.
    template<typename TValue> using ct_wstring_view_map  = basic_ctsv_map<wchar_t, false, TValue>;

    /// \brief Ordered, ASCII-case-insensitive map keyed on ct_cstring_view.
    template<typename TValue> using ct_cstring_view_ci_map =
        basic_ctsv_map<char, true, TValue, ctsv_ci_less<char>>;
    /// \brief Ordered, ASCII-case-insensitive map keyed on ct_string_view.
    template<typename TValue> using ct_string_view_ci_map =
        basic_ctsv_map<char, false, TValue, ctsv_ci_less<char>>;
    /// \brief Ordered, ASCII-case-insensitive map keyed on ct_wcstring_view.
    template<typename TValue> using ct_wcstring_view_ci_map =
        basic_ctsv_map<wchar_t, true, TValue, ctsv_ci_less<wchar_t>>;
    /// \brief Ordered, ASCII-case-insensitive map keyed on ct_wstring_view.
    template<typename TValue> using ct_wstring_view_ci_map =
        basic_ctsv_map<wchar_t, false, TValue, ctsv_ci_less<wchar_t>>;

    // -- unordered set -----------------------------------------------------
    /// \brief Unordered, case-sensitive set of ct_cstring_view.
    using ct_cstring_view_unordered_set     = basic_ctsv_unordered_set<char, true>;
    /// \brief Unordered, case-sensitive set of ct_string_view.
    using ct_string_view_unordered_set      = basic_ctsv_unordered_set<char, false>;
    /// \brief Unordered, case-sensitive set of ct_wcstring_view.
    using ct_wcstring_view_unordered_set    = basic_ctsv_unordered_set<wchar_t, true>;
    /// \brief Unordered, case-sensitive set of ct_wstring_view.
    using ct_wstring_view_unordered_set     = basic_ctsv_unordered_set<wchar_t, false>;

    /// \brief Unordered, ASCII-case-insensitive set of ct_cstring_view.
    using ct_cstring_view_ci_unordered_set  =
        basic_ctsv_unordered_set<char, true, ctsv_ci_hash<char>, ctsv_ci_equal_to<char>>;
    /// \brief Unordered, ASCII-case-insensitive set of ct_string_view.
    using ct_string_view_ci_unordered_set   =
        basic_ctsv_unordered_set<char, false, ctsv_ci_hash<char>, ctsv_ci_equal_to<char>>;
    /// \brief Unordered, ASCII-case-insensitive set of ct_wcstring_view.
    using ct_wcstring_view_ci_unordered_set =
        basic_ctsv_unordered_set<wchar_t, true, ctsv_ci_hash<wchar_t>, ctsv_ci_equal_to<wchar_t>>;
    /// \brief Unordered, ASCII-case-insensitive set of ct_wstring_view.
    using ct_wstring_view_ci_unordered_set  =
        basic_ctsv_unordered_set<wchar_t, false, ctsv_ci_hash<wchar_t>, ctsv_ci_equal_to<wchar_t>>;

    // -- unordered map -----------------------------------------------------
    /// \brief Unordered, case-sensitive map keyed on ct_cstring_view.
    template<typename TValue> using ct_cstring_view_unordered_map =
        basic_ctsv_unordered_map<char, true, TValue>;
    /// \brief Unordered, case-sensitive map keyed on ct_string_view.
    template<typename TValue> using ct_string_view_unordered_map =
        basic_ctsv_unordered_map<char, false, TValue>;
    /// \brief Unordered, case-sensitive map keyed on ct_wcstring_view.
    template<typename TValue> using ct_wcstring_view_unordered_map =
        basic_ctsv_unordered_map<wchar_t, true, TValue>;
    /// \brief Unordered, case-sensitive map keyed on ct_wstring_view.
    template<typename TValue> using ct_wstring_view_unordered_map =
        basic_ctsv_unordered_map<wchar_t, false, TValue>;

    /// \brief Unordered, ASCII-case-insensitive map keyed on ct_cstring_view.
    template<typename TValue> using ct_cstring_view_ci_unordered_map =
        basic_ctsv_unordered_map<char, true, TValue, ctsv_ci_hash<char>, ctsv_ci_equal_to<char>>;
    /// \brief Unordered, ASCII-case-insensitive map keyed on ct_string_view.
    template<typename TValue> using ct_string_view_ci_unordered_map =
        basic_ctsv_unordered_map<char, false, TValue, ctsv_ci_hash<char>, ctsv_ci_equal_to<char>>;
    /// \brief Unordered, ASCII-case-insensitive map keyed on ct_wcstring_view.
    template<typename TValue> using ct_wcstring_view_ci_unordered_map =
        basic_ctsv_unordered_map<wchar_t, true, TValue,
            ctsv_ci_hash<wchar_t>, ctsv_ci_equal_to<wchar_t>>;
    /// \brief Unordered, ASCII-case-insensitive map keyed on ct_wstring_view.
    template<typename TValue> using ct_wstring_view_ci_unordered_map =
        basic_ctsv_unordered_map<wchar_t, false, TValue,
            ctsv_ci_hash<wchar_t>, ctsv_ci_equal_to<wchar_t>>;

    // =====================================================================
    // Structural self-checks
    // =====================================================================
    //
    // The insert/lookup asymmetry is the central guarantee of this header, so
    // it is asserted rather than merely documented.

    static_assert(!std::constructible_from<ct_cstring_view_map<int>::key_type, std::string_view>,
        "a ct_cstring_view key must not be constructible from a string_view -- "
        "that is what makes every insertion path safe");
    static_assert(!std::constructible_from<ct_string_view_map<int>::key_type, std::string_view>);
    static_assert(std::convertible_to<ct_cstring_view_map<int>::key_type,
        ct_cstring_view_map<int>::std_sv_type>,
        "...while every lookup path must accept a key by implicit conversion");

    // The non-C-string flavor IS constructible from the C-string flavor
    // (dropping a guarantee is always safe), so its operator[] gains the
    // other-flavor overload; the C-string flavor's does not.
    static_assert(std::constructible_from<ct_string_view, ct_cstring_view>);
    static_assert(!std::constructible_from<ct_cstring_view, ct_string_view>);

    static_assert(std::ranges::input_range<ct_cstring_view_map<int>>);
    static_assert(std::ranges::bidirectional_range<ct_cstring_view_set>);
    static_assert(std::ranges::forward_range<ct_cstring_view_unordered_set>);
}

#endif // CPS_CT_STR_CTSV_CONTAINERS_HPP


