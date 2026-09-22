#include "fmt_reg_demo.hpp"
#include <fmt/format.h>
#include <fmt/xchar.h>
#include <chrono>
#include <sstream>
#include <string>
#include <format>
#include <ranges>
#include <concepts>
#include <utility>
#include <locale>
static constexpr auto newl = "\n";
namespace cps::ct_string::test_app
{
    namespace
    {
        template<typename T>
        using wo_qual_t = std::remove_cvref_t<T>;
        template<typename T1, typename T2>
        concept same_wo_qual = std::same_as<wo_qual_t<T1>, wo_qual_t<T2>>;
        template<typename T>
        concept range_of_char = RG::input_range<T> && RG::viewable_range<T> && same_wo_qual<RG::range_value_t<T>, char>;
        template<typename T>
        concept sized_ra_range_of_char = range_of_char<T> && RG::sized_range<T> && RG::random_access_range<T>;
        template<typename T>
        concept range_of_wchar_t = RG::input_range<T> && RG::viewable_range<T> && same_wo_qual<RG::range_value_t<T>, wchar_t>;
        template<typename T>
        concept sized_ra_range_of_wchar_t = range_of_wchar_t<T> && RG::sized_range<T> && RG::random_access_range<T>;

        // NOTE (lifetimes): a view NEVER owns the container it adapts unless it is an
        // owning_view. Taking the source range *by value* and piping it therefore
        // manufactures a ref_view onto the (about-to-die) parameter, and the returned
        // adaptor dangles the instant the function returns. Take the source by
        // forwarding reference and launder it through VW::all(std::forward<R>(r)):
        //   - lvalue argument  -> ref_view onto the CALLER's object (which outlives us),
        //   - rvalue argument  -> owning_view that MOVES the container into the pipeline.
        // Either way the result is safe to return.
        template<sized_ra_range_of_wchar_t R>
        constexpr sized_ra_range_of_char auto wchar_t_to_char(R&& wchars) noexcept
        {
            return VW::all(std::forward<R>(wchars)) |
                VW::transform([](const wchar_t wc) constexpr noexcept -> char {return static_cast<char>(wc);});
        }

        template<sized_ra_range_of_wchar_t R>
        std::ostream& write_to_stream(std::ostream& os, R&& wchars)
        {
            // `wchars` is a named parameter, hence an lvalue: the pipeline below binds a
            // ref_view to the caller's object, which is alive for the whole call.
            auto chars = wchar_t_to_char(wchars);
            for (const auto c : chars )
            {
                os << c;
            }
            return os;
        }

        template<sized_ra_range_of_char R>
        std::ostream& write_to_stream(std::ostream& os, R&& chars)
        {

            for (const auto c : chars )
            {
                os << c;
            }
            return os;
        }

        struct cross_eq_chars
        {
            template<sized_ra_range_of_wchar_t L, sized_ra_range_of_char R>
            constexpr bool operator()(L&& lhs, R&& rhs) const noexcept
            {
                return RG::equal(wchar_t_to_char(lhs), rhs);
            }

            template<sized_ra_range_of_char L, sized_ra_range_of_wchar_t R>
            constexpr bool operator()(L&& lhs, R&& rhs) const noexcept
            {
                return RG::equal(lhs, wchar_t_to_char(rhs));
            }
        };

        // -------------------------------------------------------------------
        // "Where did my thousands separators go?"
        //
        // Nowhere -- they were never asked for. Digit grouping is OPT-IN via the
        // `L` (locale) flag in the format spec. A bare "{}" is required by the
        // standard to produce the locale-INDEPENDENT rendering, no matter what
        // the global locale is and no matter what locale the target stream was
        // imbued with.
        //
        // Because our formatters inherit parse() from formatter<nwf_rep_t>, the
        // ENTIRE integer spec grammar (L, width, fill/align, #x, +, ...) already
        // works and applies to the seconds count. This routine proves it.
        // -------------------------------------------------------------------
        void demo_locale_aware_specs(std::ostream& os, const narrow_and_wide_formattable& nwf)
        {
            // setup_streams() already did std::locale::global(std::locale{"en_US.UTF-8"}),
            // so the default-constructed locale IS en_US.UTF-8.
            const std::locale global_loc{};
            const std::locale classic_loc = std::locale::classic();

            os << "locale-aware ('L') renderings ..." << newl;

            // No 'L' -> never grouped. This is what the demo was doing all along.
            os << R"(  fmt "{}"        : )" << fmt::format("{}", nwf) << newl;
            os << R"(  std "{}"        : )" << std::format("{}", nwf) << newl;

            // 'L' with no locale argument -> uses the GLOBAL locale.
            os << R"(  fmt "{:L}"      : )" << fmt::format("{:L}", nwf) << newl;
            os << R"(  std "{:L}"      : )" << std::format("{:L}", nwf) << newl;

            // 'L' with an explicit locale -> that locale wins, global is ignored.
            os << R"(  fmt "{:L}" (en) : )" << fmt::format(global_loc, "{:L}", nwf) << newl;
            os << R"(  std "{:L}" (en) : )" << std::format(global_loc, "{:L}", nwf) << newl;

            // The "C" locale has an EMPTY grouping spec, so 'L' is a no-op there.
            os << R"(  fmt "{:L}" (C)  : )" << fmt::format(classic_loc, "{:L}", nwf) << newl;
            os << R"(  std "{:L}" (C)  : )" << std::format(classic_loc, "{:L}", nwf) << newl;

            // The rest of the inherited spec grammar works too. Note that width /
            // fill / align apply to the COUNT ONLY -- the literal prefix and suffix
            // our format() writes are outside the base formatter's control.
            os << R"(  std "{:*>15L}"  : )" << std::format("{:*>15L}", nwf) << newl;
            os << R"(  std "{:+}"      : )" << std::format("{:+}", nwf) << newl;
            os << R"(  std "{:#x}"     : )" << std::format("{:#x}", nwf) << newl;

            // Wide flavors, narrowed for display. These pass PRVALUE std::wstrings
            // straight into write_to_stream -- safe now that the pipeline launders
            // its source through VW::all(std::forward<R>(r)).
            os << R"(  fmt L"{:L}"     : )";
            write_to_stream(os, fmt::format(L"{:L}", nwf)) << newl;
            os << R"(  std L"{:L}"     : )";
            write_to_stream(os, std::format(L"{:L}", nwf)) << newl;

            // The synthesized operator<< CANNOT be grouped:
            // ctsv_format_registration.hpp hard-codes fmt::format(L"{}", v) /
            // std::format("{}", v). A stream inserter has no channel through
            // which to pass a format spec. If you need a spec, call format
            // yourself (or imbue + use a spec-aware path).
            os << "  (operator<< has no spec channel -> never grouped)" << newl;
        }

    }
    using namespace std::chrono_literals;
    using namespace std::literals;

