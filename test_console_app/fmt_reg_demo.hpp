//
// Created by cpsusie on 9/22/26.
//

#ifndef HAND_EVALUATOR_FMT_REG_DEMO_HPP
#define HAND_EVALUATOR_FMT_REG_DEMO_HPP
#include <chrono>
#include <compare>
#include <ostream>
#include <ranges>
#include <utility>
#include "ct_str/ct_string_view.hpp"
#include "ct_str/ctsv_format_registration.hpp"
#include <format>
#include <fmt/format.h>
#include <fmt/xchar.h>   // REQUIRED for wide ({fmt}) formatting support

namespace cps::ct_string::test_app
{
    namespace CHR = std::chrono;
    namespace RG = std::ranges;
    namespace VW = std::ranges::views;
    using namespace cps::ct_string::literals;

    // class declaration
    class narrow_and_wide_formattable;

    /// \brief The arithmetic representation type of the duration stored by
    /// narrow_and_wide_formattable. All four formatter specializations below
    /// inherit their parse() from the formatter for this type, so the entire
    /// format-spec supplied by the caller (e.g. "L", ">12", "+#x", ...) is
    /// applied to the seconds count.
    using nwf_rep_t = CHR::seconds::rep;

    // We remember to provide the narrow stream operator but not the wide!
    std::ostream& operator<<(std::ostream&, const narrow_and_wide_formattable&);

    void run_demo_auto_stream_insert_test(std::ostream& os, std::ostream& err);
}

// Best if can: predeclare all types in header that will have fmt providers defined, then go out to global namespace after
// all such forward declarations and put your desired formatters thereafter.
// Definitions for formatters goes at bottom of header.

// narrow formattable via fmt
template<>
struct fmt::formatter<cps::ct_string::test_app::narrow_and_wide_formattable, char>;

// wide formattable via fmt
template<>
struct fmt::formatter<cps::ct_string::test_app::narrow_and_wide_formattable, wchar_t>;

// narrow formattable via std
template<>
struct std::formatter<cps::ct_string::test_app::narrow_and_wide_formattable, char>;

// wide formattable via std
template<>
struct std::formatter<cps::ct_string::test_app::narrow_and_wide_formattable, wchar_t>;

namespace cps::ct_string::test_app
{
    // definition for the formattable type
    class narrow_and_wide_formattable
    {
    public:
        [[nodiscard]] constexpr CHR::seconds duration() const noexcept { return m_duration; }
        [[nodiscard]] constexpr ct_cstring_view name() const noexcept { return m_name; }
        [[nodiscard]] constexpr ct_wcstring_view wname() const noexcept { return m_wname; }

        constexpr narrow_and_wide_formattable(CHR::seconds dur,
            ct_cstring_view name = "Anonymous"_ctsv, ct_wcstring_view wname = L"Anonymous"_ctsv) noexcept
            : m_duration{dur}, m_name {name}, m_wname{wname} {}
        constexpr narrow_and_wide_formattable() noexcept : narrow_and_wide_formattable{CHR::seconds{0}} {}

        constexpr bool operator==(const narrow_and_wide_formattable&) const noexcept = default;
        constexpr std::strong_ordering operator<=>(const narrow_and_wide_formattable&) const noexcept = default;

    private:
        std::chrono::seconds m_duration{};
        ct_cstring_view m_name {};
        ct_wcstring_view m_wname {};
    };
}

// ---------------------------------------------------------------------------
// Formatter definitions.
//
// Every one of the four specializations below produces exactly:
//
//      "NAME" has lasted COUNT seconds.
//
// i.e. the equivalent of formatting with R"("{}" has lasted {:L} seconds.)"
// (mutatis mutandis for the wide flavors, which use wname() and L"...").
//
// Rather than hand-rolling a parse() (and thereby losing width / fill / align /
// the locale-aware 'L' flag / alternate integer presentations), each
// specialization *inherits* from the formatter for the duration's
// representation type. That gives us a fully conforming parse() for free: the
// caller's spec is parsed by, and applied to, the seconds count. So
//
//      fmt::format(std::locale{"en_US.UTF-8"}, "{:L}", nwf)
//
// yields:  "Anonymous" has lasted 1,234,567 seconds.
//
// The inherited parse() consumes the whole spec, so format() simply:
//   1. writes the literal prefix (including the quoted name),
//   2. advances the context's output iterator (REQUIRED before delegating to
//      the base formatter, which writes through ctx.out()),
//   3. delegates the count to the base formatter,
//   4. advances again and writes the literal suffix.
// ---------------------------------------------------------------------------

