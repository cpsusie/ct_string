#ifndef CPS_FIXED_STRING_HPP
#define CPS_FIXED_STRING_HPP
#include <string>
#include <string_view>
#include <array>
#include <span>
#include <cstdint>
#include <cstddef>
#include <ostream>
#include <type_traits>
#include <ranges>
#include <algorithm>
#include <concepts>
#include <compare>

namespace cps::ct_string
{
    /// \brief A concept describing one of the built-in character types:
    /// char, wchar_t, char8_t, char16_t and char32_t.
    template<typename TChar>
    concept std_char =
        std::same_as<TChar, char>       ||
        std::same_as<TChar, wchar_t>    ||
        std::same_as<TChar, char8_t>    ||
        std::same_as<TChar, char16_t>   ||
        std::same_as<TChar, char32_t>;

    /// \brief forward declaration of basic_fixed_string
    template<std_char TChar, std::size_t CSTR_LEN>
        requires (CSTR_LEN > 0)
    struct basic_fixed_string;

    /// \brief shorthand for std::remove_cv_t: removes top-level const and volatile
    template<typename T>
    using wo_cv_t = std::remove_cv_t<T>;

    namespace detail
    {
        template<typename T>
        struct is_basic_fixed_string : std::false_type {};

        template<std_char TChar, std::size_t CSTR_LEN>
          requires (CSTR_LEN > 0)
        struct is_basic_fixed_string<basic_fixed_string<TChar, CSTR_LEN>> : std::true_type {};

        template<std_char TChar, std::size_t CSTR_LEN>
          requires (CSTR_LEN > 0)
        struct is_basic_fixed_string<const basic_fixed_string<TChar, CSTR_LEN>> : std::true_type {};

        template<typename TSrc, typename TDst>
        concept nothrow_convertible_to_IMPL = std::is_nothrow_convertible_v<TSrc, TDst>;
    }

    /// \brief Concept structurally constraining a type to be an instantiation of
    /// basic_fixed_string
    template<typename T>
    concept basic_fixed_string_type = detail::is_basic_fixed_string<wo_cv_t<T>>::value;

    /// \brief Requires that 1) TSre be std::convertible_to<TDst> and 2) that it be
    /// so-convertible without the possibility of throwing an exception
    /// \remarks The standard does not require that const char* be nothrow convertible to std::string_view
    /// but on clang, gcc, msvc they are.
    template<typename TSrc, typename TDst>
    concept nothrow_convertible_to = std::convertible_to<TSrc, TDst> &&
        detail::nothrow_convertible_to_IMPL<TSrc, TDst>;

    /// \brief Yields the length of two concatenated cstrings.
    template<std::size_t CSTR_LEN1, std::size_t CSTR_LEN2>
            requires (CSTR_LEN1 > 0 && CSTR_LEN2 > 0)
    inline constexpr std::size_t concat_length_v = (CSTR_LEN1 -1U) + (CSTR_LEN2 -1U) + 1U;

    /// \brief Yields the basic_fixed_string type that is the result of concatenating two
    /// basic_fixed_strings together.
    template<std_char TChar, std::size_t CSTR_LEN1, std::size_t CSTR_LEN2>
            requires (CSTR_LEN1 > 0 && CSTR_LEN2 > 0)
    using concat_fixed_string_t = basic_fixed_string<TChar, concat_length_v<CSTR_LEN1, CSTR_LEN2>>;