    // NOTE: there is no UDL for the *duration* std::chrono::years -- the `y`
    // suffix is taken by the calendar type std::chrono::year -- so we roll our own.
    consteval CHR::years operator""_yrs(unsigned long long v) noexcept
    {
        return CHR::years{static_cast<CHR::years::rep>(v)};
    }

    std::ostream& operator<<(std::ostream& os, const narrow_and_wide_formattable& insert_me)
    {
        return  os << fmt::format("{}", insert_me);
    }

    void run_demo_auto_stream_insert_test(std::ostream& os, std::ostream& err)
    {
        err << "No text like this (save this), it passed." << newl;
        os << "Begin demo auto stream insert test." << newl;

        static constexpr auto s_k_nwf = narrow_and_wide_formattable{12_yrs, "Demo Animal"_ctsv, L"Demo Animal"_ctsv};

        const std::string str_via_fmt_fmt = fmt::format("{}", s_k_nwf);
        const std::string str_via_std_fmt = std::format("{}", s_k_nwf);
        const std::wstring w_str_via_fmt_fmt = fmt::format(L"{}", s_k_nwf);
        const std::wstring w_str_via_std_fmt = std::format(L"{}", s_k_nwf);

        // hand-written narrow operator<<
        const std::string str_via_stream_insert = []()->std::string
        {
            std::ostringstream oss{};
            oss << s_k_nwf;
            return std::move(oss).str();
        }();
        // synthesized wide operator<< (cps::ct_string::operator<<, reachable only
        // thanks to the using-declaration in the header)
        const std::wstring w_str_via_stream_insert = []()-> std::wstring
        {
            std::wostringstream oss{};
            oss << s_k_nwf;
            return std::move(oss).str();
        }();

        os << "narrows ... native... " << newl;

        os  << "  via fmt::format:  " << str_via_fmt_fmt << newl
            << "  via std::format:  " << str_via_std_fmt << newl
            << "  via operator<<:   " << str_via_stream_insert << newl;

        os << "wides ... as converted... " << newl;

        os << " via fmt::format: ";
        write_to_stream(os, w_str_via_fmt_fmt) << newl;

        os << " via std::format: ";
        write_to_stream(os, w_str_via_std_fmt) << newl;

        os << " via operator<<: ";
        write_to_stream(os, w_str_via_stream_insert) << newl;




        const bool narrow_ok = str_via_fmt_fmt == str_via_std_fmt && str_via_std_fmt == str_via_stream_insert;
        const bool wide_ok = w_str_via_fmt_fmt == w_str_via_std_fmt && w_str_via_std_fmt == w_str_via_stream_insert;

        os << "  narrow renderings all agree: " << std::boolalpha << narrow_ok << newl
           << "  wide renderings all agree:   " << std::boolalpha << wide_ok << newl;

        if (!narrow_ok)
        {
            err << "ERROR: narrow renderings disagree!" << newl;
        }
        if (!wide_ok)
        {
            err << "ERROR: wide renderings disagree!" << newl;
        }

        os << newl;
        demo_locale_aware_specs(os, s_k_nwf);
        os << newl;

        static constexpr auto equator = cross_eq_chars{};
        const bool cross_via_fmt_match = equator(str_via_fmt_fmt, w_str_via_fmt_fmt);
        const bool cross_via_steam_match = equator(str_via_stream_insert, w_str_via_stream_insert);
        const bool cross_via_std_match = equator(str_via_std_fmt, w_str_via_std_fmt);
        const bool cross_ok = cross_via_fmt_match && cross_via_steam_match && cross_via_std_match;

        if (cross_ok)
        {
            os << "wide and narrow strings match! ... all good!" <<newl;
            return;
        }

        auto print_mismatch = [&]<sized_ra_range_of_char C, sized_ra_range_of_wchar_t W>
            (C&& chars, W&& wchars, std::string_view descrIfMismatched) -> void
        {
            if (!equator(chars, wchars))
            {
                os << descrIfMismatched << ": chars: \"";
                write_to_stream(os, chars) << "\";\t wchars: \"";
                write_to_stream(os, wchars) << "\"." << newl;
            }
        };


        print_mismatch(str_via_fmt_fmt, w_str_via_fmt_fmt,
            "Wide and narrow chars from fmt::format do not match."sv);
        os << newl;

        print_mismatch(str_via_stream_insert, w_str_via_stream_insert,
            "Wide and narrow chars from stream insert do not match."sv);
        os << newl;

        print_mismatch(str_via_std_fmt, w_str_via_std_fmt,
            "Wide and narrow chars from std::format do not match."sv);
        os << newl;

        os << "Demo auto stream insert test DONE." << newl;
    }
}
