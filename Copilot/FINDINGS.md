# Findings: evidence gathered for Task 1

> Snapshot taken **2026-10-10** against commit `ab2a8e5` (branch `develop-release_1`).
> This file records facts and measurements only. The decisions drawn from them are in
> [DECISIONS.md](DECISIONS.md), and the plan that acts on them is in [PLAN.md](PLAN.md).
> Runner images and compilers change, so re-check any finding before relying on it months later.

## F-1 Repository baseline

- **Library.** Header-only, with six public headers in `inc/ct_str/`: `ct_string_view.hpp`,
  `fixed_string.hpp`, `char_fold.hpp`, `ctsv_comparators.hpp`, `ctsv_containers.hpp` and
  `ctsv_format_registration.hpp`. Namespace `cps::ct_string`. MIT licence.
- **Tests:**
  - a GoogleTest suite in `tests/`, with 156 tests that currently run (see F-4);
  - a C++20-pinned smoke translation unit, `tests/cxx20_header_smoke.cpp`;
  - a C++23 demo program, `test_console_app/`.
- **Build.** A single `CMakeLists.txt`:
  - `cmake_minimum_required(VERSION 4.0.2)` and `project(cstr_view VERSION 1.0.0)`.
  - INTERFACE target `cstr_view` (`cxx_std_20`).
  - Static library `cstr_view_cxx20_smoke`, pinned to C++20.
  - C++23 executables `cstr_view_tests` and `test_console_app`.
  - `find_package(fmt CONFIG REQUIRED)` and `find_package(GTest CONFIG REQUIRED)` run
    unconditionally. Every configure, including a consumer's `add_subdirectory` or
    `FetchContent`, therefore needs both fmt and GoogleTest installed.
  - No install or export rules, package config, version header, CMake presets, CI workflow,
    vcpkg or Conan files. No Git tags.
- **Docs.** `README.md` (English) and `LEGENDUM.md` (Latin) are parallel documents.
  - README's `FetchContent` example uses the placeholder URL
    `https://github.com/<your-org>/cstr_view.git`.
  - README's requirements list "VS 2022 / cl 19.4x" and "G++ 11.5". Neither is verified by CI.
- **Naming is not yet uniform:**

| Thing | Current name |
|---|---|
| GitHub repository | `ct_string` |
| CMake project and target | `cstr_view` |
| C++ namespace | `cps::ct_string` |
| Include directory | `ct_str/` |
| Feature macros | `CPS_CT_STRING_VIEW_HAS_FMT`, `CPS_CT_STRING_VIEW_HAS_WFMT` |
| Include guards | mixed `CPS_*` and `CSTR_VIEW_*` |

## F-2 C++20 library: earliest working compilers

**Method.**
- Compiled `tests/cxx20_header_smoke.cpp`, which includes every public header and exercises the
  API, with `-std=c++20 -Wall -Wextra -Werror -fsyntax-only -Iinc`.
- Platform: Ubuntu 24.04 with Ubuntu-packaged compilers. GCC 11.1–11.3 came from the official
  `gcc:11.x` Docker images.
- "With fmt" means the same compile with fmt 9.1 (Ubuntu `libfmt-dev`, `FMT_HEADER_ONLY`) on the
  include path, which switches on the library's fmt integration.

| Compiler | Standard library | Result | Root cause of failure |
|---|---|---|---|
| GCC 10.5.0 | libstdc++ 10 | ❌ | Heterogeneous lookup in unordered containers (`ctsv_containers.hpp:735`) is a C++20 library feature that first shipped in GCC 11. |
| GCC 11.1.0, 11.2.0, 11.3.0 (Docker) | libstdc++ 11 | ✅ | — |
| GCC 11.5.0, 12.4.0, 13.3.0, 14.2.0 | own libstdc++ | ✅ (also with fmt) | — |
| Clang 14.0.6, 15.0.7 | libstdc++ 14 | ❌ | Dependent type names without `typename`. P0634 ("Down with `typename`!") first appeared in Clang 16. Examples: `fixed_string.hpp:163`, `ct_string_view.hpp:134`, `:642`, `:655`, `:734-736`. libstdc++ 14's ranges are also beyond these compilers. |
| Clang 16.0.6 | libstdc++ 14 | ❌ | One compiler bug: the constrained `friend` declaration of `basic_ct_sv_factory` (`ct_string_view.hpp:119-121`) is rejected with "requires clause differs in template redeclaration". This cascades into access errors at `:630`. |
| Clang 17.0.6, 18.1.3, 19.1.1, 20.1.2 | libstdc++ 14 | ✅ (also with fmt) | — |
| Clang 17, 18, 19, 20 | matching libc++ | ❌ | See F-5. |

Conclusions:

- **GCC floor = 11.1**, which is earlier than the 11.3 the brief asks for. GCC 10 cannot be supported
  without replacing a C++20 library feature.