    /// A string that is backed by a fixed-size character buffer.
    /// It has two imagined use cases:
    ///     1. being the constexpr static storage type into which ct_string_views point into
    ///     2. serving as string type that works as a non-type template parameter
    /// \tparam TChar the character type of the basic_fixed_string.  It is constrained to be
    /// char, wchar_t, char8_t, char16_t or char32_t.
    /// \tparam CSTR_LEN the length of the fixed size buffer.  This includes the null terminator.  Thus,
    /// it must always be > 0.
    /// \remarks the static data member "m_str_arr" is public to meet the requirements of a type used as
    /// a non-type-template parameter.  The constructors and literal operators for this type are available only at
    /// compile-time and guarantee the following:
    ///    1- string will be null-terminated AND
    ///    2- string will not contain any null characters other than the terminator
    /// Improvident direct mutation of m_str_arr may break these guarantees,
    /// but the intended use of the type does not involve direct access to m_str_arr,
    /// and the type's member functions will continue to work as long as the invariants are maintained.
    /// When this type is used to construct a basic_ct_string_view, the invariants are checked again (compile-time-only)
    /// and a basic_fixed_string with broken invariants will trigger compilation-error on conversion
    /// \remarks the type's member functions (except buff_size() and buff_length()) do not treat the null-terminator
    /// as being part of the string. You may access the null-terminator via the indexer operator [], but attempted
    /// access to it via at() will throw std::out_of_range
    template<std_char TChar, std::size_t CSTR_LEN>
        requires (CSTR_LEN > 0)
    struct basic_fixed_string
    {
        /// \brief the character type
        using char_type = TChar;
        /// \brief the character type
        using value_type = char_type;
        /// the character traits type: always std::char_traits
        using traits_type = std::char_traits<char_type>;
        /// the string view type corresponding to this type's char_type and traits_type
        using std_sv_type = std::basic_string_view<char_type, traits_type>;
        /// the std::string_type corresponding to this type's char_type and traits_type
        using string_type = std::basic_string<char_type, traits_type>;
        /// the pointer type.  Note that both pointer and const_pointer are const
        using pointer = std_sv_type::pointer;
        /// the constant pointer type
        using const_pointer = std_sv_type::const_pointer;
        /// the reference type (note: is const)
        using reference = std_sv_type::reference;
        /// the const reference type
        using const_reference = std_sv_type::const_reference;

        /// the const iterator type
        using const_iterator = std_sv_type::const_iterator;
        /// the iterator type.  NOTE iterator and const_iterator are both const
        using iterator = std_sv_type::iterator;
        /// the const reverse iterator type
        using const_reverse_iterator = std_sv_type::const_reverse_iterator;
        /// the reverse iterator type (note that const_reverse_iterator and reverse_iterator
        /// are both const
        using reverse_iterator = std_sv_type::reverse_iterator;
        /// the size type
        using size_type = std_sv_type::size_type;
        /// the difference type
        using difference_type = std_sv_type::difference_type;
        /// the ostream type corresponding to this type's char and traits type
        using ostream_type = std::basic_ostream<char_type, traits_type>;

        /// \brief gets the character at the front.  This function participates in overload resolution only if the
        /// string value is non-empty (null terminator not counting)
        [[nodiscard]] constexpr value_type front() const noexcept requires (CSTR_LEN > 1U) { return m_str_arr.front(); }
        /// \brief gets the character at the back.  This function participates in overload resolution only if the
        /// string value is non-empty (null terminator not counting)
        [[nodiscard]] constexpr value_type back() const noexcept requires (CSTR_LEN > 1U) { return m_str_arr[size() - 1U]; }

