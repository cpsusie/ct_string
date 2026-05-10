//
// Created by Christopher Susie on 4/25/2026.
//

#ifndef CSTR_VIEW_CT_STRING_VIEW_HPP
#define CSTR_VIEW_CT_STRING_VIEW_HPP
#include "fixed_string.hpp"
#include <string>
#include <string_view>
#include <ostream>
#include <type_traits>
#include <concepts>
#include <array>
#include <span>
#include <ranges>
#include <functional> // std::less<>, std::equal_to<>, std::hash
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <version> // for feature-test macros (__cpp_lib_format etc.)
#if defined(__cpp_lib_format) && (__cpp_lib_format >= 201907L)
#  include <format>
#endif
#if defined(__has_include)
#  if __has_include(<fmt/format.h>)
#    include <fmt/format.h>
#    define CJM_CT_STRING_VIEW_HAS_FMT 1
#  endif
#endif

namespace cjm::ct_string
{
    /// \brief Forward declaration of basic_ct_string_view, the compile-time
    /// constructed string view type.
    /// \tparam TChar the character type. Constrained to char, wchar_t, char8_t,
    /// char16_t or char32_t by the std_char concept.
    /// \tparam VALID_CSTR if true, the view is statically known to refer to a
    /// null-terminated character range usable as a C-string (data()[size()]
    /// equals TChar{} and no embedded null appears in [data(), data()+size()).
    /// If false, the view makes no such guarantee.
    template<std_char TChar, bool VALID_CSTR>
    class basic_ct_string_view;

    /// \brief Forward declaration of the factory used by the _ctsv literal and
    /// by make_ctsv to materialize a basic_ct_string_view that points into a
    /// basic_fixed_string with static storage duration.
    /// \tparam VALUE a basic_fixed_string NTTP that MUST satisfy valid_cstr().
    /// The requires clause enforces this at compile time, so an invalid VALUE
    /// fails to compile rather than producing undefined behavior.
    template<basic_fixed_string VALUE>
        requires (VALUE.valid_cstr())
    struct basic_ct_sv_factory;

    namespace detail
    {
        /// \brief Trait: true_type iff T is some specialization of basic_ct_string_view.
        /// (cv-qualifications are stripped at the use site via wo_cv_t).
        template<typename T>
        struct is_basic_ct_string_view : std::false_type {};

        template<std_char TChar, bool VALID_CSTR>
        struct is_basic_ct_string_view<basic_ct_string_view<TChar, VALID_CSTR>> : std::true_type {};

        template<std_char TChar, bool VALID_CSTR>
        struct is_basic_ct_string_view<const basic_ct_string_view<TChar, VALID_CSTR>> : std::true_type {};
    }

    /// \brief Concept structurally constraining a type to be an instantiation
    /// of basic_ct_string_view (after stripping cv-qualifications).
    template<typename T>
    concept basic_ct_string_view_type = detail::is_basic_ct_string_view<wo_cv_t<T>>::value;

    /// \brief A non-owning view over a contiguous run of TChar characters
    /// that, when VALID_CSTR is true, is statically known to be null-terminated
    /// and thus usable as a C-string.
    ///
    /// Unlike std::basic_string_view, this type carries a compile-time bit
    /// (the VALID_CSTR template parameter, exposed as the static known_cstr
    /// member) tracking whether the viewed range is guaranteed to be a valid
    /// C-string. Operations that would invalidate that guarantee (e.g.
    /// remove_suffix, the bounded substr(pos, count) overload) are either
    /// disabled on the known_cstr flavor or return a non-known_cstr view.
    ///
    /// Provenance / null-termination invariants:
    ///   * data() always points either into a basic_fixed_string<TChar, N>
    ///     NTTP object (static storage duration, compile-time-constructed) or
    ///     to the static null_term_char member.
    ///   * When known_cstr is true, data()[size()] is guaranteed to equal
    ///     TChar{} ('\0') AND no embedded null appears in
    ///     [data(), data()+size()).
    ///
    /// \tparam TChar the character type. Constrained by std_char.
    /// \tparam VALID_CSTR whether instances of this specialization carry the
    /// null-termination guarantee. Convenience aliases such as
    /// ct_cstring_view (true) and ct_string_view (false) are provided.
    template<std_char TChar, bool VALID_CSTR>
    class basic_ct_string_view
    {
        static constexpr auto null_term_char = TChar{};

        // Provenance / null-termination invariants:
        //  * m_str_v.data() always points into a basic_fixed_string<TChar, N> NTTP
        //    object (static storage duration, compile-time-constructed) -- OR -- to
        //    `null_term_char`, a static constexpr member of this class.
        //  * When VALID_CSTR (a.k.a. known_cstr) is true, m_str_v.data()[m_str_v.size()]
        //    is guaranteed to equal TChar{} ('\0') AND no embedded null appears in
        //    [data(), data()+size()).
        //
        // The only operations that can mutate m_str_v are: defaulted copy/move
        // assignment, the cross-flavor (true->false) converting assignments, swap,
        // remove_prefix, and remove_suffix. Each of those preserves both invariants
        // for the flavor on which it is enabled. See per-member comments below.

        // Allow other specializations to access m_str_v and the private ctor.
        template<std_char, bool>
        friend class basic_ct_string_view;

        template<basic_fixed_string VALUE>
            requires (VALUE.valid_cstr())
        friend struct basic_ct_sv_factory;

        // Disambiguator tag so the private ctor cannot be reached by accidental
        // {std_sv_type} brace-init even from a friend context.
        struct private_ctor_tag{};
        static constexpr auto s_k_priv_tag = private_ctor_tag{};
    public:
        using char_type = TChar;
        using value_type = TChar;
        using traits_type = std::char_traits<TChar>;
        using std_sv_type = std::basic_string_view<TChar, traits_type>;
        using std_string_type = std::basic_string<char_type, traits_type>;
        using std_ostream_type = std::basic_ostream<char_type, traits_type>;
        using pointer = std_sv_type::pointer;
        using const_pointer = std_sv_type::const_pointer;
        using reference = std_sv_type::reference;
        using const_reference = std_sv_type::const_reference;
        using iterator = std_sv_type::iterator;
        using const_iterator = std_sv_type::const_iterator;
        using reverse_iterator = std_sv_type::reverse_iterator;
        using const_reverse_iterator = std_sv_type::const_reverse_iterator;
        using size_type = std_sv_type::size_type;
        using difference_type = std_sv_type::difference_type;
        static constexpr bool known_cstr = VALID_CSTR;
        static constexpr size_type npos = std_sv_type::npos;


