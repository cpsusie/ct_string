#include "setup_streams.hpp"
#include <clocale>
#include <iostream>
#include <locale>
#include <fmt/format.h>

namespace cps::ct_string::test_app
{

    std::expected<void, std::string> setup_streams(ct_cstring_view locale_txt,
        [[maybe_unused]] std::span<const std::string_view> args) noexcept
    {
        try
        {
            // 1. Set the C standard library locale (affects printf, scanf, etc.)
            std::setlocale(LC_ALL, "en_US.UTF-8");

            // 2. Set the C++ standard library global locale (affects new streams)
            std::locale::global(std::locale("en_US.UTF-8"));

            // 3. Imbue existing standard streams so they use the new locale
            std::wcout.imbue(std::locale());
            std::wcin.imbue(std::locale());
            std::cout.imbue(std::locale());
            std::cin.imbue(std::locale());
            std::cerr.imbue(std::locale());
            std::wcerr.imbue(std::locale());
        }
        catch (const std::exception& e)
        {
            return std::unexpected{fmt::format(
                "Failed to set locale to \"{}\" with error: <{}>."sv, locale_txt, e.what())};
        }

        std::ios_base::sync_with_stdio(false);
        std::cout.tie(nullptr);
        std::cin.tie(nullptr);
        std::cerr.tie(nullptr);

        std::cout << "Locale set to: " << locale_txt << "\n";
        if (!args.empty() && !args.front().empty())
        {
            std::cout << "Program: [" << args.front() << "],\n";
        }

        return {};
    }
}