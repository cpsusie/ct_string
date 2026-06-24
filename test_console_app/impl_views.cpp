#include "impl_views.hpp"

namespace cjm::ct_string::example_impl
{
  template<typename TCStrHaver>
  concept has_c_str = requires (const TCStrHaver& x)
  {
    { x.c_str() } noexcept;
  };

  static constexpr auto ascii_whitespace =  " \t\n\v\f\r"_ctsv;


  static constexpr ct_string_view trim(ct_string_view sv,
                      ct_string_view chars = ascii_whitespace) noexcept
  {
    const auto first = sv.find_first_not_of(chars);

    if (first == ct_string_view::npos)
    {
      return {};
    }

    const auto last = sv.find_last_not_of(chars);

    sv.remove_prefix(first);
    sv.remove_suffix(sv.size() - (last - first + 1));

    return sv;
  }

   constinit const ct_cstring_view g_k_padded_evangeline =
      R"(     THIS is the forest primeval. The murmuring pines and the hemlocks,
  Bearded with moss, and in garments green, indistinct in the twilight,
  Stand like Druids of eld, with voices sad and prophetic,
  Stand like harpers hoar, with beards that rest on their bosoms.
  Loud from its rocky caverns, the deep-voiced neighboring ocean
  Speaks, and in accents disconsolate answers the wail of the forest.

    This is the forest primeval; but where are the hearts that beneath it
  Leaped like the roe, when he hears in the woodland the voice of the huntsman?
  Where is the thatch-roofed village, the home of Acadian farmers,—
  Men whose lives glided on like rivers that water the woodlands,
  Darkened by shadows of earth, but reflecting an image of heaven?
  Waste are those pleasant farms, and the farmers forever departed!
  Scattered like dust and leaves, when the mighty blasts of October
  Seize them, and whirl them aloft, and sprinkle them far o'er the ocean.
  Naught but tradition remains of the beautiful village of Grand-Pré.

    Ye who believe in affection that hopes, and endures, and is patient,
  Ye who believe in the beauty and strength of woman's devotion,
  List to the mournful tradition still sung by the pines of the forest;
  List to a Tale of Love in Acadie, home of the happy.       )"_ctsv;

  constinit const ct_string_view g_k_evangeline = trim(g_k_padded_evangeline);

  constinit const ct_cstring_view g_k_pre = "pre"_ctsv;
  constinit const ct_cstring_view g_k_post = "post"_ctsv;
  constinit const ct_cstring_view g_k_forest = "forest"_ctsv;
  constinit const ct_cstring_view g_k_voices = "voices"_ctsv;

  static_assert(g_k_padded_evangeline.known_cstr);
  static_assert(!g_k_evangeline.known_cstr);
  static_assert(has_c_str<decltype(g_k_padded_evangeline)>);

}