        // -- iterators --
        /// \brief Iterator to the first character of the view.
        [[nodiscard]] constexpr const_iterator begin() const noexcept { return m_str_v.begin(); }
        /// \brief Iterator to one past the last character of the view.
        [[nodiscard]] constexpr const_iterator end() const noexcept { return m_str_v.end(); }
        /// \brief Const iterator to the first character of the view.
        [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return m_str_v.cbegin(); }
        /// \brief Const iterator to one past the last character of the view.
        [[nodiscard]] constexpr const_iterator cend() const noexcept { return m_str_v.cend(); }
        /// \brief Reverse iterator to the last character of the view.
        [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return m_str_v.rbegin(); }
        /// \brief Reverse iterator to the theoretical character preceding the first.
        [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return m_str_v.rend(); }
        /// \brief Const reverse iterator to the last character of the view.
        [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return m_str_v.crbegin(); }
        /// \brief Const reverse iterator to the theoretical character preceding the first.
        [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return m_str_v.crend(); }

        // -- element access --
        /// \brief Indexer returning a const reference to the character at \p pos.
        /// \param[in] pos zero-based index of the desired character.
        /// \remarks Behavior is undefined if pos > size(). When known_cstr is
        /// true, pos == size() yields the null terminator (well-defined);
        /// when known_cstr is false, even pos == size() is undefined behavior
        /// because no null terminator is promised at that location.
        [[nodiscard]] constexpr const_reference operator[](size_type pos) const noexcept { return m_str_v[pos]; }
        /// \brief Bounds-checked element access.
        /// \param[in] pos zero-based index.
        /// \throws std::out_of_range if pos >= size().
        [[nodiscard]] constexpr const_reference at(size_type pos) const { return m_str_v.at(pos); }
        /// \brief Reference to the first character.
        /// \remarks Behavior is undefined if empty().
        [[nodiscard]] constexpr const_reference front() const noexcept { return m_str_v.front(); }
        /// \brief Reference to the last character.
        /// \remarks Behavior is undefined if empty().
        [[nodiscard]] constexpr const_reference back() const noexcept { return m_str_v.back(); }
        /// \brief Pointer to the underlying character buffer.
        /// \remarks Not guaranteed null-terminated unless known_cstr is true;
        /// in that case prefer c_str() to make intent explicit.
        [[nodiscard]] constexpr const_pointer data() const noexcept { return m_str_v.data(); }
        /// \brief Pointer to a null-terminated C-string equivalent to this
        /// view. Participates in overload resolution only when known_cstr is
        /// true.
        [[nodiscard]] constexpr const_pointer c_str() const noexcept requires known_cstr { return m_str_v.data(); }

        // -- capacity --
        /// \brief Number of characters in the view (excluding any null terminator).
        [[nodiscard]] constexpr size_type size() const noexcept { return m_str_v.size(); }
        /// \brief Synonym for size().
        [[nodiscard]] constexpr size_type length() const noexcept { return m_str_v.length(); }
        /// \brief The largest possible size().
        [[nodiscard]] constexpr size_type max_size() const noexcept { return m_str_v.max_size(); }
        /// \brief True iff size() == 0.
        [[nodiscard]] constexpr bool empty() const noexcept { return m_str_v.empty(); }

        // -- copy --
        /// \brief Copies up to \p count characters starting at \p pos into \p dest.
        /// \param[out] dest destination buffer; must have room for at least
        /// min(count, size()-pos) characters. Behavior is undefined if dest is
        /// null or its capacity is insufficient.
        /// \param[in] count maximum number of characters to copy.
        /// \param[in] pos starting offset into this view.
        /// \throws std::out_of_range if pos > size().
        /// \returns the number of characters actually copied.
        constexpr size_type copy(char_type* dest, size_type count, size_type pos = 0) const
        {
            return m_str_v.copy(dest, count, pos);
        }

        // -- compare --
        /// \brief Lexicographically compares this view to \p v.
        /// \returns negative, zero, or positive value as in
        /// std::basic_string_view::compare.
        [[nodiscard]] constexpr int compare(std_sv_type v) const noexcept { return m_str_v.compare(v); }
        /// \brief Compares the substring [pos1, pos1+count1) of this to \p v.
        /// \throws std::out_of_range if pos1 > size().
        [[nodiscard]] constexpr int compare(size_type pos1, size_type count1, std_sv_type v) const
        {
            return m_str_v.compare(pos1, count1, v);
        }
        /// \brief Compares the substring [pos1, pos1+count1) of this to the
        /// substring [pos2, pos2+count2) of \p v.
        /// \throws std::out_of_range if pos1 > size() or pos2 > v.size().
        [[nodiscard]] constexpr int compare(size_type pos1, size_type count1, std_sv_type v,
                                            size_type pos2, size_type count2) const
        {
            return m_str_v.compare(pos1, count1, v, pos2, count2);
        }
        /// \brief Compares this to the null-terminated C-string \p s.
        /// \remarks Behavior is undefined if s is null or not null-terminated.
        [[nodiscard]] constexpr int compare(const char_type* s) const { return m_str_v.compare(s); }
        /// \brief Compares the substring [pos1, pos1+count1) of this to the
        /// null-terminated C-string \p s.
        /// \remarks Behavior is undefined if s is null or not null-terminated.
        /// \throws std::out_of_range if pos1 > size().
        [[nodiscard]] constexpr int compare(size_type pos1, size_type count1, const char_type* s) const
        {
            return m_str_v.compare(pos1, count1, s);
        }
        /// \brief Compares the substring [pos1, pos1+count1) of this to the
        /// first \p count2 characters of \p s.
        /// \remarks Behavior is undefined if s is null or points to fewer than
        /// count2 readable characters.
        /// \throws std::out_of_range if pos1 > size().
        [[nodiscard]] constexpr int compare(size_type pos1, size_type count1, const char_type* s,
                                            size_type count2) const
        {
            return m_str_v.compare(pos1, count1, s, count2);
        }

        // -- starts_with / ends_with / contains --
        /// \brief True iff the view begins with \p sv.
        [[nodiscard]] constexpr bool starts_with(std_sv_type sv) const noexcept { return m_str_v.starts_with(sv); }
        /// \brief True iff the view begins with character \p c.
        [[nodiscard]] constexpr bool starts_with(char_type c) const noexcept { return m_str_v.starts_with(c); }
        /// \brief True iff the view begins with the null-terminated C-string \p s.
        /// \remarks Behavior is undefined if s is null or not null-terminated.
        [[nodiscard]] constexpr bool starts_with(const char_type* s) const { return m_str_v.starts_with(s); }

