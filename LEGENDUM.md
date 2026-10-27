# cstr_view

[![CI](https://github.com/cpsusie/ct_string/actions/workflows/ci.yml/badge.svg?branch=develop-release_1)](https://github.com/cpsusie/ct_string/actions/workflows/ci.yml?query=branch%3Adevelop-release_1)

Bibliotheca parva C++20, quae solis capitibus constat, duo genera inter se coniuncta, ex `std::basic_string_view` ducta, praebet:

| Genus | Caput | Quid sit |
|---|---|---|
| `cps::ct_string::basic_fixed_string<TChar, N>` | `<ct_str/fixed_string.hpp>` | Area characterum certae capacitatis, nullo charactere terminata, quae pro argumento formae quod valorem, non genus, repraesentat (NTTP) adhiberi potest. |
| `cps::ct_string::basic_ct_string_view<TChar, VALID_CSTR>` | `<ct_str/ct_string_view.hpp>` | Prospectus non possidens, cuius characteres **compositionis tempore constantes atque per totum programma mansuri** esse ex ipso genere certo constat; si `VALID_CSTR == true`, nullo quoque charactere rite terminantur. Si haec terminatio constat, API C tradi tuto potest; pendere ex re iam deleta non potest. |

Si tale quid scribere voluisti,

```c++
constexpr auto sv = "hello"_ctsv;            // a string_view-like type
const char*  cs   = sv.c_str();               // ...that ALSO has c_str()
                                              //    and CANNOT dangle.
```

ad hoc bibliotheca facta est.

---

## Index

- [Ratio summa: origo tempore compositionis in genere expressa](#ratio-summa-origo-tempore-compositionis-in-genere-expressa)
- [Cur non `std::string_view`?](#cur-non-stdstring_view)
- [Quae praebeantur](#quae-praebeantur)
- [Duo genera breviter](#duo-genera-breviter)
- [Initium celeriter](#initium-celeriter)
- [Usus I: `c_str()` sine exemplari neque memoria adsignanda](#usus-i-c_str-sine-exemplari-neque-memoria-adsignanda)
- [Usus II: prospectus qui pendere non possunt](#usus-ii-prospectus-qui-pendere-non-possunt)
- [Usus III: chorda ut argumentum formae non generis](#usus-iii-chorda-ut-argumentum-formae-non-generis)
- [Usus IV: receptacula associativa et inquisitio casus neglegens](#usus-iv-receptacula-associativa-et-inquisitio-casus-neglegens)
- [Usus V: formatio](#usus-v-formatio)
- [Usus VI: prospectus pigri ad casum ASCII mutandum](#usus-vi-prospectus-pigri-ad-casum-ascii-mutandum)
- [Usus VII: insertio in rivos, si quis velit](#usus-vii-insertio-in-rivos-si-quis-velit)
- [Duae species `basic_ct_string_view`](#duae-species-basic_ct_string_view)
- [Constantes globales aut staticae membrorum definiendae](#constantes-globales-aut-staticae-membrorum-definiendae)
- [Comparatio brevissima](#comparatio-brevissima)
- [Aedificatio et coniunctio](#aedificatio-et-coniunctio)
- [Quae requirantur](#quae-requirantur)
- [Licentia](#licentia)

---

## Ratio summa: origo tempore compositionis in genere expressa

Una res praecipue hac bibliotheca praestatur:

> **Qui `basic_ct_string_view` tenet, scit characteres ad quos is spectat compositionis tempore constantes esse atque memoriam per totum programma manentem habere.** Si autem species `VALID_CSTR == true` adhibetur, constat eos etiam nullo charactere rite terminari.

Obici potest nihil novi effici: `std::string_view` ad litterale dirigi posse, `std::string` autem chordam possidere. Ita est; neutrum tamen **conditionem in ipso genere exprimit**. In eo tota causa posita est.

- **`std::string_view` fide consuetudinis nititur.** Nihil impedit quominus temporario `std::string`, areae in pila positae, aut parti nullo charactere non terminatae alligetur. Condicio igitur commentario committitur; commentarius, si violetur, compositionem non prohibet.
- **`std::string` conditionem tempore executionis emit.** Possessionem terminationemque praestat; sed exemplari facto et plerumque memoria adsignata. Quod in re immutabili atque ante executionem nota supervacuum est.
- **`basic_ct_string_view` conditionem compositione cogit.** Praeter constructionem vacuam, publice ex se ipso vel ex altera specie copiatur aut movetur; aliter per viam `consteval`, quae `basic_fixed_string` NTTP accipit, efficitur. Ex datis tempore executionis ortis confingi non potest; itaque quocumque transfertur, eadem fides manet.

Si igitur membrum cuiusdam generis non quamlibet chordam sed solam constantem compositionis tempore continere debet — nuntium erroris, nomen imperii, clavem descriptam, genus inscriptionis — id iam non commentario, sed genere dicitur:

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

Alternativa sic comparantur:

| Genus membri | Pendere potest? | Terminatio nullo charactere certa? | Copiat / memoriam adsignat? | Conditio quo modo cogitur |
|---|---|---|---|---|
| `const char*` | ita | non, neque magnitudo nota | non | consuetudine |
| `std::string_view` | ita | non | non | consuetudine |
| `std::string` | non | ita | **ita** | executione |
| `ct_cstring_view` | **non** | **ita** | **non** | **compositione** |

Haec fides etiam transit nec quidquam executionis tempore constat. Prospectus duorum verborum est et proprietatem `trivially copyable` habet; reddi, in receptaculis condi, inter fila transmitti, API C per `c_str()` tradi potest: nullo examine, nullo exemplari, nulla memoria adsignata; condicio tamen casu violari non potest.

---

## Cur non `std::string_view`?

`std::string_view` ad chordas tantum legendas egregium est; duo tamen vitia in usu graviora habet.

1. **Terminationem nullo charactere non promittit.** Pars maioris chordae esse potest. Si `const char*` API C — POSIX, Win32, OpenSSL, SQLite, libcurl, `fopen` — tradendus est, index quidem per `data()` sumi potest; terminatio ex solo genere non constat. Nisi ea aliunde certa est, chorda prius in `std::string` copianda est.

2. **Pendere potest.** Temporario `std::string` alligari atque eo deleto superesse potest. Quaedam talia instrumentum compositionis deprehendit; multa, non.

`basic_ct_string_view` utrumque tollit:

1. Species `VALID_CSTR == true`, quae etiam **`known_cstr`** dicitur, `c_str()` praebet; inde index `const TChar*` ad seriem nullo charactere certo terminatam redditur. Nec exemplar, nec memoria, nec `strlen`.

2. Omnis `basic_ct_string_view` aut ex datis tempore compositionis notis (`basic_fixed_string` NTTP aut fabrica propria), aut ex alio `basic_ct_string_view`, efficitur. Characteres igitur semper per totum programma manent; prospectus pendere non potest.

`basic_fixed_string`, quo haec memoria continetur, alterum quoque usum habet: litterale pro argumento formae tradi potest.

---

## Quae praebeantur

Quid quodque caput afferat:

| Caput | Quid praebeat | Quorsum prosit |
|---|---|---|
| `<ct_str/fixed_string.hpp>` | `basic_fixed_string<TChar, N>`: area structuralis, NTTP apta, nullo charactere terminata; concatenatio `consteval` (`operator+`), litterale `_fs`, omnes operationes ad chordas legendas. | Chordae ut argumenta formarum; compositio et probatio tempore compositionis per `static_assert`. Vide [usum III](#usus-iii-chorda-ut-argumentum-formae-non-generis). |
| `<ct_str/ct_string_view.hpp>` | Utraque species `basic_ct_string_view<TChar, VALID_CSTR>`, `_ctsv`, `make_ctsv`, `basic_ct_sv_factory`, specialitates `std::formatter` et, si adsit, `fmt::formatter`, `std::hash`, atque `std::ranges::enable_borrowed_range`. | `string_view` quod pendere non potest; cuius origo compositionis tempore constat; cui, si `known_cstr`, `c_str()` certum est. |
| `<ct_str/char_fold.hpp>` | Conversio casus ASCII `constexpr`, sine memoria: `ascii_to_lower`, `ascii_to_upper`, atque prospectus pigri `views::ascii_lower`, `views::ascii_upper`. | Comparatio casus neglegens, projectiones, algoritmi; nulla memoria. |
| `<ct_str/ctsv_comparators.hpp>` | Comparatores et functiones hash transparentes ad usum `constexpr`; `noexcept` a conversionibus argumentorum pendet: `ctsv_less`, `ctsv_equal_to`, `ctsv_three_way`, `ctsv_hash`, cum speciebus `_ci_`; item conceptus quibus exiguntur. | Comparatio heterogenea inter `basic_ct_string_view`, `std::basic_string_view`, `basic_fixed_string`, `std::basic_string`, litteralia; hash `constexpr`, quod `std::hash` non praebet. |
| `<ct_str/ctsv_containers.hpp>` | Involucra `map`, `set`, `unordered_map`, `unordered_set` clavibus `basic_ct_string_view`; insertio stricte distinguitur ab inquisitione; additur `at_if` non iaciens. | Claves receptaculum certo superant, sed quavis re chordae simili quaeri possunt, etiam per `at` et `erase`. |
| `<ct_str/ctsv_format_registration.hpp>` | Formae optativae quae `operator<<` efficiunt iis generibus quae per `std::format` aut `fmt::format` formari possunt, sed rivo inseri nondum possunt. | Una specialitate `formatter` scripta, etiam iostreams habentur; alter `operator<<` scribendus non est. |

---

## Duo genera breviter

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

Utrumque genus cum altero eiusdem generis characterum confertur, cuiusvis speciei et magnitudinis, atque cum his:

- cum litteralibus, areis et indicibus eiusdem generis characterum, sive `const` sive non; ex his characteres usque ad primum characterem nullum leguntur;
- cum `std::basic_string` et `std::basic_string_view` eiusdem generis characterum;
- cum quavis alia re quae ex relatione `const T&` in `std::basic_string_view` eiusdem generis characterum sine exceptione convertitur; sic enim quaeque comparatio eam accipit.

Alterum argumentum a sinistra vel a dextra poni potest; `==`, `!=`, `<`, `<=`, `>`, `>=`, `<=>` omnia valent. Omnes hae comparationes `noexcept` sunt, quacumque bibliotheca standard utaris. Chordae alterius generis characterum, atque `nullptr`, compositionis tempore recusantur. Ut in `std::basic_string_view`, index nullus esse non debet; area autem, vel series characterum ad quam index spectat, charactere nullo terminata esse debet.

Nomina commodiora:

| Nomen | = |
|---|---|
| `ct_cstring_view`  | `basic_ct_string_view<char,    true>`  (`c_str()` habet) |
| `ct_string_view`   | `basic_ct_string_view<char,    false>` (`string_view` simile; `c_str()` non habet) |
| `ct_wcstring_view` | `basic_ct_string_view<wchar_t, true>`  |
| `ct_wstring_view`  | `basic_ct_string_view<wchar_t, false>` |
| `ct_u8cstring_view` / `ct_u8string_view`   | species `char8_t` |
| `ct_u16cstring_view` / `ct_u16string_view` | species `char16_t` |
| `ct_u32cstring_view` / `ct_u32string_view` | species `char32_t` |

---

## Initium celeriter

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

## Usus I: `c_str()` sine exemplari neque memoria adsignanda

Ubi nunc tale invenitur:

```c++
void log(std::string_view msg) {
    std::string copy{msg};               // allocate, just to get a c_str()
    ::syslog(LOG_INFO, "%s", copy.c_str());
}
```

si vocans chordam tempore compositionis praebere potest, hoc sufficit:

```c++
void log(cps::ct_string::ct_cstring_view msg) {
    ::syslog(LOG_INFO, "%s", msg.c_str()); // no allocation
}
```

`c_str()` soli speciei `known_cstr == true` datum est. Si terminatio certa non est, vocari nequit; quod commentario monendum erat, genere prohibetur.

---

## Usus II: prospectus qui pendere non possunt

`ct_cstring_view` et `ct_string_view` non tantum dicunt se pendere non posse; constructio ipsa hoc efficit. Solis chordis compositionis tempore constantibus, quarum memoria per totum programma manet, alligari possunt.

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

Praeter constructorem vacuum, constructores publici sunt:

- copia aut motus eiusdem vel alterius speciei;
- via `consteval` (`_ctsv`, `make_ctsv`, `basic_ct_sv_factory`) quae `basic_fixed_string` NTTP accipit, cuius memoria natura sua per totum programma manet.

Constructor publicus ex `const char*` executionis tempore, `std::string&`, aut `std::string_view` **nullus est**. Fontes igitur frequentissimi prospectuum pendentium tolluntur.

Genus etiam in `std::ranges::enable_borrowed_range` recipitur; quare etiam:

```c++
std::ranges::find("x"_ctsv, 'y')
```

`std::ranges::dangling` non reddit.

**Cur hoc pertinet?** `std::string_view` tribus praecipue modis adhibetur:

1. ut litteralia chordarum melius quam `const char*` condantur, magnitudine retenta atque operationibus bibliothecae ad legendum praebitis;
2. ut argumentum functionis, cum genus rei subiectae nihil intersit;
3. ut partes chordae O(1), sine memoria nova, efficiantur.

In primo usu litterale numquam deletur; periculum nullum. In secundo et tertio, prospectus tam facile rem superare potest quam `const std::string&`; sed genus ipsum nihil promittit.

Exempli causa:

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

Receptaculum ad litteralia constantia destinatum est; alibi tamen aliquis `std::string` addit. Nec conversio aperta requiritur nec monitum datur: `std::string` in `std::string_view` consulto implicite convertitur. Exitus facile malus.

[Godbolt Demo](https://godbolt.org/z/h8T3soWqW)

Si clavis contra `std::map<ct_cstring_view, int>` esset, ubi terminatio interest, aut `std::map<ct_string_view, int>`, ubi non interest, duo simul haberemus:

1. consilium ipsum genere declaratum;
2. idem compositionis tempore coactum: nullum examen executionis tempore; quod non congruit, non componitur.

---

## Usus III: chorda ut argumentum formae non generis

`basic_fixed_string` regulis generis structuralis satisfacit; itaque directe NTTP esse potest. Hic usus bibliothecae potentissimus est.

### Genus nomine compositionis tempore insignire

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

Litterale `"width"` ipsum argumentum formae est. Area per constructorem `consteval` et deduction guide implicite in `basic_fixed_string<char, 6>` convertitur.

### Operationes chordarum compositionis tempore probatae

Quoniam valor pars formae est, quidquid ex eo computatur eodem tempore probari potest:

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

### Concatenatio `consteval`

`operator+` duorum `basic_fixed_string` est `consteval`; novum `basic_fixed_string` efficit, cuius longitudo summa exacta utriusque est, charactere nullo excepto:

```c++
using namespace cps::ct_string::literals;
constexpr auto greeting = "hello"_fs + ", "_fs + "world"_fs; // "hello, world"
static_assert(greeting == "hello, world");
static_assert(greeting.valid_cstr());
```

Quod inde fit, rursus NTTP esse potest; ita programmatio generica super chordis iam compositionis tempore confectis agitur.

### Inter `basic_fixed_string` et `basic_ct_string_view`

```c++
template<basic_fixed_string Greeting>
constexpr auto get_greeting() {
    return cps::ct_string::make_ctsv<Greeting>();   // ct_cstring_view
}

constexpr auto g = get_greeting<"hi there">();
static_assert(g == "hi there");
const char* p = g.c_str();   // points into a static-storage NTTP buffer
```

`make_ctsv<...>()` atque `_ctsv` efficiunt `ct_cstring_view` cuius `data()` ad NTTP memoriam `basic_ct_sv_factory<...>::fstr_val` spectat. Haec per totum programma manet; prospectus igitur copiari, reddi, condi, API C tradi potest.

---

## Usus IV: receptacula associativa et inquisitio casus neglegens

### Difficultas

`basic_ct_string_view` clavis receptaculi aptissima est: duorum verborum, proprietatem `trivially copyable` habens, cuius memoria receptaculum certo superat. Sed consulto stricta est: ex `std::string_view` effici nequit; aliter origo compositionis tempore et terminatio nullo charactere falso fingerentur.

Haec severitas, quae insertioni prodest, inquisitioni obstare potest. Praeterea inquisitio heterogenea bibliothecae standard nondum integra est:

| Operatio | Heterogenea in bibliotheca standard? |
|---|---|
| `find`, `count`, `equal_range` | a C++14 in ordinatis; a C++20 in inordinatis |
| `lower_bound`, `upper_bound` | a C++14 in ordinatis; in inordinatis absunt |
| `contains` | a C++20 in utrisque |
| `erase(key)` | a C++23 in utrisque, condicionibus propriis |
| **`at(key)`** | **in C++20/C++23 non; `const key_type&` accipit** |
| `operator[]` | in C++20/C++23 non; inserere enim potest |

Hae operationes heterogeneae comparatores transparentes, in receptaculis inordinatis etiam functiones hash transparentes, requirunt. C++26 etiam `at` et `operator[]` heterogenea addit; haec bibliotheca C++20 sustinet.

Ita operatio frequentissima — clavem quaerere et valorem accipere — in C++20/C++23 ipsa non componebatur.

### Remedium: receptacula quorum insertio ab inquisitione genere distinguitur

`<ct_str/ctsv_containers.hpp>` quattuor formas praebet:

- **Insertio** (`operator[]`, `insert`, `emplace`, `try_emplace`, ...) verum `basic_ct_string_view` requirit. Alia res clavem efficere non potest; receptaculum igitur clavem breviorem se numquam possidet.
- **Inquisitio** (`at`, `at_if`, `find`, `contains`, `count`, `erase`, `lower_bound`, ...) `std::basic_string_view` **valore** accipit. Utraque species `basic_ct_string_view`, item `std::basic_string`, `basic_fixed_string` atque litteralia, implicite convertuntur. Si `nullptr` vel litterale `0` directe traditur, vocatio compositionis tempore recusatur.

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

`at_if`, frater `at` qui exceptionem non iacit, aut indicem ad valorem reddit aut `nullptr`; plerumque hoc desideratur, sine exceptione, sine saltatione `find`/`end()`.

### Receptacula praebita

Formae generales omnibus generibus `std_char` aptae:

```c++
basic_ctsv_map<TChar, VALID_CSTR, TValue, TLess = ctsv_less<TChar>>
basic_ctsv_set<TChar, VALID_CSTR, TLess = ctsv_less<TChar>>
basic_ctsv_unordered_map<TChar, VALID_CSTR, TValue,
                         THash = ctsv_hash<TChar>, TEq = ctsv_equal_to<TChar>>
basic_ctsv_unordered_set<TChar, VALID_CSTR,
                         THash = ctsv_hash<TChar>, TEq = ctsv_equal_to<TChar>>
```

Nomina commodiora pro `char` et `wchar_t`, utraque specie, casu servato aut neglecto, dantur. Formae casum neglegenti `_ci_` inseritur:

| Casus servatur | Casus neglegitur |
|---|---|
| `ct_cstring_view_set` / `ct_string_view_set` / `ct_wcstring_view_set` / `ct_wstring_view_set` | `ct_cstring_view_ci_set` / `ct_string_view_ci_set` / ... |
| `ct_cstring_view_map<V>` / `ct_string_view_map<V>` / ... | `ct_cstring_view_ci_map<V>` / ... |
| `ct_cstring_view_unordered_set` / ... | `ct_cstring_view_ci_unordered_set` / ... |
| `ct_cstring_view_unordered_map<V>` / ... | `ct_cstring_view_ci_unordered_map<V>` / ... |

Involucra `multimap` et `multiset` non praebentur.

### Comparatores et functiones hash

`<ct_str/ctsv_comparators.hpp>` haec functionum obiecta praebet, casu servato et ASCII casu neglecto:

| Casus servatur | Casus neglegitur | Exitus |
|---|---|---|
| `ctsv_less<TChar>` | `ctsv_ci_less<TChar>` | `bool` |
| `ctsv_equal_to<TChar>` | `ctsv_ci_equal_to<TChar>` | `bool` |
| `ctsv_three_way<TChar>` | `ctsv_ci_three_way<TChar>` | `strong_ordering` / **`weak_ordering`** |
| `ctsv_hash<TChar>` | `ctsv_ci_hash<TChar>` | `std::size_t`, **`constexpr`** |

Singula etiam separatim adhiberi possunt:

```c++
std::vector<std::string_view> v{"delta", "Alpha", "charlie", "Bravo"};
std::ranges::sort(v, ctsv_ci_less<char>{});     // Alpha, Bravo, charlie, delta
static_assert(ctsv_ci_hash<char>{}("Ace") == ctsv_ci_hash<char>{}("ACE"));
```

Quattuor scienda sunt:

1. **Ordo ex eo pendet utrum litterae ad minores an ad maiores convertantur.** Haec ad litteras minores vertunt. `'_'` (`0x5F`) inter `'Z'` (`0x5A`) et `'a'` (`0x61`) iacet; quare `"Z"` post `"_"` ordinatur. Si ad maiores verteretur, contra. Uterque ordo rectus; sed uter habeatur sciendum.
2. **Ordo casum neglegens debilis, non fortis est.** `"abc"` et `"ABC"` aequivalent, quamquam aequalia non sunt; `ctsv_ci_three_way` igitur `std::weak_ordering`, `ctsv_three_way` autem `std::strong_ordering` reddit.
3. **`ctsv_hash` non est `std::hash`.** `std::hash` `constexpr` non est; eo igitur adhibito, receptacula tempore compositionis construi aut per `static_assert` probari non possunt. Hic FNV-1a super unitates characterum adhibetur. Qui functionem hash bibliothecae standard vult, `std::hash<basic_ct_string_view<TChar, B>>` aperte tradat; haec specialitas `is_transparent` praebet atque cum `std::equal_to<>` recte coniungitur.
4. **`nullptr` numquam clavis est.** Si `nullptr` cuiquam horum obiectorum, obiecto generis `std::hash<basic_ct_string_view<TChar, B>>`, aut inquisitioni receptaculi directe traditur, vocatio compositionis tempore recusatur, quacumque bibliotheca standard aut editione linguae utaris. In C++20 enim `nullptr` adhuc in `std::basic_string_view` implicite converti potest; ea autem conversio effectum indefinitum haberet.

### Tantum ASCII

Conversio casus solas litteras `'A'..'Z'` et `'a'..'z'` tangit; cetera, etiam quidquid `>= 0x80`, intacta transeunt. UTF-8/16/32 igitur non corrumpuntur; sed `"CAFÉ"` et `"café"` aequalia non habentur.

Conversio plena Unicode tabulas `CaseFolding.txt` et interdum expansionem plurium punctorum requirit — `ß`, exempli causa, in `ss` abit atque longitudinem mutat. Id opus separatum est; vide `\todo` in `char_fold.hpp`.

### Conceptus

Conditiones comparatorum conceptibus exprimuntur, non commentariis:

```c++
transparent_ctsv_less<F, TChar>
transparent_ctsv_equal_to<F, TChar>
transparent_ctsv_three_way<F, TChar>
transparent_ctsv_hash<F, TChar>
consistent_ctsv_hash_equal<THash, TEq, TChar>
```

Singuli constructionem default sine exceptione, `is_transparent`, atque rectum `noexcept` usum in omni coniunctione `{cstr view, non-cstr view, std::basic_string_view}` utroque ordine exigunt. `std::less<>` et `std::equal_to<>` satisfaciunt; `std::less<std::string_view>` non, quia transparens non est.

`consistent_ctsv_hash_equal` praecipue curandum. In receptaculo inordinato claves aequales eundem hash habere debent. Aequalitatem casum neglegentem cum functione hash casum servante iungere conditionem tacite frangeret: `"Ace"` et `"ACE"` simul condi possent, quamquam aequalia referrentur. `requires` hoc compositionis tempore recusat. Participatio per `ctsv_folds_case` indicatur, quod initio `false` est; ita genera externa ut `std::hash` et `std::equal_to<>` inter se recte coniunguntur.

### Ex nominibus veteribus migrandum

Versiones anteriores formas quae nomina alternativa definiunt in `ct_string_view.hpp` praebebant. Hae remotae sunt. Nomina concreta — ut `ct_cstring_view_map<V>` — manent, sed nunc involucra propria significant; `<ct_str/ctsv_containers.hpp>` igitur includendum est. `ct_string_view.hpp` iam `<map>`, `<set>`, `<unordered_map>`, `<unordered_set>` non trahit.

Formae generales `basic_ct_string_view_*` in `basic_ctsv_*` mutatae sunt; `multimap` et `multiset` non servantur.

---

## Usus V: formatio

Bibliotheca specialitates `std::formatter` praebet, atque, si `<fmt/format.h>` invenitur, etiam `fmt::formatter`, pro speciebus `char` et `wchar_t` `basic_ct_string_view`. A formatore standard `string_view` derivantur; tota igitur eius grammatica valet:

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

Si `<fmt/format.h>` in via capitum est, `fmt::format(...)` eadem ratione operatur. Fmt separatim includi necesse non est: caput `<ct_str/ct_string_view.hpp>` illud per `__has_include` deprehendit suaque sponte includit. Quo facto, macro `CPS_CT_STRING_VIEW_HAS_FMT` definitur.

Caput `<fmt/xchar.h>` ad series characterum latorum (`wchar_t`) formatandas necessarium est. Quo invento atque incluso, macro `CPS_CT_STRING_VIEW_HAS_WFMT` definitur.

Quo ordine capita includantur, nihil refert; nec ullum macro manu definiendum est.

---

## Usus VI: prospectus pigri ad casum ASCII mutandum

`<ct_str/char_fold.hpp>` infimum huius apparatus stratum praebet, etiam separatim utile:

- `ascii_to_lower<TChar>` / `ascii_to_upper<TChar>`: functionum obiecta `constexpr`, `noexcept`, quae sine memoria adsignanda unam unitatem mutant;
- `views::ascii_lower` / `views::ascii_upper`: adaptores pigri super quamlibet seriem characterum. Exitus `std::ranges::transform_view`; nihil copiatur, nulla memoria nova sumitur.

```c++
#include <ct_str/char_fold.hpp>

using namespace cps::ct_string;
using namespace cps::ct_string::literals;

constexpr auto shout = "hello"_ctsv;
for (const char c : views::ascii_upper(shout)) {
    // 'H', 'E', 'L', 'L', 'O' -- lazily, with no allocation
}

bool same = std::ranges::equal(views::ascii_lower("Hello"_ctsv),
                               views::ascii_lower("hELLO"_ctsv));   // true
```

Omnes species `basic_ct_string_view` pendere non possunt: hoc ipsius generis condiciones praestant. Prospectus pigri ex eis facti eandem memoriam staticam characterum adhibent; ex `std::basic_string_view` facti memoriam characterum adhuc validam requirunt. Participatio in `borrowed_range` vitam memoriae characterum non prorogat; iteratores autem `transform_view` prospectu transformato adhuc vivo adhibendi sunt.

Conversio consulto ASCII tantum est; ceterae unitates intactae transeunt. UTF-8/16/32 igitur non corrumpuntur; vide [Tantum ASCII](#tantum-ascii).

---

## Usus VII: insertio in rivos, si quis velit

`<ct_str/ctsv_format_registration.hpp>` molestiam communem tollit. Si `std::formatter` vel `fmt::formatter` iam scriptus est, saepe idem genus etiam per `operator<<` mittendum est; aliter eadem ratio iterum scribitur.

Forma variabili semel specializata, `operator<<` condicionibus restrictus praebetur. Si genus extra `cps::ct_string` definitum est, sola adscriptio non efficit ut operator per ADL inveniatur; eius nomen in locum vocationis per declarationem `using` inferatur:

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

Quattuor puncta adscriptionis sunt: `g_k_stream_insert_via_std_format`, `g_k_stream_insert_via_fmt_format`, `g_k_wide_stream_insert_via_std_format`, `g_k_wide_stream_insert_via_fmt_format`. Rivi characterum `char` et `wchar_t` separatim tractantur.

In utroque rivorum genere haec separatim servanda sunt:

- genus iam rivo inseribile adscribi non debet, ne ambiguitas aut mutatio tacita fiat; haec cautio usori relinquitur;
- idem genus simul per `std::format` et `fmt::format` adscribi non potest; hoc conceptibus prohibetur.

Exemplum plenum in `fmt_reg_demo.hpp` / `.cpp`, intra `test_console_app`, invenitur.

---

## Duae species `basic_ct_string_view`

`basic_ct_string_view` parametro `bool VALID_CSTR`, qui ut membrum staticum `known_cstr` exponitur, in duas species dividitur:

| Proprietas | `known_cstr == true` (`ct_cstring_view`, …) | `known_cstr == false` (`ct_string_view`, …) |
|---|---|---|
| `c_str()` | adest; indicem ad seriem nullo charactere certo terminatam reddit | abest |
| `data()` | adest; idem ac `c_str()` | adest; terminatio incerta |
| Constructor default | prospectus vacuus ad staticum `'\0'` | prospectus vacuus; `data() == nullptr` |
| `remove_prefix(n)` | licet | licet |
| `remove_suffix(n)` | **prohibetur**, quia terminationem frangeret | licet |
| `substr(pos)` | eandem speciem reddit | eandem speciem reddit |
| `substr(pos, count)` | speciem **non-`known_cstr`** reddit | speciem non-`known_cstr` reddit |
| Conversio implicita ex altera specie | non-`known_cstr` → `known_cstr` **non datur** | `known_cstr` → non-`known_cstr` datur |

Ita genus ipsam conditionem sequitur: aut C-string esse certo constat atque `c_str()` licet; aut non constat atque `c_str()` vetatur, ceteris operationibus `string_view` manentibus.

---

## Constantes globales aut staticae membrorum definiendae

### Via I: `inline` in capite, sicut `constexpr std::string_view`

Sic familiariter definiri possunt.

Commoda:

1. ratio nota;
2. `auto` licet;
3. valor ubicumque caput includitur per `static_assert` probari potest;
4. valor in capite et saepe in IDE statim videtur.

Incommodum:

1. ad `ct_cstring_view` instantiandum plus temporis opus est quam ad `std::string_view`.

Si instantiationes in capite late incluso multae sunt atque mora manifesta fit, via altera consideranda.

Exemplum:

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

    inline constexpr auto g_k_padded_evangeline =
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

    static_assert(g_k_padded_evangeline.known_cstr);
    static_assert(char{} == g_k_padded_evangeline.c_str()[g_k_padded_evangeline.size()]);
    static_assert(has_c_str<decltype(g_k_padded_evangeline)>);
    
    static constexpr auto g_k_evangeline = trim(g_k_padded_evangeline);
    static_assert(!has_c_str<decltype(g_k_evangeline)>);
    inline constexpr auto ev_front = g_k_evangeline.front();
    static_assert(g_k_evangeline.back() == '.');

    inline constexpr auto g_k_pre = "pre"_ctsv;
    inline constexpr auto g_k_post = "post"_ctsv;
    inline constexpr auto g_k_forest = "forest"_ctsv;
    inline constexpr auto g_k_voices = "voices"_ctsv;
}
#endif
```

### Via II: declarare in capite, definire in `.cpp` per `constinit`

Si caput magnum et late inclusum compositionem manifeste tardat, variabiles in capite declarari, in unitate translationis per `constinit` definiri possunt.

`constinit` in hoc exemplo definitioni tantum adhibetur; etiam in declaratione poni potest. Initializationem staticam praestat, sed neque `const` neque `constexpr` significat. Si constantes globales volumus, `const` utroque loco ponendum. Declaratio in capite a definitione separatur; `constinit` efficit ne initializatio ad tempus executionis differatur.

In opere ubi multa protobuf capita iam includuntur, differentia fortasse exigua erit; ubi autem tempus compositionis diligenter minuitur, exempli causa PImpl late adhibito, haec via melior esse potest. Aliter prior brevior.

Caput:

```c++
#ifndef CSTR_VIEW_IMPL_VIEWS_HPP
#define CSTR_VIEW_IMPL_VIEWS_HPP
#include "ct_str/ct_string_view.hpp"
#include <string_view>

namespace cps::ct_string::example_impl
{
    using namespace std::literals;
    using namespace literals;

    extern const ct_cstring_view g_k_padded_evangeline;
    extern const ct_string_view g_k_evangeline;
    extern const ct_cstring_view g_k_pre;
    extern const ct_cstring_view g_k_post;
    extern const ct_cstring_view g_k_forest;
    extern const ct_cstring_view g_k_voices;

    struct demo
    {
        static const ct_cstring_view s_k_author_name;
    };
}
#endif
```

Unitas translationis:

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

## Comparatio brevissima

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

## Aedificatio et coniunctio

Bibliotheca solis capitibus constat. Ad capita manu includenda C++20 sufficit; nulla bibliotheca externa requiritur. Apparatui CMake versione 3.21 vel posteriore opus est. Nisi usor [optiones aedificationis](#optiones-aedificationis) mutavit, apparatus, cum opus per se aedificatur, probationes C++20, probationes C++23 et programma demonstrativum quoque aedificat; in aliud opus per `FetchContent` aut `add_subdirectory` inclusus, solam bibliothecam praebet, neque fmt neque GoogleTest requirit.

### CMake (`FetchContent`)

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

Ante primam editionem publicam apparatus CMake hic descriptus in solo ramo `develop-release_1` exstat. Editione nota (`tag`) insignita, eadem nota in `GIT_TAG` ponatur.

### CMake (`subdirectory`)

```cmake
add_subdirectory(third_party/ct_string)
target_link_libraries(my_target PRIVATE ct_string::ct_string)
```

Propositum aedificationis olim `cstr_view` appellabatur. Id nomen manet: idem propositum significat ac `ct_string::ct_string`.

### Manu

`inc/` ad viam capitum adde atque:

```c++
#include <ct_str/ct_string_view.hpp>
```

quod `<ct_str/fixed_string.hpp>` quoque includit.

### `{fmt}` si desideratur

Caput `<ct_str/ct_string_view.hpp>` capita fmt per `__has_include` deprehendit suaque sponte includit. Ad speciem `char` `basic_ct_string_view` per `fmt::formatter` formatandam `<fmt/format.h>` requiritur; ad speciem `wchar_t`, etiam `<fmt/xchar.h>`. Cetera in [Usu V](#usus-v-formatio).

### Optiones aedificationis

Nisi usor aliter constituit, quaeque optio `ON` est, cum opus per se aedificatur, `OFF`, cum in aliud includitur. Utroque modo per `-D` mutari potest.

| Optio | Quid aedificet | Quid requirat |
|---|---|---|
| `CT_STRING_BUILD_CXX20_CHECKS` | `cstr_view_cxx20_smoke`, probationes C++20 ([infra](#norma-linguae-bibliotheca-et-probationes)) | C++20; fmt adhibet, si CMake id invenit |
| `CT_STRING_BUILD_TESTS` | `cstr_view_tests`, probationes C++23 | C++23, GoogleTest et fmt |
| `CT_STRING_BUILD_EXAMPLES` | `test_console_app`, programma demonstrativum | C++23 et fmt |

Quae bibliothecae externae quaerantur, ex optionibus electis pendet. Ad instrumentum probandum cui C++23 deest, velut GCC 11, solae probationes C++20 aedificentur:

```bash
cmake -S . -B build -DCT_STRING_BUILD_TESTS=OFF -DCT_STRING_BUILD_EXAMPLES=OFF
cmake --build build
```

Tum probationes C++20, si fmt non adest, sine eo aedificantur. Capitibus autem fmt in via capitum positis, caput bibliothecae ea sua sponte invenit atque includit (vide supra), sive CMake fmt coniungit sive non.

Opus modulis C++ non utitur. Inquisitione igitur modulorum, quam CMake a versione 3.28 adhibet, in propositis aedificationis huius operis omissa, Clang `clang-scan-deps` non requirit.

### Praescripta CMake (`presets`)

`CMakePresets.json` praescripta ad opus parandum, aedificandum, probandum continet, quorum nomina inter se congruunt. Omnia, Ninja adhibito, directorio `build/<preset>` utuntur:

| Praescripta | Instrumentum | Systema |
|---|---|---|
| `gcc-debug`, `gcc-release` | `g++` | quodvis praeter Windows |
| `clang-debug`, `clang-release` | `clang++` | quodvis praeter Windows |
| `msvc-debug`, `msvc-release` | `cl` | Windows, ambitu x64 parato |
| `clang-cl-debug`, `clang-cl-release` | `clang-cl` | Windows, ambitu x64 parato |

```bash
cmake --preset gcc-debug
cmake --build --preset gcc-debug
ctest --preset gcc-debug
```

Ad certam instrumenti versionem adhibendam, dum opus paratur, `CMAKE_CXX_COMPILER` mutetur: `cmake --preset gcc-debug -D CMAKE_CXX_COMPILER=g++-14`. Praescripta MSVC et clang-cl ex x64 Developer Command Prompt aut Developer PowerShell adhibeantur; ambitum autem Visual Studio sua sponte parat. Praescripto probandi adhibito, si nullae probationes reperiuntur, error nuntiatur. Praescriptis CMake 3.21 sufficit. Praescripta privata in `CMakeUserPresets.json` ponantur, quem fasciculum Git neglegit.

### Probationes

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Probationibus electis, GoogleTest per `find_package(GTest CONFIG REQUIRED)` quaeritur, fmt per `find_package(fmt CONFIG REQUIRED)`; utraque bibliotheca requiritur.

### Norma linguae: bibliotheca et probationes

**Bibliotheca C++20 est.** Propositum aedificationis (`target`) `ct_string` hanc normam minimam declarat per:

```cmake
target_compile_features(ct_string INTERFACE cxx_std_20)
```

**Probationes et programma demonstrativum C++23 requirunt** — hoc `std::expected` utitur — atque haec norma singulis propositis aedificationis per:

```cmake
target_compile_features(... PRIVATE cxx_std_23)
```

petitur; `CMAKE_CXX_STANDARD` totius operis non augetur.

Ad hanc distinctionem servandam, apparatus aedificationis bibliothecam staticam `cstr_view_cxx20_smoke` complectitur (optio `CT_STRING_BUILD_CXX20_CHECKS`), cui norma C++20 expresse imponitur. Neque GoogleTest neque instrumentum C++23 requirit. Una eius unitas translationis, [`tests/cxx20_header_smoke.cpp`](tests/cxx20_header_smoke.cpp), omnia capita publica includit formasque instantiat; altera, [`tests/cxx20_comparison_checks.cpp`](tests/cxx20_comparison_checks.cpp), solas probationes tempore compositionis peragendas continet. Hae explorant quibuscum duo genera conferantur, quibuscum non; confirmant etiam nullum comparatorem, nullam functionem hash, nullam inquisitionem receptaculi `nullptr` directe accipere. Si constructio quae C++23 requirit in `inc/ct_str` ingreditur, aut si qua comparatio a bibliotheca standard in qua probatur recusatur, huius bibliothecae aedificatio statim deficit; non exspectatur donec usor instrumento vetustiore laboret.

---

## Quae requirantur

- Instrumentum C++20 plene sustinens:
  - genera classium ut NTTP,
  - concepts,
  - `consteval`,
  - comparationem trium viarum.
- Probatum cum MSVC (Visual Studio 2022 / cl 19.4x, Windows 11, x86_64) et G++ 11.5 (Ubuntu x86_64, Linux).
- In Linux/x86_64 verificatum cum **GCC 13**, GCC 14, Clang 19; bibliotheca C++20, probationes C++23.
- GCC 13 terminus inferior consulto est: eius libstdc++ `std::ranges::to` (P1206R7) nondum habet; quare neque bibliotheca neque probationes eo niti possunt.
- Cum `fmtlib` et sine eo probatum.
- CI quidquid in ramos `develop-release_1` et `main` impellitur (`push`), et quamque petitionem coniungendi (`pull request`) in eos factam, in Linux/x86_64 cum GCC 14 probat: probationes C++20, cum fmt et sine eo; probationes C++23 et programma demonstrativum, utroque modo, `Debug` et `Release`.
- CMake 3.21 vel posterius, si apparatu CMake uteris; ad capita manu includenda CMake non requiritur.
- Praeter libstdc++ et bibliothecam STL MSVC, etiam **libc++ 17** et versiones posteriores adhiberi possunt. Hoc in instrumento locali Linux/x86_64 verificatum est, cum Clang 17 + libc++ 17 et cum Clang 20 + libc++ 20; cum posteriore etiam probationes C++23 succedunt. CI libc++ nondum probat.
- `std::formatter`, si adhibetur, `<format>` requirit atque `__cpp_lib_format >= 201907L`. libc++ hanc macrodefinitionem a versione 19 demum definit; cum libc++ 17 et 18 igitur specialitas `std::formatter` huius bibliothecae omittitur.
- Membrum `contains`, si in bibliotheca standard praesto est, `string_view::contains` utitur (`__cpp_lib_string_contains >= 202011L`); aliter per `find` efficitur.

---

## Licentia

Vide [`LICENSE`](LICENSE).