- **Clang floor (with libstdc++) = 17.**
  - Clang 16 would need a workaround for a compiler bug.
  - Clang 15 and earlier would need `typename` added back throughout, which is contrary to the
    requested modern style.
- **libc++ is not supported at all today** (F-5).
- These are compile-only checks of the smoke translation unit. CI (task T6) makes them continuous
  and adds an install-and-consume check.

## F-3 C++23 full test suite

**Method.** The project's own CMake build (CMake 4.1.3 from PyPI, Ninja), with GoogleTest 1.14
and fmt 9.1 from Ubuntu 24.04.

| Toolchain | Unit tests (`cstr_view_tests`) | Demo (`test_console_app`) |
|---|---|---|
| GCC 14.2.0 | ✅ 156/156 | ✅ |
| GCC 13.3.0 | ✅ 156/156 | ✅ |
| Clang 20.1.2 (libstdc++ 14) | ✅ 156/156 | ✅ |
| Clang 19.1.1 (libstdc++ 14) | ✅ 156/156 after setting `CMAKE_CXX_SCAN_FOR_MODULES=OFF` (F-6) | ✅ (same) |
| Clang 18.1.3 (libstdc++ 14) | ✅ 156/156 | ❌ `std::expected` is unavailable. libstdc++ provides it only when the compiler reports `__cpp_concepts >= 202002L`, which Clang first does in version 19. |
| Clang 17.0.6 (libstdc++ 14) | ❌ libstdc++ 14 `<tuple>`: "no matching function for call to `get`" | ❌ |

**Conclusion.** The brief's statement holds: the suite needs GCC 14+ and Clang 19+. GCC 13
also works today, but that is not promised (decision D-008).

## F-4 Defect: the fmt tests are silently compiled out

- `tests/cstr_view_tests.cpp:1181` and `:1203` test for the macro `CJM_CT_STRING_VIEW_HAS_FMT`.
  The header actually defines `CPS_CT_STRING_VIEW_HAS_FMT` (`ct_string_view.hpp:24`).
- As a result the three `CtSvFmtFormatter.*` tests are never compiled. The run reports 156 tests
  and nothing visibly fails.
- The stale name also appears in `README.md:598` and `LEGENDUM.md:492`.
- **Experiment.** I enabled the block by defining the stale macro on the command line (GCC 14 with
  fmt 9.1). All three tests pass, for 159 in total, so the fix is low-risk (task T2).
- **CI lesson.** A green run can hide tests that never compiled. T2 therefore adds a guard: if a
  build expects fmt and the header did not detect it, compilation fails loudly.

## F-5 Defect: comparisons do not compile with libc++

- **The design.** `basic_ct_string_view` and `basic_fixed_string` compare with "anything
  nothrow-convertible to `std::basic_string_view`". This is expressed by the concept
  `nothrow_convertible_to` (`fixed_string.hpp:36-44`). The operators are at
  `ct_string_view.hpp:555-571` and `fixed_string.hpp:352-369`, with 12 uses in total.
- **The cause.**
  - A string literal converts through the `basic_string_view(const CharT*)` constructor.
  - libstdc++ and the MSVC STL mark that constructor `noexcept`, which the standard permits as a
    strengthening.
  - libc++ uses the declaration exactly as the standard writes it, which is not `noexcept`.
- **The symptom.** With libc++, `view == "literal"` has no viable operator.
  `tests/cxx20_header_smoke.cpp:50-51` fails with "invalid operands to binary expression".
- **Impact.** Every libc++ user is affected:
  - macOS/AppleClang;
  - `-stdlib=libc++` on Linux;
  - Conan profiles with `compiler.libcxx=libc++`.

  A ConanCenter `test_package` or any macOS consumer would hit this.
- **Assessment.** This is a conformance defect: the library depends on behaviour the standard does
  not guarantee, which goes against the brief's "rely on standard C++". It is fixed in T3
  (decision D-024).

## F-6 CMake findings

- **The 4.0.2 minimum is higher than most environments provide.**
  - GitHub's `ubuntu-22.04`, `ubuntu-24.04` and `windows-2022` images ship CMake 3.31.6.
  - Ubuntu 22.04 packages CMake 3.22, and Ubuntu 24.04 packages 3.28.
  - Only `ubuntu-26.04` and `windows-2025-vs2026` (4.4.3) meet the minimum.
  - Nothing in the build uses a CMake 4.x feature.
- **C++ module scanning is on by default.**
  - This applies to CMake ≥ 3.28 under the newer policies (including every policy version the
    project would select) with C++20 targets, the Ninja generator and Clang.
  - CMake then scans sources for module dependencies, which needs `clang-scan-deps`. Ubuntu
    packages that tool separately (`clang-tools-<N>`).
  - With Clang 19 the build failed until scanning was turned off. The project uses no modules, so
    scanning should be disabled for its targets (T4).