        /// \brief True iff the view ends with \p sv.
        [[nodiscard]] constexpr bool ends_with(std_sv_type sv) const noexcept { return m_str_v.ends_with(sv); }
        /// \brief True iff the view ends with character \p c.
        [[nodiscard]] constexpr bool ends_with(char_type c) const noexcept { return m_str_v.ends_with(c); }
        /// \brief True iff the view ends with the null-terminated C-string \p s.
        /// \remarks Behavior is undefined if s is null or not null-terminated.
        [[nodiscard]] constexpr bool ends_with(const char_type* s) const { return m_str_v.ends_with(s); }
#if defined(__cpp_lib_string_contains) && (__cpp_lib_string_contains >= 202011L)
        /// \brief True iff the view contains \p sv as a substring.
        [[nodiscard]] constexpr bool contains(std_sv_type sv) const noexcept { return m_str_v.contains(sv); }
        /// \brief True iff the view contains the character \p c.
        [[nodiscard]] constexpr bool contains(char_type c) const noexcept { return m_str_v.contains(c); }
        /// \brief True iff the view contains the null-terminated C-string \p s as a substring.
        /// \remarks Behavior is undefined if s is null or not null-terminated.
        [[nodiscard]] constexpr bool contains(const char_type* s) const { return m_str_v.contains(s); }
        /// \brief True iff the view contains the first \p len characters of \p s as a substring.
        /// \remarks Behavior is undefined if s is null or points to fewer than
        /// len readable characters.
        [[nodiscard]] constexpr bool contains(const char_type* s, std::size_t len) const noexcept
        {
            return m_str_v.contains(std_sv_type{s, len});
        }

#else
        /// \brief True iff the view contains \p sv as a substring.
        /// (Fallback when the standard library does not provide a contains member.)
        [[nodiscard]] constexpr bool contains(std_sv_type sv) const noexcept
        {
            const auto pos = m_str_v.find(sv);
            return pos != std_sv_type::npos;
        }
        /// \brief True iff the view contains the character \p c. (Fallback.)
        [[nodiscard]] constexpr bool contains(char_type c) const noexcept
        {
            const auto pos = m_str_v.find(c);
            return pos != std_sv_type::npos;
        }
        /// \brief True iff the view contains the null-terminated C-string \p s
        /// as a substring. (Fallback.)
        /// \remarks Behavior is undefined if s is null or not null-terminated.
        [[nodiscard]] constexpr bool contains(const char_type* s) const
        {
            const auto pos = m_str_v.find(s);
            return pos != std_sv_type::npos;
        }
        /// \brief True iff the view contains the first \p len characters of
        /// \p s as a substring. (Fallback.)
        /// \remarks Behavior is undefined if s is null or points to fewer than
        /// len readable characters.
        [[nodiscard]] constexpr bool contains(const char_type* s, std::size_t len) const noexcept
        {
            auto other = std_sv_type{s, len};
            return contains(other);
        }
#endif

        // -- find --
        /// \brief First index >= \p pos at which \p v occurs, or npos.
        [[nodiscard]] constexpr size_type find(std_sv_type v, size_type pos = 0) const noexcept { return m_str_v.find(v, pos); }
        /// \brief First index >= \p pos at which \p c occurs, or npos.
        [[nodiscard]] constexpr size_type find(char_type c, size_type pos = 0) const noexcept { return m_str_v.find(c, pos); }
        /// \brief First index >= \p pos at which the first \p count characters of \p s occur, or npos.
        /// \remarks Behavior is undefined if s is null or points to fewer than
        /// count readable characters.
        [[nodiscard]] constexpr size_type find(const char_type* s, size_type pos, size_type count) const { return m_str_v.find(s, pos, count); }
        /// \brief First index >= \p pos at which the null-terminated C-string \p s occurs, or npos.
        /// \remarks Behavior is undefined if s is null or not null-terminated.
        [[nodiscard]] constexpr size_type find(const char_type* s, size_type pos = 0) const { return m_str_v.find(s, pos); }

        // -- rfind --
        /// \brief Last index <= \p pos at which \p v occurs, or npos.
        [[nodiscard]] constexpr size_type rfind(std_sv_type v, size_type pos = npos) const noexcept { return m_str_v.rfind(v, pos); }
        /// \brief Last index <= \p pos at which \p c occurs, or npos.
        [[nodiscard]] constexpr size_type rfind(char_type c, size_type pos = npos) const noexcept { return m_str_v.rfind(c, pos); }
        /// \brief Last index <= \p pos at which the first \p count characters of \p s occur, or npos.
        /// \remarks Behavior is undefined if s is null or points to fewer than
        /// count readable characters.
        [[nodiscard]] constexpr size_type rfind(const char_type* s, size_type pos, size_type count) const { return m_str_v.rfind(s, pos, count); }
        /// \brief Last index <= \p pos at which the null-terminated C-string \p s occurs, or npos.
        /// \remarks Behavior is undefined if s is null or not null-terminated.
        [[nodiscard]] constexpr size_type rfind(const char_type* s, size_type pos = npos) const { return m_str_v.rfind(s, pos); }

        // -- find_first_of --
        /// \brief First index >= \p pos of any character in \p v, or npos.
        [[nodiscard]] constexpr size_type find_first_of(std_sv_type v, size_type pos = 0) const noexcept { return m_str_v.find_first_of(v, pos); }
        /// \brief Equivalent to find(c, pos).
        [[nodiscard]] constexpr size_type find_first_of(char_type c, size_type pos = 0) const noexcept { return m_str_v.find_first_of(c, pos); }
        /// \brief First index >= \p pos of any character among the first \p count characters of \p s, or npos.
        /// \remarks Behavior is undefined if s is null or points to fewer than
        /// count readable characters.
        [[nodiscard]] constexpr size_type find_first_of(const char_type* s, size_type pos, size_type count) const { return m_str_v.find_first_of(s, pos, count); }
        /// \brief First index >= \p pos of any character in the null-terminated C-string \p s, or npos.
        /// \remarks Behavior is undefined if s is null or not null-terminated.
        [[nodiscard]] constexpr size_type find_first_of(const char_type* s, size_type pos = 0) const { return m_str_v.find_first_of(s, pos); }

