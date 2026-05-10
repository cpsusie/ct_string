#include <iostream>
#include "../inc/ct_str/ct_string_view.hpp"

using cjm::ct_string::literals::operator ""_fs;
using namespace std::literals;
int main()
{
    constexpr auto expected = "This is the forest primaeval, the murmuring pines and the hemlocks,"sv;

    constexpr auto expected_fs = "This "_fs + "is the forest primaeval,"_fs + " the murmuring pines and the hemlocks,"_fs;
    static_assert(expected_fs.valid_cstr());
    static_assert(expected_fs == expected);
    static_assert(expected_fs.size() == expected.size());

    [[maybe_unused]] constexpr char bad_array[] = {'h', 'e', 'l', 'l', 'o'}; // Not a valid C-string (missing null terminator)
    [[maybe_unused]] constexpr char also_bad_array[] = "Hel\0lo"; // Not a valid C-string (null terminator in body)
    //constexpr auto fixed_bad = cjm::ct_string::basic_fixed_string{bad_array}; // Should trigger compile-time error
    //constexpr auto also_fixed_bad = cjm::ct_string::basic_fixed_string{also_bad_array}; // Should trigger compile-time error
    constexpr auto fixed = "Hello, World!"_fs;
    constexpr auto empty = ""_fs;
    static_assert(empty.valid_cstr());
    static_assert(empty.empty());
    static_assert(empty.size() == 0);
    static_assert(fixed.valid_cstr());


    static_assert (fixed == "Hello, World!");

    // Borrowed range / view sanity checks.
    using cjm::ct_string::ct_cstring_view;
    static_assert(std::ranges::borrowed_range<ct_cstring_view>);
    static_assert(std::ranges::view<ct_cstring_view>);
    static_assert(std::ranges::contiguous_range<ct_cstring_view>);

    // Hash equivalence with std::string_view.
    using cjm::ct_string::literals::operator""_ctsv;
    constexpr auto ctsv = "Hello, World!"_ctsv;
    const auto sv = std::string_view{"Hello, World!"};
    if (std::hash<ct_cstring_view>{}(ctsv) != std::hash<std::string_view>{}(sv))
    {
        std::cerr << "Hash mismatch!\n";
        return 1;
    }

    std::cout << "Hello, World!" << std::endl;
    return 0;
}