- **fmt and GoogleTest are mandatory for every configure** (F-1). This is the main obstacle for
  `add_subdirectory`/`FetchContent` consumers and for packaging.

## F-7 fmt auto-detection

- If `<fmt/format.h>` is anywhere on the include path, `ct_string_view.hpp` includes it (and
  `<fmt/xchar.h>` when available) and enables the `fmt::formatter` specialisation. Include order
  does not matter.
- README and LEGENDUM say "include `<fmt/format.h>` *before* …". That is inaccurate, and T2
  corrects it.
- Consequences for packaging:
  - The library's API surface and compile time change depending on what else happens to be
    installed, for example in a vcpkg or Conan tree.
  - There is no way to opt out.
  - vcpkg's maintainer guide forbids ports whose behaviour depends on which other ports are
    installed, and requires features to control dependencies explicitly.
  - Addressed in T11 (decision D-020).

## F-8 GitHub-hosted runners (snapshot 2026-10-10)

| Label | OS | GCC | Clang | MSVC / Visual Studio | CMake | Notes |
|---|---|---|---|---|---|---|
| `ubuntu-22.04` | Ubuntu 22.04 | 10.5, 11.4, 12.3 | 13, 14, 15 | — | 3.31.6 | Ninja 1.13, vcpkg, Docker |
| `ubuntu-24.04` (= `ubuntu-latest`) | Ubuntu 24.04 | 12.4, 13.3, 14.2 | 16, 17, 18 | — | 3.31.6 | The Ubuntu archive also offers `clang-19`/`clang-20` (with libc++) and `g++-11` |
| `ubuntu-26.04` | Ubuntu 26.04 | 13.4, 14.3, 15.2 | 20.1, 21.1, 22.1 | — | 4.4.3 | |
| `windows-2022` | Server 2022 | — | LLVM 20.1.8; VS-bundled clang-cl | VS 2022 17.14 (MSVC 14.44, v143). Whether the v142 x64 toolset is installed is not yet verified. | 3.31.6 | vcpkg, Ninja |
| `windows-2025-vs2026` (= `windows-latest`, `windows-2025`) | Server 2025 | — | LLVM 20.1.8; VS-bundled clang-cl | VS 2026 18.10 (MSVC 14.5x, v145), plus MSVC 14.44 side by side | 4.4.3 | vcpkg, Ninja |
| `macos-15`, `macos-26` (= `macos-latest`) | macOS, arm64 | — | AppleClang (Xcode) | — | — | Relevant only if macOS is in scope (Q-07) |

- Arm64 Linux and Windows runners also exist (`ubuntu-24.04-arm`, `windows-11-arm`).
- Conan is not preinstalled on any image. CI installs a pinned version with pipx or pip.
- Sources: the `actions/runner-images` README and the per-image software lists.

## F-9 MSVC STL: minimum Clang for clang-cl

From `stl/inc/yvals_core.h` at the `microsoft/STL` release tags:

| STL version | Requires Clang | Requires MSVC |
|---|---|---|
| VS 2022 17.10 | ≥ 17 | ≥ 19.40 |
| VS 2022 17.14 | ≥ 19 | ≥ 19.44 |
| `main` (next VS 2026 update) | ≥ 22 | ≥ 19.52 |

**Implication.**
- With clang-cl, the floor comes from the MSVC STL in use. The library's own Clang 17 floor only
  matters with older STLs.
- The practical clang-cl matrix is therefore the clang-cl bundled with each supported Visual Studio,
  plus the standalone LLVM on the runner (decision D-010).

## F-10 Registry rules that shape the design

**vcpkg maintainer guide:**
- Port names must be distinctive. `<github-owner>-<repo>` is an accepted way to disambiguate.
- A port must not change behaviour depending on which other ports are installed. Features must
  control dependencies explicitly.
- If upstream does not provide CMake exports, the port's exports must use an `unofficial-` prefix.
  An upstream-provided config is used as-is.
- Default features must not add APIs.
- Ports disable "irrelevant-in-vcpkg components of the build such as tests or examples".
- New ports must be tested in at least one official triplet.
- New PRs should start as drafts.

**ConanCenter:**
- It is "not a testing service". Recipes must not build or run unit tests by default.
- `test_package` is only a consumer smoke test.
- Options should match upstream defaults.
- Contributors must sign a CLA.
- New recipe updates are published only for Conan 2.

**Consequence.** Neither registry is a natural home for a "test-suite package". The test suite
therefore gets in-repo vcpkg and Conan configurations instead (decision D-022, question Q-08).

## F-11 Package-name availability (checked 2026-10-10)

- **vcpkg curated registry.** `ct-string`, `ctstring`, `cstr-view` and `cps-ct-string` are free.
  `fixed-string` is taken by an unrelated project.
- **ConanCenter.** `ct_string`, `ctstring`, `cstr_view`, `fixed_string` and `cps_ct_string` are
  free.