        /// get if the string is empty (null terminator does not count)
        [[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }

        /// Gets constant iterator to first character
        [[nodiscard]] constexpr const_iterator begin() const noexcept { return get_std_sv().begin(); }
        /// Gets constant iterator to one past the last character (
        [[nodiscard]] constexpr const_iterator end() const noexcept { return get_std_sv().end(); }
        /// Gets constant iterator to first character
        [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return get_std_sv().cbegin(); }
        /// Gets constant iterator to one past the last character
        [[nodiscard]] constexpr const_iterator cend() const noexcept { return get_std_sv().cend(); }
        /// Gets reverse constant iterator to last character
        [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return get_std_sv().rbegin(); }
        /// Gets reverse const iterator to the theoretical character preceding first
        [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return get_std_sv().rend(); }
        /// Gets reverse constant iterator to last character
        [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return get_std_sv().crbegin(); }
        /// Gets reverse const iterator to the theoretical character preceding first
        [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return get_std_sv().crend(); }

        /// Indexer returns cref to character at specified pos
        /// \param[value] n index of desired character.
        /// \remarks if n == buff_size() - 1, will return null terminator.  If n >= buff_size(),
        /// behavior is undefined
        [[nodiscard]] constexpr const_reference operator[](size_type n) const noexcept { return m_str_arr[n]; }
        /// Indexer returns cref to character at specified pos
        /// \param[value] n index of desired character.
        /// \throws std::out_of_range if n >= size() (NOT buff_size)
        [[nodiscard]] constexpr const_reference at(size_type n) const { return m_str_arr.at(n); }
        /// Get const pointer to front of buffer.  Because buff_size() is always > 0, guaranteed
        /// to be non-null.
        [[nodiscard]] constexpr const_pointer data() const noexcept { return m_str_arr.data(); }
        /// Get the std::basic_string_view corresponding to this type's char_type and traits type
        [[nodiscard]] constexpr std_sv_type get_std_sv() const noexcept
        {
            return std_sv_type{m_str_arr.data(), CSTR_LEN -1};
        }
        /// Get the size of the string (NOT including null terminator)
        [[nodiscard]] constexpr size_type size() const noexcept { return m_str_arr.size() - 1U; }
        /// Get the size of the string (NOT including null terminator)
        [[nodiscard]] constexpr size_type length() const noexcept { return size(); }
        /// Get the size of entire buffer (including null terminator)
        [[nodiscard]] constexpr size_type buff_size() const noexcept { return m_str_arr.size(); }
        /// Get the size of entire buffer (including null terminator)
        [[nodiscard]] constexpr size_type buff_length() const noexcept { return buff_size(); }

        /// Get whether this is a valid cstr.  A valid cstring
        ///    1- is null terminated AND
        ///    2- has no other null characters in it.
        /// \remarks These strings are always valid cstrings upon construction.  To be an NTTP,
        /// however, its underlying buffer must be public.  Therefore, direct access to underlying
        /// buffer (which is not the intended use of the type) can invalidate it.
        [[nodiscard]] constexpr bool valid_cstr() const noexcept
        {
            return m_str_arr.back() == char_type{} && std::ranges::find(get_std_sv(), char_type{})
                == get_std_sv().end();
        }

        // NOLINTBEGIN(*-explicit-constructor) Implicit conversion necessary and intended
        // ReSharper disable once CppNonExplicitConvertingConstructor (implicitness intended)
        /// Construct a basic_fixed_string from a character literal.
        /// \param[in] str - character array to be copied in.
        /// \remarks If str is not null-terminated or if it contains other null-terminators,
        /// the program will not compile
        consteval basic_fixed_string(const char_type (&str)[CSTR_LEN])
        {
            std::ranges::copy_n(str, CSTR_LEN, m_str_arr.data());
            if (!valid_cstr())
            {
                throw "Input string must be null-terminated, but cannot contain null characters."; // NOLINT(*-exception-baseclass)
            }
        }

        // ReSharper disable once CppNonExplicitConvertingConstructor (implicitness intended)
        /// Construct a basic_fixed_string a fixed-size span of characters
        /// \param[in] str - character array to be copied in.
        /// \remarks If str is not null-terminated or if it contains other null-terminators,
        /// the program will not compile
        consteval basic_fixed_string(const std::span<const char_type, CSTR_LEN> str)
        {
            std::ranges::copy_n(str.data(), CSTR_LEN, m_str_arr.data());
            if (!valid_cstr())
            {
                throw "Input string must be null-terminated, but cannot contain null characters."; // NOLINT(*-exception-baseclass)
            }
        }
        // NOLINTEND(*-explicit-constructor)

        /// Default ctor only works for an empty basic_fixed_string.  Buffer will contain null terminator,
        /// string will be empty
        consteval basic_fixed_string() noexcept requires (CSTR_LEN == 1)
        {
            m_str_arr.at(0) = char_type{};
        }

        /// basic_fixed_string is nothrow copy constructible
        constexpr basic_fixed_string(const basic_fixed_string&) noexcept = default;
        /// basic_fixed_string is nothrow move constructible
        constexpr basic_fixed_string(basic_fixed_string&&) noexcept = default;
        /// basic_fixed_string is nothrow copy assignable
        constexpr basic_fixed_string& operator=(const basic_fixed_string&) noexcept = default;
        /// basic_fixed_string is nothrow move assignable
        constexpr basic_fixed_string& operator=(basic_fixed_string&&) noexcept = default;
        /// dtor
        ~basic_fixed_string() noexcept = default;

        /// Explicit conversion to appropriate std::basic_string
        constexpr explicit operator string_type() const { return string_type{m_str_arr.data(), CSTR_LEN -1}; }
        // ReSharper disable once CppNonExplicitConversionOperator

        // NOLINTBEGIN(*-explicit-constructor) implicit conversion to std::basic_string_view intentional
        /// Implicit nothrow conversion to appropriate basic_string_view
        /// \remarks if the fixed_string is a constant with static storage duration, the resultant
        /// std::string_view will never dangle.  Otherwise, the returned std::basic_string_view
        /// will be valid as long as the fixed_string remains alive and unmutated.
        constexpr operator std_sv_type() const noexcept { return get_std_sv(); }
        // NOLINTEND(*-explicit-constructor)

        /// basic fixed string is equality comparable with another basic_fixed_string of the
        /// same type.  Equality is treated as string_view equality: the null-terminator
        /// is not considered.
        constexpr bool operator==(const basic_fixed_string& other) const noexcept
        {
            return get_std_sv() == other.get_std_sv();
        }

        /// basic fixed strings form a strict total order and are relationally comparable
        /// with other basic_fixed_strings of the same type.  The comparisons are made as if
        /// a string_view: null terminator character is not considered
        constexpr std::strong_ordering operator<=>(const basic_fixed_string& other) const noexcept
        {
            return get_std_sv() <=> other.get_std_sv();
        }

        /// Basic fixed strings are equality comparable with other basic fixed strings of the same
        /// character type but different lengths.  The comparisons are made as if
        /// a string_view: null terminator character is not considered
        template<std::size_t OCSTR_LEN>
            requires (OCSTR_LEN > 0 && OCSTR_LEN != CSTR_LEN)
        constexpr bool operator==([[maybe_unused]] const basic_fixed_string<value_type, OCSTR_LEN>& other)
            const noexcept
        {
            return get_std_sv() == other.get_std_sv();
        }

        /// basic_fixed_strings of the same character type but different lengths form a strict total order.
        /// The comparisons are made as if  a string_view: null terminator character is not considered
        template<std::size_t OCSTR_LEN>
            requires (OCSTR_LEN > 0 && OCSTR_LEN != CSTR_LEN)
        constexpr std::strong_ordering operator<=>(const basic_fixed_string<value_type, OCSTR_LEN>& other)
            const noexcept
        {
            return get_std_sv() <=> other.get_std_sv();
        }

        /// basic_fixed_strings are equality comparable with anything nothrow convertible to a basic_string_view
        /// of the same character and traits type.  The basic_fixed_string's null terminator character is not
        /// considered
        template<nothrow_convertible_to<std_sv_type> TOther>
            requires (!basic_fixed_string_type<TOther>)
        constexpr bool operator==(const TOther& other) const noexcept
        {
            const std_sv_type other_sv = other;
            return get_std_sv() == other_sv;
        }

        /// Basic fixed strings form a strict total order with anything nothrow convertible to a std::basic_string_view
        /// of the same character and traits_type.  \param other is first converted to a basic_string_view before
        /// the comparison.  The basic_fixed_string's null terminator is not considered.
        template<nothrow_convertible_to<std_sv_type> TOther>
            requires (!basic_fixed_string_type<TOther>)
        constexpr std::strong_ordering operator<=>(const TOther& other) const noexcept
        {
            const std_sv_type other_sv = other;
            return get_std_sv() <=> other_sv;
        }

        /// basic_fixed_strings of the same character type can be concatenated.
        /// \param[in] other the string that should be appended to this one
        /// \returns a basic_fixed_string that is a valid_cstring containing the characters
        /// herein with the characters of other appended thereto.
        template<std::size_t OCSTR_LEN>
                requires (OCSTR_LEN > 0)
        consteval auto operator+(const basic_fixed_string<value_type, OCSTR_LEN>& other) const noexcept
            -> concat_fixed_string_t<value_type, CSTR_LEN, OCSTR_LEN>
        {
            using ret_t = concat_fixed_string_t<value_type, CSTR_LEN, OCSTR_LEN>;
            if (!valid_cstr() || !other.valid_cstr())
            {
                throw "Both strings must be valid null-terminated C-strings."; // NOLINT(*-exception-baseclass)
            }

            std::array<value_type, concat_length_v<CSTR_LEN, OCSTR_LEN>> result{};
            std::ranges::copy_n(get_std_sv().data(), CSTR_LEN -1, result.data());
            std::ranges::copy_n(other.get_std_sv().data(), OCSTR_LEN -1, result.data() + (CSTR_LEN -1));
            result.back() = char_type{};
            return ret_t{result};
        }

        /// The buffer storing the string and the null terminator.  If you edit it,
        /// you may break it's invariant that it be null-terminated but not contain any other null characters.
        /// It is only public because that is required for NTTPs
        std::array<char_type, CSTR_LEN> m_str_arr{};
    };

