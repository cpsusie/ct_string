#ifndef CSTR_VIEW_CTSV_ASSOC_CONTAINER_HPP
#define CSTR_VIEW_CTSV_ASSOC_CONTAINER_HPP
#include <ct_str/ct_string_view.hpp>
namespace cps::ct_string
{
#if defined(__cpp_lib_format) && (__cpp_lib_format >= 201907L)

    template<typename T>
    concept std_formattable = requires (const T& t)
    {
        { std::format("{}", t) } -> std::convertible_to<std::string>;
    };

    template<typename T>
    concept std_wide_formattable = requires (const T& t)
    {
        { std::format(L"{}", t) } -> std::convertible_to<std::wstring>;
    };
#   ifndef CPS_CTSV_STRING_VIEW_STD_NARROW_FUNC
        #define CPS_CTSV_STRING_VIEW_STD_NARROW_FUNC(v) std::format("{}", v)
#    endif
#   ifndef CPS_CTSV_STRING_VIEW_STD_WIDE_FUNC
        #define CPS_CTSV_STRING_VIEW_STD_WIDE_FUNC(v) std::format(L"{}", v)
#   endif
#else

    template<typename>
    concept std_formattable = false;

    template<typename>
    concept std_wide_formattable = false;
#   ifndef CPS_CTSV_STRING_VIEW_STD_NARROW_FUNC
        #define CPS_CTSV_STRING_VIEW_STD_NARROW_FUNC(v) ((void)(v))
#   endif
#   ifndef CPS_CTSV_STRING_VIEW_STD_WIDE_FUNC
        #define CPS_CTSV_STRING_VIEW_STD_NARROW_FUNC(v) ((void)(v))
#   endif
#endif
#if defined(CPS_CT_STRING_VIEW_HAS_FMT) && (CPS_CT_STRING_VIEW_HAS_FMT == 1)
    template<typename T>
    concept fmt_formattable = requires (const T& t)
    {
        {fmt::format("{}", t) } -> std::convertible_to<std::string>;
    };
#   ifndef CPS_CTSV_STRING_VIEW_FMT_NARROW_FUNC
        #define CPS_CTSV_STRING_VIEW_FMT_NARROW_FUNC(v) fmt::format("{}", v)
#    endif
#else
    template<typename>
    concept fmt_formattable = false;
#   ifndef CPS_CTSV_STRING_VIEW_FMT_NARROW_FUNC
        #define CPS_CTSV_STRING_VIEW_FMT_NARROW_FUNC(v) ((void)(v))
#    endif
#endif

#if defined(CPS_CT_STRING_VIEW_HAS_WFMT) && (CPS_CT_STRING_VIEW_HAS_WFMT == 1)
    template<typename T>
    concept fmt_wide_formattable = requires (const T& t)
    {
       { fmt::format(L"{}", t) } -> std::convertible_to<std::wstring>;
    };
#   ifndef CPS_CTSV_STRING_VIEW_FMT_WIDE_FUNC
        #define CPS_CTSV_STRING_VIEW_FMT_WIDE_FUNC(v) fmt::format(L"{}", v)
#   endif
#else

    template<typename>
    concept fmt_wide_formattable = false;

#   ifndef CPS_CTSV_STRING_VIEW_FMT_WIDE_FUNC
    #define CPS_CTSV_STRING_VIEW_FMT_WIDE_FUNC(v) ((void)(v))
#   endif

#endif

    template<typename T>
    concept string_source = requires (T&& t, std::string& dst)
    {
        {std::string{std::forward<T>(t)}} -> std::same_as<std::string>;
        { dst = std::forward<T>(t) } -> std::same_as<std::string&>;
    };

    template<typename T>
    concept nothrow_string_source = string_source<T> && requires (T&& t, std::string& dst)
    {
        { std::string{std::forward<T>(t)}} noexcept -> std::same_as<std::string>;
        { dst = std::forward<T>(t) } noexcept -> std::same_as<std::string&>;
    };

    template<typename T>
    concept narrow_stream_insertable = requires (std::ostream& os, const T& t)
    {
        { os << t } -> std::same_as<std::ostream&>;
    };

    template<typename T>
    concept wide_stream_insertable = requires (std::wostream& os, const T& t)
    {
        { os << t } -> std::same_as<std::wostream&>;
    };

    // Registering a std_formattable or fmt_formattable type as true for these
    // values allows for narrow or wide stream insertion to be made possible via ONE OF std::format or fmt::format.
    //
    // Do not register types that are already stream insertable.  It will likely cause ambiguous symbol resolution
    // error during compilation or may switch between the defined and via-std/fmt:: format-synthesized operator.
    //
    // Do not register the same type as true for both std::format and fmt::format.  The concepts
    // that make use of these are configured to reject anything that attempts to register for both the std and
    // format varieties.
    //
    // If your type is std_formattable AND fmt_fotmattable: no problems.  Just pick ONE of the two format functions:
    // if you want to use std::format, register it as true for g_k_stream_insert_via_std_format.
    // If you want to use fmt::format, register it as true for g_k_stream_insert_via_fmt_format.
    // Do not register it as true for both.
    //
    // Note that the narrow and wide facilities are independent of each other.
    //
    //  Usage directions: see example in fmt_reg_demo.hpp and fmt_reg_demo.cpp