        // -- find_last_of --
        /// \brief Last index <= \p pos of any character in \p v, or npos.
        [[nodiscard]] constexpr size_type find_last_of(std_sv_type v, size_type pos = npos) const noexcept { return m_str_v.find_last_of(v, pos); }
        /// \brief Equivalent to rfind(c, pos).
        [[nodiscard]] constexpr size_type find_last_of(char_type c, size_type pos = npos) const noexcept { return m_str_v.find_last_of(c, pos); }
        /// \brief Last index <= \p pos of any character among the first \p count characters of \p s, or npos.
        /// \remarks Behavior is undefined if s is null or points to fewer than
        /// count readable characters.
        [[nodiscard]] constexpr size_type find_last_of(const char_type* s, size_type pos, size_type count) const { return m_str_v.find_last_of(s, pos, count); }
        /// \brief Last index <= \p pos of any character in the null-terminated C-string \p s, or npos.
        /// \remarks Behavior is undefined if s is null or not null-terminated.
        [[nodiscard]] constexpr size_type find_last_of(const char_type* s, size_type pos = npos) const { return m_str_v.find_last_of(s, pos); }

        // -- find_first_not_of --
        /// \brief First index >= \p pos of a character NOT in \p v, or npos.
        [[nodiscard]] constexpr size_type find_first_not_of(std_sv_type v, size_type pos = 0) const noexcept { return m_str_v.find_first_not_of(v, pos); }
        /// \brief First index >= \p pos at which the character is not \p c, or npos.
        [[nodiscard]] constexpr size_type find_first_not_of(char_type c, size_type pos = 0) const noexcept { return m_str_v.find_first_not_of(c, pos); }
        /// \brief First index >= \p pos of a character not among the first \p count characters of \p s, or npos.
        /// \remarks Behavior is undefined if s is null or points to fewer than
        /// count readable characters.
        [[nodiscard]] constexpr size_type find_first_not_of(const char_type* s, size_type pos, size_type count) const { return m_str_v.find_first_not_of(s, pos, count); }
        /// \brief First index >= \p pos of a character not in the null-terminated C-string \p s, or npos.
        /// \remarks Behavior is undefined if s is null or not null-terminated.
        [[nodiscard]] constexpr size_type find_first_not_of(const char_type* s, size_type pos = 0) const { return m_str_v.find_first_not_of(s, pos); }

        // -- find_last_not_of --
        /// \brief Last index <= \p pos of a character NOT in \p v, or npos.
        [[nodiscard]] constexpr size_type find_last_not_of(std_sv_type v, size_type pos = npos) const noexcept { return m_str_v.find_last_not_of(v, pos); }
        /// \brief Last index <= \p pos at which the character is not \p c, or npos.
        [[nodiscard]] constexpr size_type find_last_not_of(char_type c, size_type pos = npos) const noexcept { return m_str_v.find_last_not_of(c, pos); }
        /// \brief Last index <= \p pos of a character not among the first \p count characters of \p s, or npos.
        /// \remarks Behavior is undefined if s is null or points to fewer than
        /// count readable characters.
        [[nodiscard]] constexpr size_type find_last_not_of(const char_type* s, size_type pos, size_type count) const { return m_str_v.find_last_not_of(s, pos, count); }
        /// \brief Last index <= \p pos of a character not in the null-terminated C-string \p s, or npos.
        /// \remarks Behavior is undefined if s is null or not null-terminated.
        [[nodiscard]] constexpr size_type find_last_not_of(const char_type* s, size_type pos = npos) const { return m_str_v.find_last_not_of(s, pos); }

        /// \brief Default constructor (known_cstr flavor): produces an empty
        /// view that nonetheless points at a valid null-terminated buffer
        /// (the static null_term_char member). size() == 0 and
        /// c_str()[0] == '\0'.
        constexpr basic_ct_string_view() noexcept requires known_cstr : m_str_v{&null_term_char, 0U} {}
        /// \brief Default constructor (non-known_cstr flavor): produces an
        /// empty view with a null/unspecified data() pointer (same state as a
        /// default-constructed std::basic_string_view).
        constexpr basic_ct_string_view() noexcept requires (!known_cstr) : m_str_v{} {}
        /// \brief basic_ct_string_view is nothrow copy constructible.
        constexpr basic_ct_string_view(const basic_ct_string_view&) noexcept = default;
        /// \brief basic_ct_string_view is nothrow move constructible.
        constexpr basic_ct_string_view(basic_ct_string_view&&) noexcept = default;
        /// \brief basic_ct_string_view is nothrow copy assignable.
        constexpr basic_ct_string_view& operator=(const basic_ct_string_view&) noexcept = default;
        /// \brief basic_ct_string_view is nothrow move assignable.
        constexpr basic_ct_string_view& operator=(basic_ct_string_view&&) noexcept = default;
        /// \brief Trivial destructor.
        ~basic_ct_string_view() noexcept = default;

        /// \brief Implicit converting constructor: a known_cstr view is always
        /// also a valid non-known_cstr view, so converting from the
        /// known_cstr flavor (OVALID_CSTR == true) into the non-known_cstr
        /// flavor is permitted. The reverse direction is intentionally not
        /// provided because a non-known_cstr view does not carry the
        /// null-termination guarantee.
        template<bool OVALID_CSTR>
            requires (OVALID_CSTR && !known_cstr)
        constexpr basic_ct_string_view(const basic_ct_string_view<char_type, OVALID_CSTR>& other) noexcept : m_str_v{other.m_str_v} {}
        /// \brief Move-converting constructor; see the copy version for
        /// directionality requirements.
        template<bool OVALID_CSTR>
             requires (OVALID_CSTR && !known_cstr)
        constexpr basic_ct_string_view(basic_ct_string_view<char_type, OVALID_CSTR>&& other) noexcept
            : m_str_v{std::move(other.m_str_v)} {}

        /// \brief Cross-flavor copy assignment from a known_cstr view into a
        /// non-known_cstr view. Same direction restriction as the converting
        /// constructor.
        template<bool OVALID_CSTR>
            requires (OVALID_CSTR && !known_cstr)
        constexpr basic_ct_string_view& operator=(const basic_ct_string_view<char_type, OVALID_CSTR>& other) noexcept
        {
            m_str_v = other.m_str_v;
            return *this;
        }

        /// \brief Cross-flavor move assignment. See the copy version.
        template<bool OVALID_CSTR>
            requires (OVALID_CSTR && !known_cstr)
        constexpr basic_ct_string_view& operator=(basic_ct_string_view<char_type, OVALID_CSTR>&& other) noexcept
        {
            m_str_v = std::move(other.m_str_v);
            return *this;
        }

