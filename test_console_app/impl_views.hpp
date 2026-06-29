#ifndef CSTR_VIEW_IMPL_VIEWS_HPP
#define CSTR_VIEW_IMPL_VIEWS_HPP
#include "ct_str/ct_string_view.hpp"
#include <string_view>

namespace cps::ct_string::example_impl
{
    using namespace std::literals;
    using namespace literals;

    //Example: global constants

    //Note: unlike when putting definition in header, the value referred to by this
    //ct_sv's is not static_assertable because they are not (in this context) manifestly constant
    //expressions (their definition is invisible).
    //You can still static_assert in .cpp file where defined, as long as definition is visible

    // Since the definition cannot be seen, you cannot use "auto" in the header
    // The opening text of evangeline with leading and trailing whitespace
    extern const ct_cstring_view g_k_padded_evangeline;
    // The opening text of evangeline with the whitespace trimmed.  Note,
    // this is the only non-cstring view in this header
    extern const ct_string_view g_k_evangeline; // Not a cstring: we know it will be result of trimming padded
    // "pre"
    extern const ct_cstring_view g_k_pre;
    // "post"
    extern const ct_cstring_view g_k_post;
    // "forest"
    extern const ct_cstring_view g_k_forest;
    // "voices"
    extern const ct_cstring_view g_k_voices;


    //Example member static constants
    struct demo
    {
        // "Christopher P. Susie"
        static const ct_cstring_view s_k_author_name;
    };

}
#endif //CSTR_VIEW_IMPL_VIEWS_HPP