    template<std_formattable T>
    inline constexpr bool g_k_stream_insert_via_std_format = false;

    template<fmt_formattable T>
    inline constexpr bool g_k_stream_insert_via_fmt_format = false;

    template<std_wide_formattable T>
    inline constexpr bool g_k_wide_stream_insert_via_std_format = false;

    template<fmt_wide_formattable T>
    inline constexpr bool g_k_wide_stream_insert_via_fmt_format = false;

    namespace detail
    {
        template<typename T>
        concept nregistered_insert_via_std_format_impl = g_k_stream_insert_via_std_format<T>;

        template<typename T>
        concept nregistered_insert_via_fmt_format_impl = g_k_stream_insert_via_fmt_format<T>;

        template<typename T>
        concept wregistered_insert_via_std_format_impl = g_k_wide_stream_insert_via_std_format<T>;

        template<typename T>
        concept wregistered_insert_via_fmt_format_impl = g_k_wide_stream_insert_via_fmt_format<T>;
    }

    template<typename T>
    concept narrow_stream_insertable_via_std_format =
        std_formattable<T>                                  &&
        detail::nregistered_insert_via_std_format_impl<T>   &&
        !detail::nregistered_insert_via_fmt_format_impl<T>;

    template<typename T>
    concept narrow_stream_insertable_via_fmt_format = fmt_formattable<T>
        && detail::nregistered_insert_via_fmt_format_impl<T> && !detail::nregistered_insert_via_std_format_impl<T>;

    template<typename T>
    concept narrow_stream_insertable_via_format =
        narrow_stream_insertable_via_std_format<T> || narrow_stream_insertable_via_fmt_format<T>;

    template<typename T>
    concept wide_stream_insertable_via_std_format =
        std_wide_formattable<T>                             &&
        detail::wregistered_insert_via_std_format_impl<T>   &&
        !detail::wregistered_insert_via_fmt_format_impl<T>;


    template<typename T>
    concept wide_stream_insertable_via_fmt_format = fmt_wide_formattable<T>
        && detail::wregistered_insert_via_fmt_format_impl<T> && !detail::wregistered_insert_via_std_format_impl<T>;

    template<typename T>
    concept wide_stream_insertable_via_format =
        wide_stream_insertable_via_std_format<T> || wide_stream_insertable_via_fmt_format<T>;

    template<typename T>
    concept insertable_via_format = narrow_stream_insertable_via_format<T> || wide_stream_insertable_via_format<T>;

    namespace detail
    {
        template<narrow_stream_insertable_via_format T>
        struct nformat_proxy_impl;

        template<narrow_stream_insertable_via_std_format T>
        struct nformat_proxy_impl<T>
        {
            using char_type = char;
            using string_type = std::basic_string<char_type>;
            using bos_type = std::basic_ostream<char_type>;
            string_type operator()(const T& t) const
            {
                return CPS_CTSV_STRING_VIEW_STD_NARROW_FUNC(t);
            }
        };

        template<narrow_stream_insertable_via_fmt_format T>
        struct nformat_proxy_impl<T>
        {
            using char_type = char;
            using string_type = std::basic_string<char_type>;
            string_type operator()(const T& t) const
            {
                return CPS_CTSV_STRING_VIEW_FMT_NARROW_FUNC(t);
            }
        };

        template<wide_stream_insertable_via_format T>
        struct wformat_proxy_impl;

        template<wide_stream_insertable_via_std_format T>
        struct wformat_proxy_impl<T>
        {
            using char_type = wchar_t;
            using string_type = std::basic_string<char_type>;
            using bos_type = std::basic_ostream<char_type>;
            string_type operator()(const T& t) const
            {
                return CPS_CTSV_STRING_VIEW_STD_WIDE_FUNC(t);
            }
        };

        template<wide_stream_insertable_via_fmt_format T>
        struct wformat_proxy_impl<T>
        {
            using char_type = wchar_t;
            using string_type = std::basic_string<char_type>;
            string_type operator()(const T& t) const
            {
                return CPS_CTSV_STRING_VIEW_FMT_WIDE_FUNC(t);
            }
        };


    }



    std::ostream& operator<<(std::ostream& os, narrow_stream_insertable_via_format auto const& t)
    {
        using n_proxy_t = detail::nformat_proxy_impl<std::remove_cvref_t<decltype( t)>>;
        static constexpr auto proxy = n_proxy_t{};
        os << proxy(t);
        return os;
    }