// narrow formattable via fmt
template<>
struct fmt::formatter<cps::ct_string::test_app::narrow_and_wide_formattable, char>
    : fmt::formatter<cps::ct_string::test_app::nwf_rep_t, char>
{
    using base_t = fmt::formatter<cps::ct_string::test_app::nwf_rep_t, char>;
    // parse() inherited from base_t: the caller's spec applies to the count.

    template<typename FormatContext>
    auto format(const cps::ct_string::test_app::narrow_and_wide_formattable& nwf,
                FormatContext& ctx) const -> decltype(ctx.out())
    {
        auto out = fmt::format_to(ctx.out(), R"("{}" has lasted )", nwf.name());
        ctx.advance_to(out);
        out = base_t::format(nwf.duration().count(), ctx);
        ctx.advance_to(out);
        return fmt::format_to(ctx.out(), " seconds.");
    }
};

// wide formattable via fmt
template<>
struct fmt::formatter<cps::ct_string::test_app::narrow_and_wide_formattable, wchar_t>
    : fmt::formatter<cps::ct_string::test_app::nwf_rep_t, wchar_t>
{
    using base_t = fmt::formatter<cps::ct_string::test_app::nwf_rep_t, wchar_t>;
    // parse() inherited from base_t: the caller's spec applies to the count.

    template<typename FormatContext>
    auto format(const cps::ct_string::test_app::narrow_and_wide_formattable& nwf,
                FormatContext& ctx) const -> decltype(ctx.out())
    {
        auto out = fmt::format_to(ctx.out(), LR"("{}" has lasted )", nwf.wname());
        ctx.advance_to(out);
        out = base_t::format(nwf.duration().count(), ctx);
        ctx.advance_to(out);
        return fmt::format_to(ctx.out(), L" seconds.");
    }
};

// narrow formattable via std
template<>
struct std::formatter<cps::ct_string::test_app::narrow_and_wide_formattable, char>
    : std::formatter<cps::ct_string::test_app::nwf_rep_t, char>
{
    using base_t = std::formatter<cps::ct_string::test_app::nwf_rep_t, char>;
    // parse() inherited from base_t: the caller's spec applies to the count.

    template<typename FormatContext>
    auto format(const cps::ct_string::test_app::narrow_and_wide_formattable& nwf,
                FormatContext& ctx) const -> decltype(ctx.out())
    {
        auto out = std::format_to(ctx.out(), R"("{}" has lasted )", nwf.name());
        ctx.advance_to(out);
        out = base_t::format(nwf.duration().count(), ctx);
        ctx.advance_to(out);
        return std::format_to(ctx.out(), " seconds.");
    }
};

// wide formattable via std
template<>
struct std::formatter<cps::ct_string::test_app::narrow_and_wide_formattable, wchar_t>
    : std::formatter<cps::ct_string::test_app::nwf_rep_t, wchar_t>
{
    using base_t = std::formatter<cps::ct_string::test_app::nwf_rep_t, wchar_t>;
    // parse() inherited from base_t: the caller's spec applies to the count.

    template<typename FormatContext>
    auto format(const cps::ct_string::test_app::narrow_and_wide_formattable& nwf,
                FormatContext& ctx) const -> decltype(ctx.out())
    {
        auto out = std::format_to(ctx.out(), LR"("{}" has lasted )", nwf.wname());
        ctx.advance_to(out);
        out = base_t::format(nwf.duration().count(), ctx);
        ctx.advance_to(out);
        return std::format_to(ctx.out(), L" seconds.");
    }
};

// ---------------------------------------------------------------------------
// Registration for the *synthesized* wide stream inserter.
//
// MUST come AFTER the wide fmt::formatter above: g_k_wide_stream_insert_via_fmt_format
// is constrained (template<fmt_wide_formattable T>), and that concept is only
// satisfied once fmt::format(L"{}", t) is viable -- i.e. once the wide formatter
// is *defined*, not merely declared. It must also live INSIDE the include guard.
//
// We deliberately do NOT register the narrow flag: a narrow operator<< is
// hand-written (in the .cpp), and registering an already-insertable type invites
// ambiguity. We also pick exactly one of the std/fmt flavors (fmt here), since
// this type is both std_wide_formattable and fmt_wide_formattable and the
// concepts reject anything registered for both.
// ---------------------------------------------------------------------------
namespace cps::ct_string
{
    template<>
    inline constexpr bool
        g_k_wide_stream_insert_via_fmt_format<test_app::narrow_and_wide_formattable> = true;
}

namespace cps::ct_string::test_app
{
    // ---------------------------------------------------------------------
    // REQUIRED, and easy to miss: this namespace declares its own operator<<
    // (the narrow one). Unqualified lookup stops at the first enclosing scope
    // that declares the name -- so it stops HERE and never reaches
    // cps::ct_string. ADL doesn't rescue us either: the associated namespace of
    // narrow_and_wide_formattable is only cps::ct_string::test_app; ADL does
    // NOT consider enclosing namespaces. Without this using-declaration,
    // `some_wostream << nwf` fails with "no match for operator<<".
    // ---------------------------------------------------------------------
    using cps::ct_string::operator<<;
}

#endif //HAND_EVALUATOR_FMT_REG_DEMO_HPP