        /// \brief Implicit nothrow conversion to the corresponding
        /// std::basic_string_view.
        [[nodiscard]] constexpr operator std_sv_type() const noexcept { return m_str_v; }
        /// \brief Explicit conversion to the corresponding std::basic_string
        /// (allocates).
        explicit operator std_string_type() const noexcept { return std_string_type{to_string()}; }
        /// \brief Allocating copy of the viewed range as a std::basic_string.
        [[nodiscard]] std_string_type to_string() const { return std_string_type{m_str_v}; }

        // -- mutators (mutate the view, not the data) --
        /// \brief Shrink the view from the front by \p n characters.
        /// \remarks Safe for both flavors: shrinking from the front keeps the
        /// same end pointer, so a known_cstr view stays null-terminated.
        /// Behavior is undefined if n > size().
        constexpr void remove_prefix(size_type n) { m_str_v.remove_prefix(n); }

        /// \brief Shrink the view from the back by \p n characters. Only
        /// available on the non-known_cstr flavor because shrinking from the
        /// back moves the end of the view away from the original null
        /// terminator.
        /// \remarks Behavior is undefined if n > size().
        constexpr void remove_suffix(size_type n) requires (!known_cstr) { m_str_v.remove_suffix(n); }

        /// \brief Swap the two views' state. Does not move characters.
        constexpr void swap(basic_ct_string_view& other) noexcept { m_str_v.swap(other.m_str_v); }

        // -- substr --
        /// \brief Trailing substring starting at \p pos. Preserves the
        /// null-termination guarantee (returns the same flavor).
        /// \throws std::out_of_range if pos > size().
        [[nodiscard]] constexpr basic_ct_string_view substr(size_type pos = 0) const
        {
            return basic_ct_string_view{s_k_priv_tag, m_str_v.substr(pos)};
        }

        /// \brief Bounded substring of length min(count, size()-pos) starting
        /// at \p pos. Always returns a non-known_cstr view because the new
        /// end may fall short of the original null terminator.
        /// \throws std::out_of_range if pos > size().
        [[nodiscard]] constexpr basic_ct_string_view<char_type, false>
            substr(size_type pos, size_type count) const
        {
            using ret_t = basic_ct_string_view<char_type, false>;
            return ret_t{ret_t::s_k_priv_tag, m_str_v.substr(pos, count)};
        }

        // -- comparison: same type --
        /// \brief Equality with another view of the same exact type. Compared
        /// as string_views; null-terminator status is irrelevant to the
        /// result.
        [[nodiscard]] constexpr bool operator==(const basic_ct_string_view& other) const noexcept
        {
            return m_str_v == other.m_str_v;
        }
        /// \brief Three-way comparison with another view of the same exact
        /// type. Yields the same ordering as the corresponding string_views.
        [[nodiscard]] constexpr auto operator<=>(const basic_ct_string_view& other) const noexcept
        {
            return m_str_v <=> other.m_str_v;
        }

        // -- comparison: cross-flavor basic_ct_string_view --
        /// \brief Equality across the two flavors of basic_ct_string_view of
        /// the same character type.
        template<bool OVALID_CSTR>
            requires (OVALID_CSTR != VALID_CSTR)
        [[nodiscard]] constexpr bool operator==(const basic_ct_string_view<TChar, OVALID_CSTR>& other) const noexcept
        {
            return m_str_v == other.m_str_v;
        }
        /// \brief Three-way comparison across the two flavors.
        template<bool OVALID_CSTR>
            requires (OVALID_CSTR != VALID_CSTR)
        [[nodiscard]] constexpr auto operator<=>(const basic_ct_string_view<TChar, OVALID_CSTR>& other) const noexcept
        {
            return m_str_v <=> other.m_str_v;
        }

        // -- comparison: anything nothrow-convertible to std_sv_type (and not one of us) --
        /// \brief Equality with anything nothrow-convertible to std_sv_type
        /// (e.g. a basic_fixed_string of the same character type, a
        /// std::basic_string, a string literal). The other operand is first
        /// converted to a std_sv_type, then compared.
        template<nothrow_convertible_to<std_sv_type> TOther>
            requires (!basic_ct_string_view_type<TOther>)
        [[nodiscard]] constexpr bool operator==(const TOther& other) const noexcept
        {
            const std_sv_type other_sv = other;
            return m_str_v == other_sv;
        }

        /// \brief Three-way comparison with anything nothrow-convertible to
        /// std_sv_type. See the equality version for conversion semantics.
        template<nothrow_convertible_to<std_sv_type> TOther>
            requires (!basic_ct_string_view_type<TOther>)
        [[nodiscard]] constexpr auto operator<=>(const TOther& other) const noexcept
        {
            const std_sv_type other_sv = other;
            return m_str_v <=> other_sv;
        }

    private:
        // Private tag-dispatched ctor used by substr to construct a view from
        // an already-validated std_sv_type without going through the
        // consteval-only public path. The caller is responsible for upholding
        // the provenance and (where applicable) null-termination invariants.
        constexpr basic_ct_string_view(private_ctor_tag, std_sv_type str_v) noexcept : m_str_v{str_v} {}
        // Consteval ctor used only by the basic_ct_sv_factory friend to build
        // a view directly from a string_view that points into a NTTP
        // basic_fixed_string. Restricted to consteval so it cannot be reached
        // with a runtime pointer that might violate the invariants.
        consteval explicit basic_ct_string_view(std_sv_type str) noexcept : m_str_v{str} {}

        std_sv_type m_str_v;
    };

    /// \brief Factory that, given a basic_fixed_string NTTP whose value is a
    /// valid C-string, produces a known_cstr basic_ct_string_view pointing
    /// into the NTTP's static-storage buffer. Used by the _ctsv literal and
    /// by make_ctsv.
    /// \tparam VALUE the basic_fixed_string NTTP. Constrained to satisfy
    /// valid_cstr() so passing an invalid value fails to compile (no UB).
    template<basic_fixed_string VALUE>
        requires (VALUE.valid_cstr())
    struct basic_ct_sv_factory
    {
        /// \brief The fixed-string value being viewed. As a static constexpr
        /// member it has static storage duration, providing the lifetime
        /// guarantee required by the resulting view.
        static constexpr auto fstr_val = basic_fixed_string{VALUE};
        /// \brief The character type of the underlying fixed-string.
        using char_type = typename decltype(fstr_val)::char_type;
        /// \brief The known_cstr basic_ct_string_view type produced by the factory.
        using ct_sv_type = basic_ct_string_view<char_type, true>;