    std::wostream& operator<<(std::wostream& os, wide_stream_insertable_via_format auto const& t)
    {
        using w_proxy_t = detail::wformat_proxy_impl<std::remove_cvref_t<decltype(t)>>;
        static constexpr auto proxy = w_proxy_t{};
        os << proxy(t);
        return os;
    }


    namespace function_objects
    {

        template<std_char TChar>
        struct make_char_ascii_lowercase final
        {
            using char_type = std::remove_cvref_t<TChar>;

            constexpr char_type operator()(const char_type c) const noexcept
            {
                return static_cast<char_type>(exec(static_cast<char>(c)));
            }

            constexpr char_type operator()(const char_type c) const noexcept requires std::same_as<char_type, char>
            {
                return exec(c);
            }

        private:

            [[nodiscard]] constexpr char exec(const char c) const noexcept
            {
                return c >= 'A' && c <= 'Z' ? static_cast<char_type>(c - 'A' + 'a') : c;
            }
        };

        template<std_char TChar>
        struct make_char_ascii_uppercase final
        {
            using char_type = std::remove_cvref_t<TChar>;

            constexpr char_type operator()(const char_type c) const noexcept
            {
                return static_cast<char_type>(exec(static_cast<char>(c)));
            }

            constexpr char_type operator()(const char_type c) const noexcept requires std::same_as<char_type, char>
            {
                return exec(c);
            }


        private:

            [[nodiscard]] constexpr char exec(const char c) const noexcept
            {
                return c >= 'a' && c <= 'z' ? static_cast<char_type>(c - 'a' + 'A') : c;
            }
        };


        using ct_string::nothrow_convertible_to;

        using ct_cstring_view = cps::ct_string::ct_cstring_view;
        using ct_string_view = cps::ct_string::ct_string_view;

        template<std_char TChar>
        struct ci_three_way_comp
        {
            using char_type = std::remove_cvref_t<TChar>;
            using c_ctsv_type = basic_ct_string_view<char_type, true>;
            using ctsv_type = basic_ct_string_view<char_type, false>;
            using std_sv_type = std::basic_string_view<char_type>;

            template<typename T>
            static constexpr bool valid_fallback_arg_v = !same_wo_qual<T, c_ctsv_type> && !same_wo_qual<T, ctsv_type> && !same_wo_qual<T, std_sv_type> && std::convertible_to<T, std_sv_type>;

            using is_transparent = std::true_type;

            template<typename TFallback1, typename TFallback2>
                requires (valid_fallback_arg_v<TFallback1> && valid_fallback_arg_v<TFallback2>)
            constexpr std::strong_ordering operator()(const TFallback1& lhs,
                const TFallback2& rhs) const noexcept(nothrow_convertible_to<decltype(lhs), std_sv_type> && nothrow_convertible_to<decltype(rhs), std_sv_type>)
            {
                const std::string_view lv = lhs;
                const std::string_view rv = rhs;
                return exec(lv, rv);
            }

        private:
            static constexpr std::strong_ordering exec(std::string_view lhs, std::string_view rhs) noexcept
            {
                auto lower_left = lhs | std::views::transform(make_char_ascii_lowercase<char>{});
                auto lower_right = rhs | std::views::transform(make_char_ascii_lowercase<char>{});
                return std::lexicographical_compare_three_way(lower_left.begin(), lower_left.end(), lower_right.begin(), lower_right.end());
            }
        };

        struct ci_less
        {
            using is_transparent = std::true_type;
            constexpr bool operator()(nothrow_convertible_to<std::string_view> auto const& lhs, nothrow_convertible_to<std::string_view> auto const& rhs) const noexcept
            {
                const std::string_view lv = lhs;
                const std::string_view rv = rhs;
                // std::ranges::less compares two ELEMENTS (it requires totally_ordered_with);
                // it has no idea how to order two RANGES. Ordering ranges element-by-element
                // is std::ranges::lexicographical_compare's job. Here the case folding is
                // done with a PROJECTION rather than a transform_view -- same result, but no
                // intermediate view need be formed on either side.
                return std::ranges::lexicographical_compare(lv, rv,
                    std::ranges::less{}, make_char_ascii_lowercase<char>{}, make_char_ascii_lowercase<char>{});
            }
        };
    }
}
#ifdef CPS_CTSV_STRING_VIEW_STD_NARROW_FUNC
#undef CPS_CTSV_STRING_VIEW_STD_NARROW_FUNC
#endif
#ifdef CPS_CTSV_STRING_VIEW_STD_WIDE_FUNC
#undef CPS_CTSV_STRING_VIEW_STD_WIDE_FUNC
#endif
#ifdef CPS_CTSV_STRING_VIEW_FMT_NARROW_FUNC
#undef CPS_CTSV_STRING_VIEW_FMT_NARROW_FUNC
#endif
#ifdef CPS_CTSV_STRING_VIEW_FMT_WIDE_FUNC
#undef CPS_CTSV_STRING_VIEW_FMT_WIDE_FUNC
#endif

#endif