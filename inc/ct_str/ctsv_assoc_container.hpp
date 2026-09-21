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
            using char_type = char;
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