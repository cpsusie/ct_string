#include <iostream>
#include <chrono>
#include <format>
#include "ct_str/ct_string_view.hpp"
#include "header_only_views.hpp"
#include "impl_views.hpp"
#include <cassert>
#include <fmt/format.h>
#include <fmt/xchar.h>
#include <print>
#include "ct_str/ctsv_assoc_container.hpp"

static constexpr auto newl = '\n';
namespace cps::ct_string::stream_per_format_test
{
    using namespace std::literals;
    using namespace std::chrono_literals;

    namespace CHR = std::chrono;
    namespace RG = std::ranges;
    namespace VW = std::ranges::views;
    using years_t = CHR::years;

    class animal;

    template<typename T>
    using wo_qual_t = std::remove_cvref_t<T>;

    template<typename T1, typename T2>
    concept same_wo_qual = std::same_as<wo_qual_t<T1>, wo_qual_t<T2>>;

    template<typename TChar>
    concept first_class_char = same_wo_qual<char, TChar> || same_wo_qual<wchar_t, TChar>;

    namespace detail
    {
        template<first_class_char TChar>
        inline constexpr std::basic_string_view<TChar> g_k_yrs_fmt_str_v;

        template<>
        inline constexpr std::basic_string_view<char> g_k_yrs_fmt_str_v<char> = "{:L} yrs"sv;

        template<>
        inline constexpr std::basic_string_view<wchar_t> g_k_yrs_fmt_str_v<wchar_t> = L"{:L} yrs"sv;

        template<first_class_char TChar>
        inline constexpr std::basic_string_view<TChar> g_k_common_fields_fmt_str_v;

        template<>
        inline constexpr std::basic_string_view<char> g_k_common_fields_fmt_str_v<char> = "Id: [{:L}], Name: \"{}\", Age: [{}]. \tDetail:"sv;

        template<>
        inline constexpr std::basic_string_view<wchar_t> g_k_common_fields_fmt_str_v<wchar_t> = L"Id: [{:L}], Name: \"{}\", Age: [{}]. \tDetail:"sv;
    }

    template<first_class_char TChar>
    static void write_to_stream(std::basic_ostream<TChar>& os, years_t period)
    {
        fmt::print(os, detail::g_k_yrs_fmt_str_v<TChar>, period.count());
    }

    template<typename T>
    concept derived_from_animal = std::derived_from<T, animal>;

    namespace detail
    {
        template<derived_from_animal T>
        struct fmt_helper_impl
        {
            using animal_type = wo_cv_t<T>;
            using string_type = std::string;
            using ostream_type = std::ostream;
            using sv_type = std::string_view;

            friend struct fmt::formatter<T>;
            constexpr fmt_helper_impl() noexcept = default;
            constexpr fmt_helper_impl(const T& obj) noexcept : mp_k_animal{std::addressof(obj)} {}
            constexpr fmt_helper_impl(std::nullptr_t) noexcept : fmt_helper_impl{} {}

            constexpr fmt_helper_impl& operator=(std::nullptr_t) noexcept
            {
                mp_k_animal = nullptr;
                return *this;
            }

            constexpr explicit operator bool() const noexcept { return mp_k_animal != nullptr; }

            constexpr bool operator==(const fmt_helper_impl& rhs) const noexcept
            {
                return mp_k_animal == rhs.mp_k_animal;
            }

            [[nodiscard]] auto operator()() const-> string_type;

        private:
            const T* mp_k_animal = nullptr;
        };


    }

}

template<cps::ct_string::stream_per_format_test::derived_from_animal TDerived>
struct fmt::formatter<TDerived>;

namespace cps::ct_string::stream_per_format_test
{
    void run_dog_test(std::ostream& os, std::ostream& err);

    class animal
    {
    public:
        template<derived_from_animal TDerived>
        friend struct detail::fmt_helper_impl;

        [[nodiscard]] years_t age() const noexcept { return m_age; }
        [[nodiscard]] std::uint64_t id() const noexcept { return m_id; };
        [[nodiscard]] std::string_view name() const&& = delete;
        [[nodiscard]] std::string_view name() const& noexcept
        {
            return exec_name();
        }

        std::string name()&& noexcept
        {
            return std::move(*this).exec_name();
        }

        [[nodiscard]] static std::uint64_t highest_used_id() noexcept
        {
            return s_next_id.load(std::memory_order_acquire);
        }

        virtual ~animal() noexcept = default;

        [[nodiscard]] bool operator==(const animal& rhs) const noexcept
        {
            return m_id == rhs.m_id && typeid(*this) == typeid(rhs);
        }

        std::string emit_animal_noise()
        {
            return exec_n_make_animal_sound();
        }

        std::wstring wide_emit_animal_noise()
        {
            return exec_w_make_animal_sound();
        }

        void emit_animal_noise(std::ostream& os)
        {
            write_noise_to_stream(os);
        }

        void emit_animal_noise(std::wostream& os)
        {
            write_noise_to_stream(os);
        }