    /// Deduction guide for making a basic_fixed_string from a string literal of the same character type
    template<std_char TChar, std::size_t CSTR_LEN>
        requires (CSTR_LEN > 0)
    basic_fixed_string(TChar const (&)[CSTR_LEN]) -> basic_fixed_string<TChar, CSTR_LEN>;

    /// Deduction guide for making a basic_fixed_string from a fixed-size span of the same character type
    template<std_char TChar, std::size_t CSTR_LEN>
        requires (CSTR_LEN > 0)
    basic_fixed_string(std::span<const TChar, CSTR_LEN>) -> basic_fixed_string<TChar, CSTR_LEN>;

    /// Stream insert operator for basic_ostream of appropriate character type
    /// \param[in|out] os - the output stream into which you wish to insert characters
    /// \param[in] bfs the basic fixed string whose characters (excluding null-terminator) to-be-inserted-into
    /// the output stream
    /// \returns non-const reference to os
    /// \remarks Only participates in overload resolution if TChar is char or wchar_t because
    /// standard library does not support stream insertions into the utf character streams
    template<std_char TChar, std::size_t CSTR_LEN>
        requires (CSTR_LEN > 0 && (std::same_as<TChar, char> || std::same_as<TChar, wchar_t>))
    auto operator<<(std::basic_ostream<TChar, std::char_traits<TChar>>& os,
        const basic_fixed_string<TChar, CSTR_LEN>& bfs) -> std::basic_ostream<TChar, std::char_traits<TChar>>&
    {
        return os << bfs.get_std_sv();
    }

    namespace literals
    {
        /// literal operator creating a basic_fixed_string
        /// \tparam BFS the string literal argument.  Must be null-terminated and not contain
        /// any other null characters
        /// \returns a basic_fixed_string with the value of BFS that is null-terminated and does not
        /// contain any other null characters
        /// \remarks if somehow BFS is not null terminated or contains other null characters,
        /// the program will not compile.
        template<basic_fixed_string BFS>
        consteval auto operator""_fs() noexcept
        {
            if (!BFS.valid_cstr())
            {
                throw "Input string must be null-terminated, but cannot contain null characters."; // NOLINT(*-exception-baseclass)
            }
            return BFS;
        }
    }
}
#endif