        /// \brief Produces a known_cstr basic_ct_string_view referring to
        /// fstr_val's character buffer. consteval to guarantee that fstr_val
        /// is materialized with static storage duration.
        consteval auto operator()() const noexcept -> ct_sv_type
        {
            return ct_sv_type{typename ct_sv_type::std_sv_type{fstr_val.data(), fstr_val.size()}};
        }
    };

    namespace literals
    {
        /// \brief Literal operator producing a known_cstr basic_ct_string_view
        /// whose underlying storage is the basic_fixed_string NTTP \p VALUE.
        /// \tparam VALUE the string-literal NTTP. Must satisfy valid_cstr();
        /// otherwise the program will not compile (rather than producing UB).
        /// \returns a basic_ct_string_view<char_type_of_VALUE, true>.
        template<basic_fixed_string VALUE>
        consteval auto operator""_ctsv() noexcept -> basic_ct_sv_factory<VALUE>::ct_sv_type
        {
            constexpr auto factory = basic_ct_sv_factory<VALUE>{};
            return factory();
        }
    }

    /// \brief Free function spelling of the _ctsv literal: produces a
    /// known_cstr basic_ct_string_view referring to the NTTP basic_fixed_string
    /// \p VALUE. Useful when literal-operator syntax is unavailable
    /// (e.g. in dependent contexts).
    /// \tparam VALUE the basic_fixed_string NTTP; must satisfy valid_cstr().
    template<basic_fixed_string VALUE>
    consteval auto make_ctsv() noexcept -> basic_ct_sv_factory<VALUE>::ct_sv_type
    {
        constexpr auto factory = basic_ct_sv_factory<VALUE>{};
        return factory();
    }

    /// \brief Convenience alias: known_cstr (C-string-usable) view of char.
    using ct_cstring_view = basic_ct_string_view<char, true>;
    /// \brief Convenience alias: known_cstr view of wchar_t.
    using ct_wcstring_view = basic_ct_string_view<wchar_t, true>;
    /// \brief Convenience alias: known_cstr view of char8_t.
    using ct_u8cstring_view = basic_ct_string_view<char8_t, true>;
    /// \brief Convenience alias: known_cstr view of char16_t.
    using ct_u16cstring_view = basic_ct_string_view<char16_t, true>;
    /// \brief Convenience alias: known_cstr view of char32_t.
    using ct_u32cstring_view = basic_ct_string_view<char32_t, true>;

    /// \brief Convenience alias: non-known_cstr view of char.
    using ct_string_view = basic_ct_string_view<char, false>;
    /// \brief Convenience alias: non-known_cstr view of wchar_t.
    using ct_wstring_view = basic_ct_string_view<wchar_t, false>;
    /// \brief Convenience alias: non-known_cstr view of char8_t.
    using ct_u8string_view = basic_ct_string_view<char8_t, false>;
    /// \brief Convenience alias: non-known_cstr view of char16_t.
    using ct_u16string_view = basic_ct_string_view<char16_t, false>;
    /// \brief Convenience alias: non-known_cstr view of char32_t.
    using ct_u32string_view = basic_ct_string_view<char32_t, false>;

    // ------------------------------------------------------------------
    // Heterogeneous-lookup-friendly container aliases.
    //
    // The library provides aliases that pre-wire the standard associative and
    // unordered-associative containers with the function objects required to
    // make heterogeneous lookup actually engage when the key type is a
    // basic_ct_string_view:
    //
    //   * Ordered containers (map/set/multimap/multiset): C++14 heterogeneous
    //     lookup is gated solely on Compare::is_transparent. The standard
    //     std::less<> (the void specialization) is itself transparent and
    //     dispatches via operator< / operator<=>, which we provide for any
    //     type nothrow-convertible to std_sv_type. So passing std::less<> as
    //     the comparator is sufficient. No transparent equal_to is needed
    //     (these containers don't use one).
    //
    //   * Unordered containers (unordered_map/set/multimap/multiset): C++20
    //     heterogeneous lookup is gated on BOTH Hash::is_transparent and
    //     KeyEqual::is_transparent. The std::hash<basic_ct_string_view<...>>
    //     specialization below is transparent and offers overloads for
    //     std::basic_string_view, std::basic_string, the opposite-flavor
    //     basic_ct_string_view, and const char_type*. The KeyEqual must also
    //     be transparent; std::equal_to<> (the void specialization) is and
    //     dispatches via operator==, which we provide for any type
    //     nothrow-convertible to std_sv_type.
    //
    // Usage example:
    //     ct_cstring_view_set s;                      // a std::set
    //     s.insert(make_ctsv<"hello">());
    //     bool found = s.contains(std::string_view{"hello"}); // heterogeneous
    //
    //     ct_cstring_view_unordered_set us;           // a std::unordered_set
    //     us.insert(make_ctsv<"world">());
    //     auto it = us.find(std::string_view{"world"});       // heterogeneous
    //
    // Caveats:
    //   * Lookup keys must be nothrow-convertible to the view's std_sv_type
    //     (that is the precondition of the comparison-operator templates).
    //     std::basic_string, std::basic_string_view, basic_fixed_string of
    //     the same character type, and the opposite-flavor basic_ct_string_view
    //     all qualify; a bare const char_type* does NOT, because the relevant
    //     basic_string_view ctor is not noexcept. To look up by raw pointer,
    //     either build a string_view from it at the call site or relax the
    //     comparison templates' constraint.
    //   * For unordered containers the same caveat applies to the hasher: a
    //     raw const char_type* hashes via the dedicated overload here, but
    //     the equality side still has to be reachable via the comparison
    //     operators, so wrap raw pointers as string_views at the call site.
    // ------------------------------------------------------------------

    /// \brief std::map<Key, TValue> with Key = basic_ct_string_view<TChar, VALID_CSTR>
    /// pre-parameterized with std::less<> so heterogeneous lookup is enabled.
    template<std_char TChar, bool VALID_CSTR, typename TValue>
    using basic_ct_string_view_map =
        std::map<basic_ct_string_view<TChar, VALID_CSTR>, TValue, std::less<>>;

    /// \brief std::multimap counterpart of basic_ct_string_view_map.
    template<std_char TChar, bool VALID_CSTR, typename TValue>
    using basic_ct_string_view_multimap =
        std::multimap<basic_ct_string_view<TChar, VALID_CSTR>, TValue, std::less<>>;