        [[nodiscard]] std::string to_string() const
        {
            auto strm = std::stringstream{};
            write_self_details(strm);
            return std::move(strm).str();
        }

        // friend std::ostream& operator<<(std::ostream& os, const animal& rhs)
        // {
        //     rhs.write_to_stream(os);
        //     return os;
        // }
        //
        // friend std::wostream& operator<<(std::wostream& os, const animal& rhs)
        // {
        //     rhs.write_to_stream(os);
        //     return os;
        // }

    protected:

        [[nodiscard]] virtual std::string_view exec_name() const& noexcept = 0;
        virtual std::string&& exec_name()&& noexcept = 0;
        [[nodiscard]] std::string_view exec_name() const&& = delete;

        virtual std::string exec_n_make_animal_sound() = 0;
        virtual std::wstring exec_w_make_animal_sound() = 0;

        void write_self_details(std::ostream& os) const
        {
            std::string years_old = [](years_t y)-> std::string {
                auto strm = std::stringstream{};
                write_to_stream(strm, y);
                return std::move(strm).str();
            }(m_age);
            static constexpr auto fmt_str = detail::g_k_common_fields_fmt_str_v<char>;
            fmt::println(os, fmt_str, m_id, exec_name(), years_old);
            exec_write_self_details(os);
            os << newl << newl;
        }

        virtual void exec_write_self_details(std::ostream& os) const = 0;



        void write_noise_to_stream(std::ostream& os)
        {
            os << exec_n_make_animal_sound();
        }
        void write_noise_to_stream(std::wostream& os)
        {
            os << exec_w_make_animal_sound();
        }

        animal() noexcept : m_id{get_next_id()} {}
        explicit animal(years_t age) noexcept : animal() { m_age = age; }
        animal(const animal&) noexcept = default;
        animal& operator=(const animal&) noexcept = default;
        animal(animal&&) noexcept = default;
        animal& operator=(animal&&) noexcept = default;

    private:

        std::uint64_t m_id{};
        years_t m_age{};

        [[nodiscard]] static std::uint64_t get_next_id() noexcept
        {
            return s_next_id.fetch_add(1ULL, std::memory_order_acq_rel) + 1ULL;
        }

        static std::atomic<std::uint64_t> s_next_id;
    };

    constinit std::atomic<std::uint64_t> animal::s_next_id = {0ULL};

    namespace detail
    {
        template<derived_from_animal T>
        auto fmt_helper_impl<T>::operator()() const -> string_type
        {
            auto ret = ""s;
            if (mp_k_animal)
            {
                const animal& ann = *mp_k_animal;
                ret = ann.to_string();
            }
            return ret;
        }
    }
}

using cps::ct_string::literals::operator ""_fs;
using namespace std::literals;

void demo_impl_views(std::ostream& os)
{
    using namespace cps::ct_string::example_impl;
    assert(g_k_padded_evangeline.known_cstr);
    assert(g_k_padded_evangeline.c_str()[g_k_padded_evangeline.size()] == char{});
    assert(g_k_evangeline.front() == 'T');
    assert(g_k_evangeline.back() == '.');

    os << fmt::format("g_k_padded_evangeline: \n\"{}\"\n\n", g_k_padded_evangeline);
    os << fmt::format("g_k_evangeline: \n\"{}\"\n\n", g_k_evangeline);
    os << fmt::format("g_k_pre:  \n\"{}\"\n\n", g_k_pre);
    os << fmt::format( "g_k_post: \"{}\"\n\n", g_k_post);
    os << fmt::format( "g_k_forest: \"{}\"\n\n", g_k_forest);
    os << fmt::format( "g_k_voices: \"{}\"\n\n", g_k_voices);
}

int main()
{
    constexpr auto expected = "This is the forest primaeval, the murmuring pines and the hemlocks,"sv;

    constexpr auto expected_fs = "This "_fs + "is the forest primaeval,"_fs + " the murmuring pines and the hemlocks,"_fs;
    static_assert(expected_fs.valid_cstr());
    static_assert(expected_fs == expected);
    static_assert(expected_fs.size() == expected.size());

    [[maybe_unused]] constexpr char bad_array[] = {'h', 'e', 'l', 'l', 'o'}; // Not a valid C-string (missing null terminator)
    [[maybe_unused]] constexpr char also_bad_array[] = "Hel\0lo"; // Not a valid C-string (null terminator in body)
    //constexpr auto fixed_bad = cps::ct_string::basic_fixed_string{bad_array}; // Should trigger compile-time error
    //constexpr auto also_fixed_bad = cps::ct_string::basic_fixed_string{also_bad_array}; // Should trigger compile-time error
    constexpr auto fixed = "Hello, World!"_fs;
    constexpr auto empty = ""_fs;
    static_assert(empty.valid_cstr());
    static_assert(empty.empty());
    static_assert(empty.size() == 0);
    static_assert(empty.size() == empty.length());
    static_assert(empty.buff_size() == 1);
    static_assert(empty.buff_size() == empty.buff_length());
    static_assert(fixed.valid_cstr());


    static_assert (fixed == "Hello, World!");

    // Borrowed range / view sanity checks.
    using cps::ct_string::ct_cstring_view;
    static_assert(std::ranges::borrowed_range<ct_cstring_view>);
    static_assert(std::ranges::view<ct_cstring_view>);
    static_assert(std::ranges::contiguous_range<ct_cstring_view>);

    // Hash equivalence with std::string_view.
    using cps::ct_string::literals::operator""_ctsv;
    constexpr auto ctsv = "Hello, World!"_ctsv;
    const auto sv = std::string_view{"Hello, World!"};
    if (std::hash<ct_cstring_view>{}(ctsv) != std::hash<std::string_view>{}(sv))
    {
        std::cerr << "Hash mismatch!\n";
        return 1;
    }

    std::cout << "Hello, World!" << newl;

    demo_impl_views(std::cout);

    std::cout << "Doggie test: " << newl;
    cps::ct_string::stream_per_format_test::run_dog_test(std::cout, std::cerr);
    return 0;
}
using namespace cps::ct_string::literals;

