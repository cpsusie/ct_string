#ifndef CSTR_VIEW_IMPL_VIEWS_HPP
#define CSTR_VIEW_IMPL_VIEWS_HPP
#include "ct_str/ct_string_view.hpp"
#include <string_view>

namespace cjm::ct_string::example_impl
{
    using namespace std::literals;
    using namespace literals;

    extern const ct_cstring_view g_k_padded_evangeline;
    extern const ct_string_view g_k_evangeline;
    extern const ct_cstring_view g_k_pre;
    extern const ct_cstring_view g_k_post;
    extern const ct_cstring_view g_k_forest;
    extern const ct_cstring_view g_k_voices;
}
#endif //CSTR_VIEW_IMPL_VIEWS_HPP