    /// \brief std::set of basic_ct_string_view pre-parameterized with
    /// std::less<> so heterogeneous lookup is enabled.
    template<std_char TChar, bool VALID_CSTR>
    using basic_ct_string_view_set =
        std::set<basic_ct_string_view<TChar, VALID_CSTR>, std::less<>>;

    /// \brief std::multiset counterpart of basic_ct_string_view_set.
    template<std_char TChar, bool VALID_CSTR>
    using basic_ct_string_view_multiset =
        std::multiset<basic_ct_string_view<TChar, VALID_CSTR>, std::less<>>;

    /// \brief std::unordered_map<Key, TValue> pre-parameterized with the
    /// transparent std::hash<basic_ct_string_view<...>> specialization
    /// provided by this header and with std::equal_to<> as the (transparent)
    /// key-equal predicate; together these enable C++20 heterogeneous lookup.
    template<std_char TChar, bool VALID_CSTR, typename TValue>
    using basic_ct_string_view_unordered_map =
        std::unordered_map<basic_ct_string_view<TChar, VALID_CSTR>, TValue,
            std::hash<basic_ct_string_view<TChar, VALID_CSTR>>, std::equal_to<>>;

    /// \brief std::unordered_multimap counterpart of
    /// basic_ct_string_view_unordered_map.
    template<std_char TChar, bool VALID_CSTR, typename TValue>
    using basic_ct_string_view_unordered_multimap =
        std::unordered_multimap<basic_ct_string_view<TChar, VALID_CSTR>, TValue,
            std::hash<basic_ct_string_view<TChar, VALID_CSTR>>, std::equal_to<>>;

    /// \brief std::unordered_set pre-parameterized with the transparent hash
    /// and std::equal_to<> for heterogeneous lookup.
    template<std_char TChar, bool VALID_CSTR>
    using basic_ct_string_view_unordered_set =
        std::unordered_set<basic_ct_string_view<TChar, VALID_CSTR>,
            std::hash<basic_ct_string_view<TChar, VALID_CSTR>>, std::equal_to<>>;

    /// \brief std::unordered_multiset counterpart of
    /// basic_ct_string_view_unordered_set.
    template<std_char TChar, bool VALID_CSTR>
    using basic_ct_string_view_unordered_multiset =
        std::unordered_multiset<basic_ct_string_view<TChar, VALID_CSTR>,
            std::hash<basic_ct_string_view<TChar, VALID_CSTR>>, std::equal_to<>>;

    // -- Per-character-type / per-flavor convenience aliases ---------------
    // (One pair per flavor for char and wchar_t -- the formatter-supported
    // character types. UTF specializations are easy to write by hand if
    // desired but elided here to keep the alias surface manageable.)

    /// \brief std::set<ct_cstring_view, std::less<>>.
    using ct_cstring_view_set        = basic_ct_string_view_set<char, true>;
    /// \brief std::set<ct_string_view, std::less<>>.
    using ct_string_view_set         = basic_ct_string_view_set<char, false>;
    /// \brief std::set<ct_wcstring_view, std::less<>>.
    using ct_wcstring_view_set       = basic_ct_string_view_set<wchar_t, true>;
    /// \brief std::set<ct_wstring_view, std::less<>>.
    using ct_wstring_view_set        = basic_ct_string_view_set<wchar_t, false>;

    /// \brief std::map<ct_cstring_view, TValue, std::less<>>.
    template<typename TValue> using ct_cstring_view_map  = basic_ct_string_view_map<char,    true,  TValue>;
    /// \brief std::map<ct_string_view, TValue, std::less<>>.
    template<typename TValue> using ct_string_view_map   = basic_ct_string_view_map<char,    false, TValue>;
    /// \brief std::map<ct_wcstring_view, TValue, std::less<>>.
    template<typename TValue> using ct_wcstring_view_map = basic_ct_string_view_map<wchar_t, true,  TValue>;
    /// \brief std::map<ct_wstring_view, TValue, std::less<>>.
    template<typename TValue> using ct_wstring_view_map  = basic_ct_string_view_map<wchar_t, false, TValue>;

    /// \brief std::unordered_set with transparent hash and std::equal_to<>.
    using ct_cstring_view_unordered_set  = basic_ct_string_view_unordered_set<char,    true>;
    /// \brief std::unordered_set with transparent hash and std::equal_to<>.
    using ct_string_view_unordered_set   = basic_ct_string_view_unordered_set<char,    false>;
    /// \brief std::unordered_set with transparent hash and std::equal_to<>.
    using ct_wcstring_view_unordered_set = basic_ct_string_view_unordered_set<wchar_t, true>;
    /// \brief std::unordered_set with transparent hash and std::equal_to<>.
    using ct_wstring_view_unordered_set  = basic_ct_string_view_unordered_set<wchar_t, false>;

    /// \brief std::unordered_map with transparent hash and std::equal_to<>.
    template<typename TValue> using ct_cstring_view_unordered_map  = basic_ct_string_view_unordered_map<char,    true,  TValue>;
    /// \brief std::unordered_map with transparent hash and std::equal_to<>.
    template<typename TValue> using ct_string_view_unordered_map   = basic_ct_string_view_unordered_map<char,    false, TValue>;
    /// \brief std::unordered_map with transparent hash and std::equal_to<>.
    template<typename TValue> using ct_wcstring_view_unordered_map = basic_ct_string_view_unordered_map<wchar_t, true,  TValue>;
    /// \brief std::unordered_map with transparent hash and std::equal_to<>.
    template<typename TValue> using ct_wstring_view_unordered_map  = basic_ct_string_view_unordered_map<wchar_t, false, TValue>;
}

/// \brief Opt in to ranges::borrowed_range, mirroring std::basic_string_view:
/// a basic_ct_string_view's iterators remain valid even after the view itself
/// is destroyed because they point at storage owned elsewhere (a
/// basic_fixed_string NTTP or the static null_term_char).
template<cjm::ct_string::std_char TChar, bool VALID_CSTR>
inline constexpr bool std::ranges::enable_borrowed_range<
    cjm::ct_string::basic_ct_string_view<TChar, VALID_CSTR>> = true;

/// \brief Opt in to ranges::view, mirroring std::basic_string_view: copying a
/// basic_ct_string_view is O(1) and does not copy the underlying characters.
template<cjm::ct_string::std_char TChar, bool VALID_CSTR>
inline constexpr bool std::ranges::enable_view<
    cjm::ct_string::basic_ct_string_view<TChar, VALID_CSTR>> = true;