namespace cps::ct_string::stream_per_format_test
{
    using namespace cps::ct_string::literals;
    class sensible_animal : public animal
    {
    public:
        ~sensible_animal() noexcept override = default;
        sensible_animal() noexcept = delete;
    protected:

        sensible_animal(string_source auto&& name, years_t age) noexcept(nothrow_string_source<decltype(name)>) :
            animal{age}, m_name{std::forward<decltype(name)>(name)} {}
        sensible_animal(const sensible_animal&) = default;
        sensible_animal(sensible_animal&&) noexcept = default;
        sensible_animal& operator=(const sensible_animal&) = default;
        sensible_animal& operator=(sensible_animal&&) noexcept = default;

        [[nodiscard]] virtual ct_cstring_view fav_activity_label() const noexcept = 0;

        [[nodiscard]] virtual int get_activity_ct() const noexcept = 0;

    private:

        void exec_write_self_details(std::ostream& os) const final
        {
            return fmt::print(os, "{}\t{}: [{:L}]",newl, fav_activity_label(),  get_activity_ct());
        }


        [[nodiscard]] std::string&& exec_name()&& noexcept final
        {
            return std::move(m_name);
        }

        [[nodiscard]] std::string_view exec_name() const& noexcept final
        {
            return m_name;
        }


        std::string m_name{};
    };
    class dog : public sensible_animal
    {
    public:

        [[nodiscard]] int num_bones_since_thursday() const noexcept
        {
            return m_bones_eaten_since_thurs;
        }

        ~dog() noexcept override = default;
        dog(string_source auto&& name, years_t age, int num_bones = 3) noexcept(nothrow_string_source<decltype(name)>) :
            sensible_animal{std::forward<decltype(name)>(name), age}, m_bones_eaten_since_thurs{num_bones} {}
        dog(const dog&) = default;
        dog(dog&&) noexcept = default;
        dog& operator=(const dog&) = default;
        dog& operator=(dog&&) noexcept = default;

    protected:

        [[nodiscard]] int get_activity_ct() const noexcept final
        {
            return m_bones_eaten_since_thurs;
        }


        std::string exec_n_make_animal_sound() override
        {
            if (++m_bones_eaten_since_thurs < 12)
            {
                return "Grrr... WOOF... GRRR!"s;
            }
            return "Arf! woof!  Bow wowowow, Bark!"s;
        }

        std::wstring exec_w_make_animal_sound() override
        {
            if (++m_bones_eaten_since_thurs < 12)
            {
                return L"Grrr... WOOF... GRRR!"s;
            }
            return L"Arf! woof!  Bow wowow, Bark!"s;
        }
        [[nodiscard]] ct_cstring_view fav_activity_label() const noexcept final
        {
            return "Number of Bones Eaten Since Thursday"_ctsv;
        }
    private:
        int m_bones_eaten_since_thurs{0};

    };


}

namespace cps::ct_string {
    template<>
       inline constexpr bool g_k_stream_insert_via_fmt_format<stream_per_format_test::dog> = true;
}

namespace cps::ct_string::stream_per_format_test
{
    using std::chrono_literals::operator""y;
    void run_dog_test(std::ostream &os, std::ostream &err)
    {
        os << "Starting dog test...."sv;
        auto virgil = dog{"Virgil"sv, years_t{5}, 42};
        os << virgil << newl;

        err << "No messages like this, no error!" << newl;
    }
}
template<cps::ct_string::stream_per_format_test::derived_from_animal TDerived>
struct fmt::formatter<TDerived> :  fmt::formatter<std::string_view>
{
    using animal_type = cps::ct_string::stream_per_format_test::wo_qual_t<TDerived>;
    template<typename FormatContext>
    auto format(const animal_type& v, FormatContext& ctx) const
    {
        return fmt::formatter<std::string_view>::format(v.to_string(), ctx);
    }
};