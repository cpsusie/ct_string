# cstr_view

A small, header-only C++20 library providing two complementary types built on
`std::basic_string_view`:

| Type | Header | What it is |
|---|---|---|
| `cps::ct_string::basic_fixed_string<TChar, N>` | `<ct_str/fixed_string.hpp>` | A fixed-capacity, null-terminated character buffer that is usable as a non-type template parameter (NTTP). |
| `cps::ct_string::basic_ct_string_view<TChar, VALID_CSTR>` | `<ct_str/ct_string_view.hpp>` | A non-owning view whose pointee is *statically guaranteed* to be a compile-time constant with static storage duration — and, when `VALID_CSTR == true`, guaranteed to be properly null-terminated. In that flavor, safe to pass to C APIs. Cannot dangle. |

If you have ever wanted to write something like

```c++
constexpr auto sv = "hello"_ctsv;            // a string_view-like type
const char*  cs   = sv.c_str();               // ...that ALSO has c_str()
                                              //    and CANNOT dangle.
```

…this library is for you.

---

## Table of contents

- [The core idea: compile-time provenance, expressed in the type system](#the-core-idea-compile-time-provenance-expressed-in-the-type-system)
- [Why not just `std::string_view`?](#why-not-just-stdstring_view)
- [Feature overview](#feature-overview)
- [The two types at a glance](#the-two-types-at-a-glance)
- [Quick start](#quick-start)
- [Use case 1: `c_str()` without copying or allocating](#use-case-1-c_str-without-copying-or-allocating)
- [Use case 2: views that cannot dangle](#use-case-2-views-that-cannot-dangle)
- [Use case 3: a string as a non-type template parameter](#use-case-3-a-string-as-a-non-type-template-parameter)
- [Use case 4: associative containers & case-insensitive lookup](#use-case-4-associative-containers--case-insensitive-lookup)
- [Use case 5: formatting](#use-case-5-formatting)
- [Use case 6: lazy ASCII case-folding views](#use-case-6-lazy-ascii-case-folding-views)
- [Use case 7: opt-in stream insertion for formattable types](#use-case-7-opt-in-stream-insertion-for-formattable-types)
- [The two flavors of `basic_ct_string_view`](#the-two-flavors-of-basic_ct_string_view)
- [Defining static-member or global constants](#defining-static-member-or-global-constants)
- [Comparison cheat-sheet](#comparison-cheat-sheet)
- [Building & integrating](#building--integrating)
- [Requirements](#requirements)
- [License](#license)

---

## The core idea: compile-time provenance, expressed in the type system

The value proposition of this library is a single, unusual guarantee:

> **If you hold a `basic_ct_string_view`, the characters it refers to are a
> compile-time constant with static storage duration.** Optionally (the
> `VALID_CSTR == true` flavor), the characters are also guaranteed to be
> properly null-terminated.

The natural objection is: *"That does nothing new. I can already point a
`std::string_view` at a string literal, or store a `std::string`."* You can —
but neither option lets you **state the invariant in the type system**, and
that difference is the whole point:

- **`std::string_view` is a promise made by convention.** Nothing stops a
  colleague (or you, six months later) from binding one to a temporary
  `std::string`, a stack buffer, or a slice with no null terminator. The
  invariant "this field only ever refers to a compile-time constant" lives in
  a comment, and comments do not fail to compile when violated.
- **`std::string` is a promise paid for at runtime.** It enforces ownership
  and null-termination, but by copying and (usually) allocating — a pointless
  cost for data that is immutable and fully known at compile time.
- **`basic_ct_string_view` is a promise enforced by the compiler.** Apart from
  default construction, its only public constructors are the
  copy/move/cross-flavor constructors and a
  `consteval` factory path that consumes a `basic_fixed_string` NTTP — a
  static-storage-duration object by definition. There is *no way* to forge
  one from runtime data, so every copy, everywhere, carries the guarantee.

Concretely: if a type has a field that is *designed* to only ever hold a
compile-time constant string — an error-code message, a command name, a
registered key, a log category — you can now say so via the type system:

```c++
struct error_info {
    int code;
    // By construction: a compile-time constant, null-terminated,
    // static-storage-duration string. Cannot dangle. No runtime checks,
    // no documentation discipline, no code-review vigilance required.
    cps::ct_string::ct_cstring_view message;
};

constexpr error_info make_not_found() {
    using namespace cps::ct_string::literals;
    return {404, "not found"_ctsv};        // OK
}

// error_info oops{500, std::string{"boom"}};   // does not compile: no
//                                              // constructor from runtime data
```

Compare the alternatives for that `message` field:

| Field type | Dangling possible? | Null-termination guaranteed? | Copies / allocates? | Invariant enforced by |
|---|---|---|---|---|
| `const char*` | yes | no (and no size either) | no | convention |
| `std::string_view` | yes | no | no | convention |
| `std::string` | no | yes | **yes** | runtime |
| `ct_cstring_view` | **no** | **yes** | **no** | **compiler** |

The guarantee is also *transitive and free*: the view is two words wide and
trivially copyable, so you can return it, store it in containers, pass it
across threads, and hand `c_str()` to any C API — all without a single
runtime check, copy, or allocation, and without ever being able to violate
the invariant by accident.

---

## Why not just `std::string_view`?

`std::string_view` is excellent for read-only string parameters but has two
properties that bite in production code:

1. **It does not promise null-termination.** A `std::string_view` may be a
   slice of a larger string. If you need a `const char*` to pass to a C API
   (POSIX, Win32, OpenSSL, SQLite, libcurl, `fopen`, …), `data()` supplies a
   pointer, but the type does not guarantee null-termination. Unless that is
   known independently, first copy the characters into a `std::string`.

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

## Feature overview

Everything the library ships, header by header, with the value each piece
adds:

| Header | What it provides | Why you would want it |
|---|---|---|
| `<ct_str/fixed_string.hpp>` | `basic_fixed_string<TChar, N>`: a structural, NTTP-eligible, null-terminated character buffer with `consteval` concatenation (`operator+`), the `_fs` literal, and the full read-only string interface. | Strings as template arguments; compile-time string building and validation via `static_assert`. See [use case 3](#use-case-3-a-string-as-a-non-type-template-parameter). |
| `<ct_str/ct_string_view.hpp>` | `basic_ct_string_view<TChar, VALID_CSTR>` in both flavors, the `_ctsv` literal, `make_ctsv`, `basic_ct_sv_factory`, `std::formatter` / optional `fmt::formatter` specializations, `std::hash` support, and `std::ranges::enable_borrowed_range` opt-in. | A `string_view` that cannot dangle, whose pointee is a compile-time constant, and (in the `known_cstr` flavor) offers a guaranteed-valid `c_str()`. See [use cases 1](#use-case-1-c_str-without-copying-or-allocating), [2](#use-case-2-views-that-cannot-dangle) and [5](#use-case-5-formatting). |
| `<ct_str/char_fold.hpp>` | `constexpr`, allocation-free ASCII case folding: `ascii_to_lower` / `ascii_to_upper` function objects and the lazy range adaptors `views::ascii_lower` / `views::ascii_upper`. | Case-insensitive machinery you can use standalone — as projections, in your own algorithms — with zero allocation. See [use case 6](#use-case-6-lazy-ascii-case-folding-views). |
| `<ct_str/ctsv_comparators.hpp>` | Transparent, `constexpr`-friendly comparators and hashers with `noexcept` conditional on argument conversions (`ctsv_less`, `ctsv_equal_to`, `ctsv_three_way`, `ctsv_hash`, plus `_ci_` case-insensitive variants) and the concepts that specify them. | Correct heterogeneous comparison across `basic_ct_string_view`, `std::basic_string_view`, `basic_fixed_string`, `std::basic_string` and literals; a `constexpr` hash `std::hash` cannot give you. See [use case 4](#use-case-4-associative-containers--case-insensitive-lookup). |
| `<ct_str/ctsv_containers.hpp>` | `map` / `set` / `unordered_map` / `unordered_set` wrappers keyed on `basic_ct_string_view`, with an enforced insert/lookup asymmetry and the non-throwing `at_if`. | Containers whose keys *provably* outlive them, yet can be looked up with any string-ish type — including `at` and `erase`, whose standard heterogeneous overloads require later language versions than C++20. See [use case 4](#use-case-4-associative-containers--case-insensitive-lookup). |
| `<ct_str/ctsv_format_registration.hpp>` | Opt-in variable templates that synthesize `operator<<` for any type that is `std::format`- or `fmt::format`-able but not stream-insertable. | Write one `formatter` specialization and get iostreams support for free — no boilerplate `operator<<`. See [use case 7](#use-case-7-opt-in-stream-insertion-for-formattable-types). |

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
static_assert(ctsv == fs);              // either order; also !=, <, <=>, ...
static_assert(ctsv == "hello" && fs == "hello");
```

Each of the two types compares with the other, in either flavor and at any
size, and with:

- string literals, and arrays of or pointers to its own character type,
  `const` or not. These are read up to their first null character;
- `std::basic_string` and `std::basic_string_view` of the same character type;
- anything else that converts to `std::basic_string_view` of that character
  type without throwing when it is read through a `const` reference, as every
  comparison reads it.

The other operand may be on either side, and `==`, `!=`, `<`, `<=`, `>`, `>=`
and `<=>` all work. Every one of these comparisons is `noexcept`, whichever
standard library you use. Strings of another character type, and `nullptr`, do
not compile. As with `std::basic_string_view`, a pointer must not be null, and
an array or the character sequence a pointer addresses must be null-terminated.

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

The `ct_cstring_view` and `ct_string_view` types document the fact that they cannot dangle; the way they are constructed guarantees it.  Only compile-time constant strings with static storage duration can be bound to them.

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

Apart from its default constructor, `basic_ct_string_view`'s public constructors are:

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

**Why is this important?** `std::string_view` is often used for three purposes:

1. Storing string literals as a superior alternative to `const char*`
   * without losing size information and
   * gaining access to the (read-only) standard-library string interface
2. As function arguments where the actual type to which the string-view refers is unimportant
3. Providing O(1) non-allocating substring operations (e.g. tokenizing)

With #1, the string literal referred to by the `const std::string_view` will never dangle.  For #2 and #3, the `std::string_view`s used are subject to dangling to the same extent that a `const std::string&` is subject to outliving the object to which it was bound.  The `std::string_view` type, however, provides no such guarantees.

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

We have a lookup above intended to store compile-time constant string literals as keys.  In some other context, however, someone decides to add a `std::string` to the map.  There will be no explicit cast required and no warning: `std::string` implicitly converts to `std::string_view`, by design.  Obviously, the result may not turn out well.

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
constexpr auto get_greeting() {
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

## Use case 4: associative containers & case-insensitive lookup

### The problem

`basic_ct_string_view` makes an excellent map key: two words wide, trivially
copyable, and its storage is statically guaranteed to outlive the container.
But it is deliberately *strict* -- you cannot build one from a
`std::string_view`, because that would forge the compile-time provenance and
null-termination guarantees that make it worth having.

That strictness bites at **lookup** time, and the standard library's
heterogeneous-lookup support is conspicuously incomplete:

| Operation | Heterogeneous in the standard? |
|---|---|
| `find`, `count`, `equal_range` | C++14 ordered / C++20 unordered |
| `lower_bound`, `upper_bound` | C++14 ordered; absent from unordered containers |
| `contains` | C++20 ordered and unordered |
| `erase(key)` | C++23 ordered and unordered, with operation-specific constraints |
| **`at(key)`** | **no in C++20/C++23 -- takes `const key_type&`** |
| `operator[]` | no in C++20/C++23; it can insert |

These heterogeneous operations require transparent comparators and, for unordered
containers, transparent hashers too. C++26 adds heterogeneous `at` and
`operator[]` overloads; this library supports C++20.

So in C++20/C++23 the single most-reached-for operation -- "look this string up
and give me the value" -- was the one that did not compile.

### The fix: wrapper containers with an enforced insert/lookup asymmetry

`<ct_str/ctsv_containers.hpp>` provides four class templates that encode the
distinction in the type system:

- **Inserting** (`operator[]`, `insert`, `emplace`, `try_emplace`, ...)
  requires a real `basic_ct_string_view`. Nothing else can produce a key, so
  the key's storage is guaranteed to outlive the container.
- **Looking up** (`at`, `at_if`, `find`, `contains`, `count`, `erase`,
  `lower_bound`, ...) takes the `std::basic_string_view` **by value**. Every
  `basic_ct_string_view` (either flavor), `std::basic_string`,
  `basic_fixed_string` and string literal converts implicitly. Directly supplied
  `nullptr` and literal `0` arguments are rejected at compile time.

```c++
#include <ct_str/ctsv_containers.hpp>
using namespace cps::ct_string;
using namespace cps::ct_string::literals;
ct_cstring_view_ci_map<int> ranks;     // ordered, ASCII case-INsensitive
ranks["Ace"_ctsv]  = 14;               // insert: needs a real ct view
ranks["King"_ctsv] = 13;
ranks.at("ACE");                       // 14 -- from a string literal
ranks.at(std::string{"ace"});          // 14 -- from a std::string
ranks.at(std::string_view{"aCe"});     // 14 -- from a string_view
ranks.contains("KING");                // true
ranks.erase("king");                   // 1
if (const int* p = ranks.at_if("queen"))   // non-throwing lookup
{
    // not reached
}
// ranks[std::string_view{"Ace"}] = 1;   // ILL-FORMED, by design.
```

`at_if` is the non-throwing sibling of `at`: it returns a pointer to the
mapped value or `nullptr`. It is what you usually want -- no exception, and
no `find`/`end()` dance.

### Available containers

Generic templates (every `std_char` type is supported):

```c++
basic_ctsv_map<TChar, VALID_CSTR, TValue, TLess = ctsv_less<TChar>>
basic_ctsv_set<TChar, VALID_CSTR, TLess = ctsv_less<TChar>>
basic_ctsv_unordered_map<TChar, VALID_CSTR, TValue,
                         THash = ctsv_hash<TChar>, TEq = ctsv_equal_to<TChar>>
basic_ctsv_unordered_set<TChar, VALID_CSTR,
                         THash = ctsv_hash<TChar>, TEq = ctsv_equal_to<TChar>>
```

Convenience aliases are spelled out for `char` and `wchar_t` in both flavors,
case-sensitive and case-insensitive. Insert `_ci_` for the case-insensitive
form:

| Case-sensitive | Case-insensitive |
|---|---|
| `ct_cstring_view_set` / `ct_string_view_set` / `ct_wcstring_view_set` / `ct_wstring_view_set` | `ct_cstring_view_ci_set` / `ct_string_view_ci_set` / ... |
| `ct_cstring_view_map<V>` / `ct_string_view_map<V>` / ... | `ct_cstring_view_ci_map<V>` / ... |
| `ct_cstring_view_unordered_set` / ... | `ct_cstring_view_ci_unordered_set` / ... |
| `ct_cstring_view_unordered_map<V>` / ... | `ct_cstring_view_ci_unordered_map<V>` / ... |

`multimap` / `multiset` wrappers are not provided.

### Comparators and hashers

`<ct_str/ctsv_comparators.hpp>` supplies the function objects, each in a
case-sensitive and an ASCII-case-insensitive flavor, templated on the
character type:

| Case-sensitive | Case-insensitive | Result |
|---|---|---|
| `ctsv_less<TChar>` | `ctsv_ci_less<TChar>` | `bool` |
| `ctsv_equal_to<TChar>` | `ctsv_ci_equal_to<TChar>` | `bool` |
| `ctsv_three_way<TChar>` | `ctsv_ci_three_way<TChar>` | `strong_ordering` / **`weak_ordering`** |
| `ctsv_hash<TChar>` | `ctsv_ci_hash<TChar>` | `std::size_t`, **`constexpr`** |

All of them are usable standalone -- as sort predicates, projections, or in
your own containers:

```c++
std::vector<std::string_view> v{"delta", "Alpha", "charlie", "Bravo"};
std::ranges::sort(v, ctsv_ci_less<char>{});     // Alpha, Bravo, charlie, delta
static_assert(ctsv_ci_hash<char>{}("Ace") == ctsv_ci_hash<char>{}("ACE"));
```

Four things are worth knowing:

1. **The fold direction is observable.** These fold *down*. `'_'` is `0x5F`,
   between `'Z'` (`0x5A`) and `'a'` (`0x61`), so folding down makes `"Z"`
   sort *after* `"_"`. Folding up would give the opposite order. Both are
   valid strict weak orderings; you just need to know which you have.
2. **Case-insensitive ordering is *weak*, not strong.** `"abc"` and `"ABC"`
   are equivalent without being equal, so `ctsv_ci_three_way` returns
   `std::weak_ordering` while `ctsv_three_way` returns `std::strong_ordering`.
3. **`ctsv_hash` is not `std::hash`.** `std::hash` is not `constexpr`, which
   rules out compile-time container construction and `static_assert`-based
   testing, so these are FNV-1a over code units. If you need the standard
   library's hash instead, pass `std::hash<basic_ct_string_view<TChar, B>>`
   explicitly -- it is transparent and pairs correctly with `std::equal_to<>`.
4. **`nullptr` is never a key.** Passing `nullptr` directly to any of these
   function objects, to `std::hash<basic_ct_string_view<TChar, B>>`, or to a container's
   lookup does not compile, whatever the standard library or language version.
   In C++20, `std::basic_string_view` itself still converts from `nullptr`, and
   the call would have undefined behaviour.

### ASCII only

Case folding touches only `'A'`..`'Z'` / `'a'`..`'z'`. Every other code unit,
including every code unit `>= 0x80`, passes through unchanged. This is safe
for UTF-8/16/32 (no code unit of a non-ASCII character can be mistaken for an
ASCII letter) but it does mean `"CAFÉ"` and `"café"` do **not** compare equal.
Full Unicode case folding needs the `CaseFolding.txt` tables and multi-code-
point expansion (`ß` folds to `ss`, changing the length), which is a project
of its own; see the `\todo` in `char_fold.hpp`.

### Concepts

The requirements on a comparator are spelled out as concepts rather than left
to documentation, so getting one wrong is a diagnostic rather than a silent
fallback to homogeneous lookup:

```c++
transparent_ctsv_less<F, TChar>        // ordered containers' Compare
transparent_ctsv_equal_to<F, TChar>    // unordered containers' KeyEqual
transparent_ctsv_three_way<F, TChar>
transparent_ctsv_hash<F, TChar>        // unordered containers' Hash
consistent_ctsv_hash_equal<THash, TEq, TChar>
```

Each requires nothrow default-constructibility, a nested `is_transparent`,
and correct, `noexcept` behaviour over *every* pairing of
`{cstr-flavor view, non-cstr-flavor view, std::basic_string_view}` in both
argument orders. `std::less<>` and `std::equal_to<>` satisfy them;
`std::less<std::string_view>` does not (it is not transparent).

`consistent_ctsv_hash_equal` deserves special mention. The unordered-container
invariant is that equal keys hash equally, and pairing a case-insensitive
equality with a case-sensitive hash violates it *silently* -- the container
would hold both `"Ace"` and `"ACE"` while reporting them equal. The wrappers'
`requires`-clause rejects that mixture at compile time. Participation is
declared via the `ctsv_folds_case` trait, which defaults to `false`, so types
that know nothing about this library (`std::hash`, `std::equal_to<>`) pair
correctly with one another.

### Migrating from the old aliases

Earlier versions exposed plain alias templates
(`basic_ct_string_view_map`, `..._unordered_set`, and the per-flavor
spellings) in `ct_string_view.hpp`. Those are gone. The concrete alias
*names* are unchanged -- `ct_cstring_view_map<V>` still works -- but they now
name the wrapper class templates, so you must include
`<ct_str/ctsv_containers.hpp>` rather than relying on `ct_string_view.hpp`,
which no longer pulls in `<map>`/`<set>`/`<unordered_map>`/`<unordered_set>`.
The generic `basic_ct_string_view_*` alias templates are replaced by
`basic_ctsv_*`; `multimap`/`multiset` forms are not carried over.

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

If `<fmt/format.h>` is on your include path, `fmt::format(...)` works with the
same syntax. You do not need to include fmt yourself, and include order does
not matter: `<ct_str/ct_string_view.hpp>` detects fmt with `__has_include`
and includes it on its own. When it does, it defines
`CPS_CT_STRING_VIEW_HAS_FMT`; when it also finds `<fmt/xchar.h>`, needed for
`wchar_t`, it includes that too and defines `CPS_CT_STRING_VIEW_HAS_WFMT`.

---

## Use case 6: lazy ASCII case-folding views

`<ct_str/char_fold.hpp>` provides the lowest layer of the case-insensitive
machinery, and it is useful entirely on its own:

- `ascii_to_lower<TChar>` / `ascii_to_upper<TChar>` -- `constexpr`,
  `noexcept`, allocation-free function objects that fold a single code unit.
  They make ideal projections for `std::ranges` algorithms.
- `views::ascii_lower` / `views::ascii_upper` -- lazy range adaptors that
  apply the fold across any character range. The result is a
  `std::ranges::transform_view`: nothing is copied and no storage is
  acquired.

```c++
#include <ct_str/char_fold.hpp>

using namespace cps::ct_string;
using namespace cps::ct_string::literals;

constexpr auto shout = "hello"_ctsv;
for (const char c : views::ascii_upper(shout)) {
    // 'H', 'E', 'L', 'L', 'O' -- lazily, with no allocation
}

// Case-insensitive equality without allocating or copying:
bool same = std::ranges::equal(views::ascii_lower("Hello"_ctsv),
                               views::ascii_lower("hELLO"_ctsv));   // true
```

Every flavor of `basic_ct_string_view` is guaranteed not to dangle by its
construction and storage invariants. Folding views built from it use the same
static character storage. Those built from `std::basic_string_view` still require
the underlying characters to remain valid. Participation in `borrowed_range`
does not extend character-storage lifetime; `transform_view` iterators also
require the transform-view object to remain alive while they are used.

Folding is ASCII-only by design (see [ASCII only](#ascii-only)); every non-ASCII
code unit passes through unchanged, which keeps the transformation safe for
UTF-8/16/32 data.

---

## Use case 7: opt-in stream insertion for formattable types

`<ct_str/ctsv_format_registration.hpp>` solves a general annoyance: you have
written a `std::formatter` (or `fmt::formatter`) specialization for your
type, and now you *also* want `operator<<` for iostreams -- more boilerplate
that just forwards to the formatter.

Register the type once by specializing a variable template to enable a
constrained `operator<<`. For a type defined outside `cps::ct_string`,
registration alone does not make ADL find the operator; bring it into scope
where the insertion is written:

```c++
#include <ct_str/ctsv_format_registration.hpp>
#include <iostream>

// my_type already has a std::formatter specialization ...
namespace cps::ct_string
{
    template<>
    inline constexpr bool g_k_stream_insert_via_std_format<my_type> = true;
}

// Make the operator visible where the insertion is written:
using cps::ct_string::operator<<;
std::cout << my_type{/*...*/} << '\n';
```

Four independent registration points are provided, covering narrow and wide
streams for both formatting libraries: `g_k_stream_insert_via_std_format`,
`g_k_stream_insert_via_fmt_format`, `g_k_wide_stream_insert_via_std_format`
and `g_k_wide_stream_insert_via_fmt_format`.

Observe these rules independently for narrow and wide streams:

- do not register a type that is *already* insertable into that kind of stream:
  this may create an ambiguity or silently switch operators. The caller must
  observe this restriction; the registration concepts do not enforce it;
- for a given stream width, a type cannot be registered for both `std::format`
  and `fmt::format` at once; the concepts reject that combination.

A worked example lives in `fmt_reg_demo.hpp` / `.cpp` in `test_console_app`.

---

## The two flavors of `basic_ct_string_view`

`basic_ct_string_view` is templated on a `bool VALID_CSTR` (exposed as the
static `known_cstr` member), giving two distinct types:

| Property | `known_cstr == true` (`ct_cstring_view`, …) | `known_cstr == false` (`ct_string_view`, …) |
|---|---|---|
| `c_str()` | available, returns a guaranteed-null-terminated pointer | not available (deleted by `requires`) |
| `data()`  | available, == `c_str()` | available, no null-termination guarantee |
| Default ctor | empty view pointing at static `'\0'` (`c_str()[0] == '\0'`) | empty view, `data() == nullptr` |
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

---

## Defining static-member or global constants

### Usage alternative 1: define as inline in a header file, as you would with a constexpr `std::string_view`

These may be defined inline in header files as you would do with
`std::string_view` global constants.

Pros:

1. familiar
2. can use `auto`
3. can `static_assert` on their value from anywhere that includes them
4. it is easy to see what their value is both by looking at the header, and often in IDE hints as well

Cons:

1. there is greater instantiation cost for defining a `ct_cstring_view` than there is for defining a `std::string_view`

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
### Usage alternative 2: declare in header, define in .cpp file with `constinit`

If a large, widely included header noticeably slows compilation, declare the
variables in the header and define them in a translation unit. In this example,
`constinit` appears only on the definitions; it may also appear on declarations.
It enforces static initialization and implies neither `const` nor `constexpr`.
For global constants, write `const` on both declarations and definitions. The
separate definitions move instantiation work out of the header; `constinit`
ensures that initialization is not deferred to runtime.

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

The library is header-only. Including its headers manually requires C++20 and
no third-party library. The CMake project requires CMake 3.21 or later.
Unless the user overrides the [build options](#build-options), it builds the
C++20 checks, the C++23 tests and the demo when it is the top-level project;
included in another project with `FetchContent` or `add_subdirectory`, it
provides the library alone and needs neither fmt nor GoogleTest.

### CMake (FetchContent)

```cmake
include(FetchContent)
FetchContent_Declare(
    ct_string
    GIT_REPOSITORY https://github.com/cpsusie/ct_string.git
    GIT_TAG        develop-release_1
)
FetchContent_MakeAvailable(ct_string)

target_link_libraries(my_target PRIVATE ct_string::ct_string)
```

Until the first release, the CMake project described here exists only on the
`develop-release_1` branch. Once a release is tagged, set `GIT_TAG` to that
tag.

### CMake (subdirectory)

```cmake
add_subdirectory(third_party/ct_string)
target_link_libraries(my_target PRIVATE ct_string::ct_string)
```

The target was formerly named `cstr_view`. That name still works: it is an
alias of `ct_string::ct_string`.

### Manual

Add `inc/` to your include path and `#include <ct_str/ct_string_view.hpp>`
(which includes `<ct_str/fixed_string.hpp>`).

### Optional: `{fmt}` support

If `<fmt/format.h>` is reachable from your include path,
`<ct_str/ct_string_view.hpp>` detects it via `__has_include`, includes it,
and enables a `fmt::formatter` specialization for `basic_ct_string_view`
automatically. Include order does not matter, and no extra macro is
required from the user.

### Build options

Each option defaults to `ON` when this is the top-level project, and `OFF`
when it is included in another. Either way, `-D` overrides it.

| Option | Builds | Requires |
|---|---|---|
| `CT_STRING_BUILD_CXX20_CHECKS` | `cstr_view_cxx20_smoke`, the C++20 checks ([below](#language-standard-library-vs-tests)) | C++20; uses fmt if CMake finds it |
| `CT_STRING_BUILD_TESTS` | `cstr_view_tests`, the test suite | C++23, GoogleTest and fmt |
| `CT_STRING_BUILD_EXAMPLES` | `test_console_app`, the demo | C++23 and fmt |

Which third-party libraries are looked up depends on the selected options.
To check a compiler that has no C++23, such as GCC 11, build the C++20 checks alone:

```bash
cmake -S . -B build -DCT_STRING_BUILD_TESTS=OFF -DCT_STRING_BUILD_EXAMPLES=OFF
cmake --build build
```

The C++20 checks then build without fmt if fmt is not installed. If fmt's
headers are on the compiler's include path, the library header finds and
includes them on its own (see above), whether CMake links fmt or not.

The project uses no C++ modules. It turns off CMake's module scanning (CMake
3.28 and later) for its own targets, so Clang does not need `clang-scan-deps`.

### CMake presets

`CMakePresets.json` defines configure, build and test presets with the same
names. Each uses Ninja and builds in `build/<preset>`:

| Presets | Compiler | Host |
|---|---|---|
| `gcc-debug`, `gcc-release` | `g++` | any but Windows |
| `clang-debug`, `clang-release` | `clang++` | any but Windows |
| `msvc-debug`, `msvc-release` | `cl` | Windows, x64 developer environment |
| `clang-cl-debug`, `clang-cl-release` | `clang-cl` | Windows, x64 developer environment |

```bash
cmake --preset gcc-debug
cmake --build --preset gcc-debug
ctest --preset gcc-debug
```

To use a particular compiler version, override the compiler when configuring:
`cmake --preset gcc-debug -D CMAKE_CXX_COMPILER=g++-14`. Run the MSVC and
clang-cl presets from an x64 Developer Command Prompt or Developer PowerShell;
Visual Studio sets that environment up itself. A test preset reports an error
if no tests are found. The presets need only CMake 3.21. Personal presets
belong in `CMakeUserPresets.json`, which Git ignores.

### Running the tests

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

When the tests are enabled, GoogleTest is located with
`find_package(GTest CONFIG REQUIRED)`, and fmt with
`find_package(fmt CONFIG REQUIRED)`.

### Language standard: library vs. tests

The **library is C++20**, and the `ct_string` interface target advertises that
floor with `target_compile_features(ct_string INTERFACE cxx_std_20)`. The
**tests and the demo console app are C++23** (the console app uses
`std::expected`); they request it per-target with
`target_compile_features(... PRIVATE cxx_std_23)` rather than by raising
`CMAKE_CXX_STANDARD` for the rest of the build.

To keep those two facts from drifting apart, the build includes a
`cstr_view_cxx20_smoke` static library (option
`CT_STRING_BUILD_CXX20_CHECKS`), **pinned** to C++20 via target properties.
It needs neither GoogleTest nor a C++23 compiler. One translation unit
([`tests/cxx20_header_smoke.cpp`](tests/cxx20_header_smoke.cpp)) includes
every public header and instantiates the templates. A second
([`tests/cxx20_comparison_checks.cpp`](tests/cxx20_comparison_checks.cpp)) is
made only of compile-time checks: what the two class templates compare with,
and what they must not. It also checks that no comparator, hasher or
container lookup directly takes `nullptr`. A C++23-only construct landing in
`inc/ct_str`, or a comparison rejected by the standard library used for that
build, therefore breaks that target immediately instead of only breaking
downstream users on older toolchains.

---

## Requirements

- A C++20-conforming compiler with full support for:
  - class types as non-type template parameters,
  - concepts,
  - `consteval`,
  - three-way comparison.
- Tested with MSVC (Visual Studio 2022 / cl 19.4x Windows 11, x86_64) and G++ (11.5 ubuntu x86_64, Linux).
- Verified on Linux/x86_64 with **GCC 13**, GCC 14 and Clang 19 (library at
  C++20, tests at C++23). GCC 13 matters as the low-water mark: its libstdc++
  predates `std::ranges::to` (P1206R7), so nothing in the library -- or in the
  tests -- may depend on it.
- Tested with and without `fmtlib`.
- CMake 3.21 or later, to use the CMake project. Including the headers manually
  needs no CMake.
- Besides libstdc++ and the MSVC STL, **libc++ 17 or later** works. This was
  verified locally on Linux/x86_64, with Clang 17 + libc++ 17 and with
  Clang 20 + libc++ 20, where the C++23 tests pass too. CI does not check
  libc++ yet.
- The optional `std::formatter` support requires `<format>`
  (`__cpp_lib_format >= 201907L`). libc++ defines that macro only from
  version 19, so with libc++ 17 and 18 the `std::formatter` support is left out.
- The optional `contains` member uses the standard
  `string_view::contains` (`__cpp_lib_string_contains >= 202011L`) when
  available and falls back to a `find`-based implementation otherwise.

---

## License

See [`LICENSE`](LICENSE)