/// \brief std::hash specialization for basic_ct_string_view. Delegates to
/// std::hash<basic_string_view> so that a basic_ct_string_view, the
/// equivalent std::basic_string_view, the equivalent std::basic_string, the
/// opposite-flavor basic_ct_string_view, and a null-terminated C-string of
/// the same character type all hash to the same value. This enables
/// heterogeneous lookup in std::unordered_* containers (the is_transparent
/// member type signals that to the standard library).
/// \remarks The const char_type* overload's behavior is undefined if its
/// argument is null or not null-terminated.
template<cjm::ct_string::std_char TChar, bool VALID_CSTR>
struct std::hash<cjm::ct_string::basic_ct_string_view<TChar, VALID_CSTR>>
{
    using argument_type = cjm::ct_string::basic_ct_string_view<TChar, VALID_CSTR>;
    using other_argument_type = cjm::ct_string::basic_ct_string_view<TChar, !VALID_CSTR>;
    using char_type = argument_type::char_type;
    using sv_type = argument_type::std_sv_type;
    using std_string_type = argument_type::std_string_type;
    using is_transparent = void;

    /// \brief Hashes a basic_ct_string_view by converting to std_sv_type.
    [[nodiscard]] std::size_t operator()(const argument_type& v) const noexcept
    {
        return std::hash<sv_type>{}(static_cast<sv_type>(v));
    }

    /// \brief Hashes a std::basic_string_view of the same char/traits.
    [[nodiscard]] std::size_t operator()(const sv_type& v) const noexcept
    {
        return std::hash<sv_type>{}(v);
    }

    /// \brief Hashes a std::basic_string of the same char/traits.
    [[nodiscard]] std::size_t operator()(const std_string_type& v) const noexcept
    {
        return std::hash<sv_type>{}(v);
    }

    /// \brief Hashes the opposite-flavor basic_ct_string_view.
    [[nodiscard]] std::size_t operator()(const other_argument_type& v) const noexcept
    {
        return std::hash<sv_type>{}(v);
    }

    /// \brief Hashes a null-terminated C-string of the same character type.
    /// \remarks Behavior is undefined if v is null or not null-terminated.
    [[nodiscard]] std::size_t operator()(const char_type* const v) const noexcept
    {
        return std::hash<sv_type>{}(v);
    }
};


// ---------------------------------------------------------------------------
// std::formatter specializations
// ---------------------------------------------------------------------------
//
// Mirror the standard's std::formatter<std::basic_string_view<TChar>, TChar>
// specialization by inheriting from it: a basic_ct_string_view is implicitly
// convertible to its corresponding std::basic_string_view, and inheriting the
// parse() / format() implementations gives us the full string-view format
// spec ("{:>10}", "{:.5}", fill/align/width/precision, etc.) for free.
//
// The standard library only ships std::formatter specializations for char and
// wchar_t -- there are no formatter specializations in the standard for
// char8_t / char16_t / char32_t. We therefore restrict the specialization to
// char and wchar_t to avoid forming an ill-formed base class.
// ---------------------------------------------------------------------------
#if defined(__cpp_lib_format) && (__cpp_lib_format >= 201907L)

/// \brief std::formatter specialization for basic_ct_string_view<TChar, ...>
/// when TChar is char or wchar_t. Inherits the parse/format implementation
/// from std::formatter<std::basic_string_view<TChar>, TChar>; values of
/// basic_ct_string_view convert implicitly to that string_view via the
/// inherited (noexcept) conversion operator, so formatting is a zero-copy
/// pass-through. Supports the full standard string-view format spec.
template<cjm::ct_string::std_char TChar, bool VALID_CSTR>
    requires (std::same_as<TChar, char> || std::same_as<TChar, wchar_t>)
struct std::formatter<cjm::ct_string::basic_ct_string_view<TChar, VALID_CSTR>, TChar>
    : std::formatter<std::basic_string_view<TChar>, TChar>
{
    /// \brief Formats \p v by delegating to the inherited string_view
    /// formatter after an implicit conversion to std::basic_string_view.
    template<typename FormatContext>
    auto format(const cjm::ct_string::basic_ct_string_view<TChar, VALID_CSTR>& v,
                FormatContext& ctx) const
    {
        return std::formatter<std::basic_string_view<TChar>, TChar>::format(
            static_cast<std::basic_string_view<TChar>>(v), ctx);
    }
};

#endif // __cpp_lib_format

// ---------------------------------------------------------------------------
// fmt::formatter specializations (only when {fmt} is available)
// ---------------------------------------------------------------------------
//
// {fmt} supplies a fmt::formatter<fmt::basic_string_view<TChar>, TChar>
// (and accepts std::basic_string_view via implicit conversion). We mirror
// the std::formatter approach: inherit from the string_view formatter and
// forward via the implicit operator std::basic_string_view conversion.
//
// {fmt} only meaningfully formats char and wchar_t (its non-char/wchar_t
// support is limited and version-dependent), so we again restrict to those
// two character types.
// ---------------------------------------------------------------------------
#if defined(CJM_CT_STRING_VIEW_HAS_FMT)

/// \brief fmt::formatter specialization for basic_ct_string_view<TChar, ...>
/// when TChar is char or wchar_t. Inherits the parse/format implementation
/// from fmt::formatter<std::basic_string_view<TChar>, TChar>; basic_ct_string_view
/// converts implicitly to std::basic_string_view, so this is a zero-copy
/// pass-through. Supports the full {fmt} string-view format spec.
template<cjm::ct_string::std_char TChar, bool VALID_CSTR>
    requires (std::same_as<TChar, char> || std::same_as<TChar, wchar_t>)
struct fmt::formatter<cjm::ct_string::basic_ct_string_view<TChar, VALID_CSTR>, TChar>
    : fmt::formatter<std::basic_string_view<TChar>, TChar>
{
    /// \brief Formats \p v by delegating to the inherited string_view
    /// formatter after an implicit conversion to std::basic_string_view.
    template<typename FormatContext>
    auto format(const cjm::ct_string::basic_ct_string_view<TChar, VALID_CSTR>& v,
                FormatContext& ctx) const
    {
        return fmt::formatter<std::basic_string_view<TChar>, TChar>::format(
            static_cast<std::basic_string_view<TChar>>(v), ctx);
    }
};

#endif // CJM_CT_STRING_VIEW_HAS_FMT

#endif //CSTR_VIEW_CT_STRING_VIEW_HPP

