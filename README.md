# cstr_view

A small, header-only C++20 library providing two complementary types built on
`std::basic_string_view`:

| Type | Header | What it is |
|---|---|---|
| `cps::ct_string::basic_fixed_string<TChar, N>` | `<ct_str/fixed_string.hpp>` | A fixed-capacity, null-terminated character buffer that is usable as a non-type template parameter (NTTP). |
| `cps::ct_string::basic_ct_string_view<TChar, VALID_CSTR>` | `<ct_str/ct_string_view.hpp>` | A non-owning view that — when `VALID_CSTR == true` — is *statically known* to refer to a null-terminated, static-storage-duration character buffer. Safe to pass to C APIs. Cannot dangle. |

If you have ever wanted to write something like

```c++
constexpr auto sv = "hello"_ctsv;            // a string_view-like type
const char*  cs   = sv.c_str();               // ...that ALSO has c_str()
                                              //    and CANNOT dangle.
```

…this library is for you.

---

## Table of contents

- [Why not just `std::string_view`?](#why-not-just-stdstring_view)
- [The two types at a glance](#the-two-types-at-a-glance)
- [Quick start](#quick-start)
- [Use case 1: `c_str()` without copying or allocating](#use-case-1-c_str-without-copying-or-allocating)
- [Use case 2: views that cannot dangle](#use-case-2-views-that-cannot-dangle)
- [Use case 3: a string as a non-type template parameter](#use-case-3-a-string-as-a-non-type-template-parameter)
- [Use case 4: heterogeneous lookup in maps & sets](#use-case-4-heterogeneous-lookup-in-maps--sets)
- [Use case 5: formatting](#use-case-5-formatting)
- [The two flavors of `basic_ct_string_view`](#the-two-flavors-of-basic_ct_string_view)
- [Defining Static-Member or Global Constants](#Defining-Static-Member-or-Global-Constants)
- [Comparison cheat-sheet](#comparison-cheat-sheet)
- [Building & integrating](#building--integrating)
- [Requirements](#requirements)
- [License](#license)

---

## Why not just `std::string_view`?

`std::string_view` is excellent for read-only string parameters but has two
properties that bite in production code:

1. **It does not promise null-termination.** A `std::string_view` may be a
   slice of a larger string. If you need a `const char*` to pass to a C API
   (POSIX, Win32, OpenSSL, SQLite, libcurl, `fopen`, …) you cannot get one
   from a `string_view` — you must copy into a `std::string` first.

2. **It can dangle.** Nothing in the type stops you from constructing one
   from a temporary `std::string` and outliving it. Some such cases are
   diagnosed; many are not.

`basic_ct_string_view` addresses both:

1. The `VALID_CSTR == true` (a.k.a. **`known_cstr`**) flavor exposes
   `c_str()` returning a real, guaranteed-null-terminated `const TChar*`.
   No copy, no allocation, no `strlen`.

2. Every `basic_ct_string_view` is constructible **only** from compile-time
   data (a `basic_fixed_string` NTTP or its own factory) or from another
   `basic_ct_string_view`. The pointed-to characters always have static
   storage duration, so the view *cannot* dangle.

`basic_fixed_string` is the storage type that backs all of this and has its
own use case: as a literal you can pass as a template argument.

---

## The two types at a glance

```c++
#include <ct_str/ct_string_view.hpp>

using namespace cps::ct_string;            // basic_fixed_string, basic_ct_string_view, ...
using namespace cps::ct_string::literals;  // _fs, _ctsv

constexpr auto fs   = "hello"_fs;       // basic_fixed_string<char, 6>
constexpr auto ctsv = "hello"_ctsv;     // basic_ct_string_view<char, true> == ct_cstring_view

static_assert(fs.size()    == 5);
static_assert(ctsv.size()  == 5);
static_assert(ctsv.known_cstr);
static_assert(ctsv == fs);              // also == "hello", == std::string_view{"hello"}, ...
```

Convenience aliases:

| Alias | = |
|---|---|
| `ct_cstring_view`  | `basic_ct_string_view<char,    true>`  (has `c_str()`) |
| `ct_string_view`   | `basic_ct_string_view<char,    false>` (`string_view`-shaped, no `c_str()`) |
| `ct_wcstring_view` | `basic_ct_string_view<wchar_t, true>`  |
| `ct_wstring_view`  | `basic_ct_string_view<wchar_t, false>` |
| `ct_u8cstring_view` / `ct_u8string_view`   | `char8_t`  variants |
| `ct_u16cstring_view` / `ct_u16string_view` | `char16_t` variants |
| `ct_u32cstring_view` / `ct_u32string_view` | `char32_t` variants |

---

## Quick start

```c++
#include <ct_str/ct_string_view.hpp>
#include <cstdio>

using namespace cps::ct_string::literals;

void greet(cps::ct_string::ct_cstring_view name) {
    // c_str() is guaranteed valid: zero-copy, null-terminated, no allocation.
    std::printf("Hello, %s!\n", name.c_str());
}

int main() {
    greet("world"_ctsv);                 // OK: literal -> compile-time NTTP storage
    constexpr auto n = "Alice"_ctsv;
    greet(n);                            // OK
}
```

---

## Use case 1: `c_str()` without copying or allocating

Anywhere you currently see this pattern:

```c++
void log(std::string_view msg) {
    std::string copy{msg};               // allocate, just to get a c_str()
    ::syslog(LOG_INFO, "%s", copy.c_str());
}
```

…you can replace it with this when the caller can supply a compile-time
string:

```c++
void log(cps::ct_string::ct_cstring_view msg) {
    ::syslog(LOG_INFO, "%s", msg.c_str()); // no allocation
}
```

`c_str()` only exists on the `known_cstr == true` flavor, so the type
system will *prevent* you from accidentally calling it on a view that may
not be null-terminated.

---

## Use case 2: views that cannot dangle

The `ct_cstring_view` and `ct_string_view` types document the fact that they cannot dangle; the way they are constructed guarantees it.  Only compile time constant strings with static storage duration can be bound to them.

```c++
auto bad() -> std::string_view {
    std::string s = "hello";
    return s;                  // dangling: s dies on return. UB at the call site.
}

auto good() -> cps::ct_string::ct_cstring_view {
    using namespace cps::ct_string::literals;
    return "hello"_ctsv;       // OK: backing storage is a static-duration NTTP
}
```

`basic_ct_string_view`'s only public constructors are:

- the same-type / cross-flavor copy/move constructors,
- a `consteval` factory path (`_ctsv`, `make_ctsv`, `basic_ct_sv_factory`)
  that takes a `basic_fixed_string` NTTP — whose buffer is by definition a
  static-duration object.

There is **no** public constructor from a runtime `const char*`, a
`std::string&`, or a `std::string_view`. That eliminates the most common
sources of dangling views.

(The library opts the type into `std::ranges::enable_borrowed_range`, so
even rvalue-returning expressions like `std::ranges::find("x"_ctsv, 'y')`
do not yield `std::ranges::dangling`.)

**Why is this important?**  `std::string_view` is often used for three purposes:
1. Storing string literals as a superior alternative to `const char*`
  * without losing size information and
  * gaining access to (read-only) std::library string interface 
2. As function arguments where the actual type to which the string-view is unimportant
3. Providing O(1) non-allocating substring operations (e.g. tokenizing) 
With #1, the string literal referred to by the `const std::string_view` will never dangle.  For ##2-3, the `std::string_view`s used are subject to dangling to the same extent that a const std::string& is subject to outliving the object to which it was bound.  The `std::string_view` type, however, provides no such guarantees.  

Imagine:

```c++
auto names_ids = std::map<std::string_view, int>
{
	{g_k_annabelle, 1}, 
	{g_k_benjamin, 2}, 
	{g_k_christina, 3}
};

// ...... stuff happens

std::string david = "David";
names_ids[david] = 4;	
```

We have a lookup above intented to store compile-time constants string literals as keys.  In some other context, however, someone decides to add a `std::string` to the map.  There will be no explicit cast required and no warning: `std::string` implicitly converts to `std::string_view`, by design.  Obviously, the result may not turn out well.
[Godbolt Demo](https://godbolt.org/z/h8T3soWqW) 

If instead of `std::string_view` as a key, the map had been `std::map<ct_cstring_view, int>` (if we care about null-termination) or `std::map<ct_string_view, int>` (if null-termination is irrelevant), we would have both:
1. Documented our intent that the map is designed only to hold string literals and
2. Enforced our intent at compile-time: no runtime checks necessary, code that attempts to add something non-conforming simply will not compile

---

## Use case 3: a string as a non-type template parameter
`basic_fixed_string` satisfies the structural-type rules and works directly
as a non-type template parameter. This is the most powerful use of the
library.

### Tagging a type with a compile-time name

```c++
#include <ct_str/fixed_string.hpp>
#include <iostream>

using cps::ct_string::basic_fixed_string;

template<basic_fixed_string Name, typename T>
struct named {
    T value;

    void print() const {
        std::cout << Name.get_std_sv() << " = " << value << '\n';
    }
};

int main() {
    named<"width",  int>    w{1920};
    named<"height", int>    h{1080};
    named<"label", const char*> l{"hello"};
    w.print();   // width = 1920
    h.print();   // height = 1080
    l.print();   // label = hello
}
```

Notice the literal `"width"` appearing as a *template argument*: the array
is implicitly converted to a `basic_fixed_string<char, 6>` via its
`consteval` constructor and the class template's deduction guide.

### Compile-time-validated string operations

Because the value is part of the type, anything you compute from it can be
checked at compile time:

```c++
template<basic_fixed_string Path>
struct route {
    static_assert(Path.size() > 0,                          "route must be non-empty");
    static_assert(Path.front() == '/',                      "route must start with '/'");
    static_assert(std::ranges::find(Path, ' ') == Path.end(),
                  "route must not contain spaces");

    static constexpr auto value = Path;
};

route<"/api/v1/users"> users;            // OK
// route<"api/v1/users"> bad;            // compile error: route must start with '/'
```

### Concatenation as a `consteval` operation

`operator+` on two `basic_fixed_string`s is `consteval` and produces a new
`basic_fixed_string` whose length is the exact sum of the two operand
lengths (sans null terminator):

```c++
using namespace cps::ct_string::literals;
constexpr auto greeting = "hello"_fs + ", "_fs + "world"_fs; // "hello, world"
static_assert(greeting == "hello, world");
static_assert(greeting.valid_cstr());
```

You can use the result of concatenation as another NTTP, allowing
generic programming over compile-time-built strings.

### Bridging `basic_fixed_string` (NTTP) and `basic_ct_string_view` (view)

```c++
template<basic_fixed_string Greeting>
auto get_greeting() {
    return cps::ct_string::make_ctsv<Greeting>();   // ct_cstring_view
}

constexpr auto g = get_greeting<"hi there">();
static_assert(g == "hi there");
const char* p = g.c_str();   // points into a static-storage NTTP buffer
```

`make_ctsv<...>()` (and the `_ctsv` literal) materialize a
`ct_cstring_view` whose `data()` aims into the static-duration NTTP buffer
held by `basic_ct_sv_factory<...>::fstr_val`. The buffer outlives the
program, so the view can be freely copied, returned, stored, and passed to
C APIs.

---

## Use case 4: heterogeneous lookup in maps & sets

Both flavors of `basic_ct_string_view` ship with:

- a transparent `std::hash` specialization,
- transparent `operator==` / `operator<=>` overloads against any type
  nothrow-convertible to the corresponding `std::basic_string_view`
  (covers `std::basic_string`, `std::basic_string_view`, the opposite
  flavor of view, and `basic_fixed_string` of the same character type).

The library exposes pre-wired container aliases that engage C++20
heterogeneous lookup without any user-side template gymnastics:

```c++
#include <ct_str/ct_string_view.hpp>
#include <string>
#include <string_view>

using namespace cps::ct_string;
using namespace cps::ct_string::literals;

ct_cstring_view_unordered_set s;           // unordered_set<ct_cstring_view, hash, equal_to<>>
s.insert("hello"_ctsv);
s.insert("world"_ctsv);

bool a = s.contains(std::string_view{"hello"});  // heterogeneous, no temporary view
auto fs = "hello"_fs;
bool c = s.contains(fs);                         // heterogeneous against basic_fixed_string

ct_cstring_view_map<int> m;                // map<ct_cstring_view, int, less<>>
m.emplace("alpha"_ctsv, 1);
auto it = m.find(std::string_view{"alpha"});     // heterogeneous
```

Available aliases (each has a `_set` / `_map<TValue>` / `_unordered_set` /
`_unordered_map<TValue>` form, plus generic templates parameterised on
`TChar` / `VALID_CSTR`):

| Concrete alias | Container |
|---|---|
| `ct_cstring_view_set`  / `ct_string_view_set`  / `ct_wcstring_view_set`  / `ct_wstring_view_set`  | `std::set<..., std::less<>>` |
| `ct_cstring_view_map<V>`  / `ct_string_view_map<V>`  / `ct_wcstring_view_map<V>`  / `ct_wstring_view_map<V>`  | `std::map<..., V, std::less<>>` |
| `ct_cstring_view_unordered_set`  / `ct_string_view_unordered_set`  / `ct_wcstring_view_unordered_set`  / `ct_wstring_view_unordered_set`  | `std::unordered_set<..., std::hash<...>, std::equal_to<>>` |
| `ct_cstring_view_unordered_map<V>` / `ct_string_view_unordered_map<V>` / `ct_wcstring_view_unordered_map<V>` / `ct_wstring_view_unordered_map<V>` | `std::unordered_map<..., V, std::hash<...>, std::equal_to<>>` |

Why is this nontrivial? C++20 heterogeneous lookup in unordered containers
requires both `Hash::is_transparent` *and* `KeyEqual::is_transparent`.
The library's `std::hash` specialization is transparent; the aliases
additionally pass `std::equal_to<>` (which is also transparent and works
because of the comparison-operator templates on the view) so that the
ergonomic call sites above just work. See the doc comments in
`ct_string_view.hpp` for the rationale and caveats (notably that lookup
keys must be nothrow-convertible to the corresponding
`std::basic_string_view`, which excludes raw `const TChar*`).

---

## Use case 5: formatting

The library provides `std::formatter` (and, when `<fmt/format.h>` is on the
include path, `fmt::formatter`) specializations for `char` and `wchar_t`
flavors of `basic_ct_string_view`. They inherit from the standard
string-view formatter, so the full string-view format spec is supported:

```c++
#include <ct_str/ct_string_view.hpp>
#include <format>

using namespace cps::ct_string::literals;

auto a = std::format("{}",       "hi"_ctsv);     // "hi"
auto b = std::format("[{:>5}]",  "hi"_ctsv);     // "[   hi]"
auto c = std::format("[{:*<5}]", "hi"_ctsv);     // "[hi***]"
auto d = std::format("{:.3}",    "hello"_ctsv);  // "hel"
auto w = std::format(L"{}",      cps::ct_string::make_ctsv<L"wide">()); // L"wide"
```

If you also include `<fmt/format.h>` *before* `<ct_str/ct_string_view.hpp>`,
`fmt::format(...)` works with the same syntax. The fmt support is
auto-detected via `__has_include` and gated on the
`CJM_CT_STRING_VIEW_HAS_FMT` macro defined by the header.

---

## The two flavors of `basic_ct_string_view`

`basic_ct_string_view` is templated on a `bool VALID_CSTR` (exposed as the
static `known_cstr` member), giving two distinct types:

| Property | `known_cstr == true` (`ct_cstring_view`, …) | `known_cstr == false` (`ct_string_view`, …) |
|---|---|---|
| `c_str()` | available, returns a guaranteed-null-terminated pointer | not available (deleted by `requires`) |
| `data()`  | available, == `c_str()` | available, no null-termination guarantee |
| Default ctor | empty view pointing at static `'\0'` (`c_str()[0] == '\0'`) | empty view, `data()` unspecified |
| `remove_prefix(n)` | OK (does not move the end pointer) | OK |
| `remove_suffix(n)` | **disabled** (would invalidate null-termination) | OK |
| `substr(pos)` (trailing) | returns same flavor (preserves guarantee) | returns same flavor |
| `substr(pos, count)` (bounded) | returns the **non-`known_cstr`** flavor | returns the non-`known_cstr` flavor |
| Implicit converting ctor from the other flavor | from non-`known_cstr` → `known_cstr` is **not provided** | from `known_cstr` → non-`known_cstr` **is** provided (downgrade is always safe) |

This design lets the type system reflect the actual invariant: a view is
either statically guaranteed to be a C-string (and you can call `c_str()`)
or it is not (and you cannot, but you can still do anything else a
`string_view` does, including the operations that would invalidate the
guarantee).
--
## Defining Static-Member or Global Constants

### Usage Alternative 1: define as inline in header file as you would with a constexpr std::string_view

These may defined inline in header files as you would do with std::string_view global constants.

Pros: 
    1. familiar
    2. can use "auto"
    3. can static assert on their value from anywhere that includes them
    4. it is easy to see what their value is both by looking at header, and often in ide hints as well
Cons:
    1. there is greater instantiation cost for defining a ct_cstring_view than there is for defining a std::string_view

If the number of instantiations is large in a widely included header and noticeable delay is added, 
consider alternative 2.

Example code:
```c++
#ifndef CSTR_VIEW_HEADER_ONLY_VIEWS_HPP
#define CSTR_VIEW_HEADER_ONLY_VIEWS_HPP

#include "ct_str/ct_string_view.hpp"
#include <string_view>


namespace cps::ct_string::example_ho
{
    using namespace std::literals;
    using namespace literals;

    template<typename TCStrHaver>
    concept has_c_str = requires (const TCStrHaver& x)
    {
        { x.c_str() } noexcept;
    };


    inline constexpr auto ascii_whitespace =  " \t\n\v\f\r"_ctsv;


    constexpr ct_string_view trim(ct_string_view sv,
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
// global definitions start here.....
    inline constexpr auto g_k_padded_evangeline = // type: ct_cstring_view (null-terminated)
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


    // you can static assert on their value globally if this is done in a header
    static_assert(g_k_padded_evangeline.known_cstr);
    static_assert(char{} == g_k_padded_evangeline.c_str()[g_k_padded_evangeline.size()]);
    static_assert(has_c_str<decltype(g_k_padded_evangeline)>);
    
    // note this is NOT a cstring because trimmed; NOT null-terminated
    static constexpr auto g_k_evangeline = trim(g_k_padded_evangeline); //type: ct_string_view (no 'c' before 's') 
    static_assert(!has_c_str<decltype(g_k_evangeline)>);
    inline constexpr auto ev_front = g_k_evangeline.front();
    static_assert(g_k_evangeline.back() == '.');

    // inline assures linker will not complain about multiple definitions of these variables,
    // since they are defined in a header file.  If a pointer or reference to them is taken,
    // they will be consistent within a given process (unlike "static" keyword where the value will
    // have a different address in every TU).
    inline constexpr auto g_k_pre = "pre"_ctsv; //ct_cstring_view
    inline constexpr auto g_k_post = "post"_ctsv; //ct_cstring_view
    inline constexpr auto g_k_forest = "forest"_ctsv; //ct_cstring_view
    inline constexpr auto g_k_voices = "voices"_ctsv; //ct_cstring_view

}
#endif //CSTR_VIEW_HEADER_ONLY_VIEWS_HPP
```
### Usage Alternative 2: declare in header, define in .cpp file with constinit.

If you decide that a large, widely-included header file is slowing down compilation noticeably, you can declare the variables in the header but define them with constinit in the translation unit.  Note that constinit will only be applied in the definition, not in the header. Also, constinit does *NOT* imply const.  We want these to be global constants (presumably) so ensure they are marked const both at declaration and definition.  We are using constinit here **not** to have mutable views, but solely to segregate header declaration from translation unit definition. 

In a project with a large number of protobufs included in headers, I suspect the difference in compilation time will be noise.  If a project is adopting effective techniques to minimize compile time, e.g. by heavy usage of PImpl, this may be the best option. Otherwise, just defining them in the header file should be the shortest path to success.

Header file:
```c++
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
```

Translation Unit:
```c++
#include "impl_views.hpp"

namespace cps::ct_string::example_impl
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

  // still compiles if skipping the constexpr variable, but the IDE hates it and gives errors, even though it compiles.
  // so put in constexpr variable first, before assigning to the variables defined in header.

  static constexpr auto g_k_prv_pd_ev = R"(     THIS is the forest primeval. The murmuring pines and the hemlocks,
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

  constinit const ct_cstring_view g_k_padded_evangeline = g_k_prv_pd_ev;

  constinit const ct_string_view g_k_evangeline = trim(g_k_prv_pd_ev);

  constinit const ct_cstring_view g_k_pre = "pre"_ctsv;
  constinit const ct_cstring_view g_k_post = "post"_ctsv;
  constinit const ct_cstring_view g_k_forest = "forest"_ctsv;
  constinit const ct_cstring_view g_k_voices = "voices"_ctsv;

  static_assert(g_k_padded_evangeline.known_cstr);
  static_assert(!g_k_evangeline.known_cstr);
  static_assert(has_c_str<decltype(g_k_padded_evangeline)>);

  constinit const ct_cstring_view demo::s_k_author_name = "Christopher P. Susie"_ctsv;

}
```

---

## Comparison cheat-sheet

```text
                                 std::string_view   basic_ct_string_view
                                                    (VALID_CSTR == true)
─────────────────────────────────────────────────────────────────────────
non-owning, contiguous, cheap        ✓                    ✓
implicit conversion to std SV        —                    ✓ (noexcept)
c_str() / null-termination promise   ✗                    ✓
can dangle                           ✓                    ✗
constructible from runtime ptr/string ✓                   ✗  (by design)
usable as map/unordered_map key      ✓                    ✓ (with provided aliases)
NTTP-friendly storage type           —                    basic_fixed_string

                                 basic_fixed_string
─────────────────────────────────────────────────────────────────────────
fixed-capacity, owning buffer        ✓
NTTP-eligible (structural type)      ✓
constexpr concatenation (+)          ✓
implicit conversion to std SV        ✓
implicit conversion to std string    explicit (allocates)
```

---

## Building & integrating

The library is header-only.

### CMake (FetchContent)

```cmake
include(FetchContent)
FetchContent_Declare(
    cstr_view
    GIT_REPOSITORY https://github.com/<your-org>/cstr_view.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(cstr_view)

target_link_libraries(my_target PRIVATE cstr_view)
```

### CMake (subdirectory)

```cmake
add_subdirectory(third_party/cstr_view)
target_link_libraries(my_target PRIVATE cstr_view)
```

### Manual

Add `inc/` to your include path and `#include <ct_str/ct_string_view.hpp>`
(which includes `<ct_str/fixed_string.hpp>`).

### Optional: `{fmt}` support

If `<fmt/format.h>` is reachable from your include path *and* it is
included before `<ct_str/ct_string_view.hpp>` (or the header detects it via
`__has_include`), a `fmt::formatter` specialization for
`basic_ct_string_view` is enabled automatically. No extra macro is
required from the user.

### Running the tests

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

GoogleTest is used and is located via `find_package(GTest CONFIG REQUIRED)`.

---

## Requirements

- A C++20-conforming compiler with full support for:
  - class types as non-type template parameters,
  - concepts,
  - `consteval`,
  - three-way comparison.
- Tested with MSVC (Visual Studio 2022 / cl 19.4x Windows 11, x86_64) and G++ (11.5 ubuntu x86_64, Linux).
- Tested with and without `fmtlib`.
- The optional `std::formatter` support requires `<format>`
  (`__cpp_lib_format >= 201907L`).
- The optional `contains` member uses the standard
  `string_view::contains` (`__cpp_lib_string_contains >= 202011L`) when
  available and falls back to a `find`-based implementation otherwise.

---

## License

See [`LICENSE`](LICENSE)

