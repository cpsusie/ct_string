# Findings: evidence gathered for Task 1

> Snapshot taken **2026-10-10** against commit `ab2a8e5` (branch `develop-release_1`).
> This file records facts and measurements only. The decisions drawn from them are in
> [DECISIONS.md](DECISIONS.md), and the plan that acts on them is in [PLAN.md](PLAN.md).
> Runner images and compilers change, so re-check any finding before relying on it months later.
> F-14 was added on 2026-10-11, during T3, F-15 on 2026-10-18, during T4, and F-16 and F-17 on
> 2026-10-27, during T5.

## F-1 Repository baseline

- **Library.** Header-only, with six public headers in `inc/ct_str/`: `ct_string_view.hpp`,
  `fixed_string.hpp`, `char_fold.hpp`, `ctsv_comparators.hpp`, `ctsv_containers.hpp` and
  `ctsv_format_registration.hpp`. Namespace `cps::ct_string`. MIT licence.
- **Tests:**
  - a GoogleTest suite in `tests/`, with 156 tests that currently run (see F-4);
  - a C++20-pinned smoke translation unit, `tests/cxx20_header_smoke.cpp`;
  - a C++23 demo program, `test_console_app/`. It also checks itself, in `main.cpp`:
    `static_assert`s (lines 291–316), runtime `assert`s that are active only in builds without
    `NDEBUG` (lines 273–276), and a non-zero exit code on failure (lines 325 and 333).
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
- **Outcome (T3, 2026-10-11).** Fixed as D-030 describes. The smoke target, now with a second
  translation unit of compile-time comparison checks, builds with Clang 17 + libc++ 17 and with
  Clang 20 + libc++ 20. With Clang 20 + libc++ 20, the whole suite (175 tests) passes, and so does
  the demo with fmt used header-only. What T3 found on the way is in F-14.

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
| `windows-2022` | Server 2022 | — | LLVM 20.1.8; VS-bundled clang-cl | VS 2022 17.14 (MSVC 14.44, v143). The x64 v142 toolset is **not** installed: only its ARM, ARM64 and UWP parts are. Older toolsets can be added during a job (F-13). | 3.31.6 | vcpkg, Ninja |
| `windows-2025-vs2026` (= `windows-latest`, `windows-2025`) | Server 2025 | — | LLVM 20.1.8; VS-bundled clang-cl | VS 2026 18.10 (MSVC 14.5x, v145), plus MSVC 14.44 side by side. No x64 v142. | 4.4.3 | vcpkg, Ninja |
| `macos-15`, `macos-26` (= `macos-latest`) | macOS, arm64 | — | AppleClang (Xcode) | — | — | Relevant only if macOS is in scope (Q-07) |

- Arm64 Linux and Windows runners also exist (`ubuntu-24.04-arm`, `windows-11-arm`).
- Conan is not preinstalled on any image. CI installs a pinned version with pipx or pip.
- Sources: the `actions/runner-images` README and the per-image software lists.

## F-9 MSVC STL: minimum Clang for clang-cl

From `stl/inc/yvals_core.h` at the `microsoft/STL` release tags:

| STL version (toolset) | Requires Clang | Requires MSVC |
|---|---|---|
| VS 2022 17.10 (14.40) | ≥ 17 | ≥ 19.40 |
| VS 2022 17.11 (14.41) | ≥ 17 | ≥ 19.40 |
| VS 2022 17.12 (14.42) | ≥ 17 | ≥ 19.41 |
| VS 2022 17.13 (14.43) | ≥ 18 | ≥ 19.42 |
| VS 2022 17.14 (14.44) | ≥ 19 | ≥ 19.44 |
| `main` (next VS 2026 update) | ≥ 22 | ≥ 19.52 |

**Implication.**
- With clang-cl, the minimum Clang comes from the MSVC STL in use, as well as from the library.
- The library's own floor, Clang 17, can still be tested on Windows: pair it with an STL that
  accepts it (14.42 at the newest). Clang 18 pairs with 14.43, and Clang 19 or later with 14.44.
- Official LLVM Windows installers exist for every release from 17 on. LLVM 17.0.6 and 18.1.8 were
  checked on the `llvm/llvm-project` releases page. Older toolsets can be installed side by side
  (F-13).
- So clang-cl can be tested from the library's floor upwards: one job per Clang major, each with an
  STL that accepts it, plus the clang-cl bundled with Visual Studio (decision D-010).

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

## F-12 Branches, and the GitHub rules that shape CI (checked 2026-10-10)

**Branches:**
- `main` is the default branch and the only protected one. `develop` points at the same commit.
- `main` is an ancestor of `develop-release_1`, which is two commits ahead (LEGENDUM and the
  brief). Merging `develop-release_1` into `main` is therefore a fast-forward today, as long as
  nothing else lands on `main`.
- `develop-handeval` has one work-in-progress commit that is on no other branch. This plan does not
  touch it.
- No branch has a `.github/` directory yet.

**What works only from the default branch** (GitHub documentation):
- Push and pull-request runs use the workflow file in the commit being tested. CI therefore works
  on `develop-release_1` and its PRs as soon as the workflow is merged there.
- Scheduled runs use only the workflow files on the default branch, and they test that branch.
- The manual "Run workflow" button (`workflow_dispatch`) appears only for workflow files on the
  default branch.
- Dependabot reads its configuration only from the default branch. Its `target-branch` setting can
  send version updates to another branch, but security updates always target the default branch.
- PR and issue templates take effect only from the default branch.

**Required checks and rulesets** (GitHub documentation):
- A required check passes if it is successful, skipped or neutral, and a skipped job reports
  success. So a job that is skipped because an earlier job failed would not block a merge.
- A ruleset can restrict the merge methods allowed on its branches.
- Branch protection rules and rulesets can stop "Automatically delete head branches" from deleting
  a protected branch, for example `develop-release_1` after it is merged into `main`.

## F-13 MSVC toolsets that can be installed side by side (checked 2026-10-10)

- The Visual Studio 2022 installer offers every x64 MSVC toolset from **14.29** (VS 2019 16.11,
  the v142 toolset) through **14.44** (VS 2022 17.14) as a separate component, one per VS 2022
  minor version: 16 toolsets in all.
- The runner images preinstall only the newest x64 toolset of each Visual Studio (F-8). A job can
  add any of the others with the Visual Studio installer before building. T7 measures how long that
  takes.
- Each toolset brings its own MSVC STL. That is what lets a clang-cl job pair an older Clang with an
  STL that accepts it (F-9).
- Sources: the Visual Studio 2022 Build Tools component list in `MicrosoftDocs/visualstudio-docs`,
  and the Visual Studio component tables in the `actions/runner-images` software lists.

## F-14 libc++ details found while verifying T3 (checked 2026-10-11)

- **Ubuntu packages.** On Ubuntu 24.04, `libc++-17-dev` and `libc++-20-dev` cannot be installed
  together: installing one removes the other. Both put their headers behind
  `/usr/include/c++/v1`, and every `clang++-N -stdlib=libc++` then uses whichever is installed.
  Check `_LIBCPP_VERSION` before trusting a result. T8's libc++ jobs need one libc++ per job, or
  explicit include and library paths.
- **`std::format`.** libc++ 17 and 18 ship `<format>`, but do not define `__cpp_lib_format`
  (their `<version>` has the line commented out). libc++ 19 and later define it (`202110L` in
  libc++ 20). Checked: libc++ 17 and 20 locally, 18 and 19 in the LLVM `release/18.x` and
  `release/19.x` sources.
  - The library's `std::formatter` is guarded by that macro (`ct_string_view.hpp:18`, `:794`), so
    with libc++ 17 or 18 it is left out.
  - The suite's `std::format` tests have the same guard (`tests/cstr_view_tests.cpp`, the
    `CtSvFormatter` block). With Clang 20 and libc++ 17, 168 of the 175 tests ran, the seven
    `CtSvFormatter.*` tests were silently missing, and the run was green: the F-4 pattern (Q-25).
- **`nullptr`.** In C++20 mode, with libstdc++ 14 and with libc++ 20, `std::basic_string_view`
  converts from `nullptr`. C++23 (P2166) removes that conversion. The comparators' concept
  `ctsv_lookup_key` and the container lookups therefore accept `nullptr` in C++20 mode (Q-24).
  - **Outcome.** At your request, T3 applies Q-24 (a), recorded as D-032. Doing so found two more
    cases:
    - `std::hash<basic_ct_string_view>` accepted `nullptr` in C++23 mode too, through its
      overload for `const char_type*`. D-032 covers it.
    - The view's members that mirror `std::basic_string_view`, such as `compare`, `starts_with`
      and `find`, take `nullptr` in every language version, exactly as the standard type does
      (Q-26).
- **Clang 16** still fails the smoke target, with the same compiler bug as in F-2. T3 did not
  change that.

## F-15 Build environment details found while doing T4 (checked 2026-10-18)

- **CMake versions.** CMake 3.21.4 and 4.4.4, both from PyPI, configure, build and test the T4
  project with GCC 14. 4.4.4 is the newest CMake on PyPI, so T4 makes 4.4 the policy maximum
  (D-017).
- **Where module scanning fails (F-6).** On `develop-release_1` before T4, with CMake 4.4.4 and
  Clang 19 or Clang 20, and `CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS` pointing at a missing file,
  configuring fails. It fails inside GoogleTest's package file, which looks for threads: the test
  compile for `-pthread` fails, so CMake reports "Could NOT find Threads". With `clang-scan-deps`
  present, the old build has 39 scanning rules. After T4 it has none, and the same simulation
  configures, builds and passes all 176 tests.
- **Presets.**
  - A preset's `condition` is inherited through `inherits`. With CMake 3.21.4 and 4.4.4 on Linux,
    the MSVC and clang-cl presets are hidden.
  - Build and test presets do not take the condition of the configure preset they name, so they
    need their own.
  - `-D` on the command line overrides a preset's cache variable, such as `CMAKE_CXX_COMPILER`.
  - Preset schema 3 is the newest that CMake 3.21 reads.
- **The demo needs the `en_US.UTF-8` locale** (`test_console_app/setup_streams.cpp:16` and `:19`).
  In a plain `ubuntu:24.04` container it prints "Failed to setup streams: Failed to set locale to
  "en_US.UTF-8"" and exits with 255, in Debug and in Release. After installing `locales` and running
  `locale-gen en_US.UTF-8`, both exit with 0 (Q-15).
- **The `ubuntu-24.04` runner** (image `20261012.334.1`): GCC 14.2 is `g++-14`, and plain `g++` is
  GCC 13. CMake 3.31.6 and Ninja 1.13.2 are installed; GoogleTest and fmt are not. The
  `windows-2022`, `windows-2025` and `macos-15-arm64` images also have Ninja 1.13.2.
- **`actions/checkout`.** The newest release is v7.0.1 (2026-07-20), commit
  `3d3c42e5aac5ba805825da76410c181273ba90b1`.
- **Branches.** `main` is still the default branch and the only protected one. Neither `main` nor
  `develop-release_1` has a `.github/` directory.
- **A trap when testing in containers.** `find_package(... CONFIG)` also searches directories named
  `<package>*` directly under each prefix, `/` included. A host directory mounted at `/fmtcmake`
  was therefore taken for fmt's package. Mount dependencies at their real paths.
- **Sources:** the T4 runs listed in PLAN.md (T4, Outcome); the `actions/runner-images` software
  lists; the `actions/checkout` release page.

## F-16 CI details found while doing T5 (checked 2026-10-27)

**The first CI runs** (PR #21: runs [40583728552], [40584049573] and [40584323698]):
- **Toolchain.** `ubuntu-24.04` image `20261023.334.3`, with GCC `14.2.0-4ubuntu2~24.04.1` as
  `g++-14`, CMake 3.31.6 and Ninja 1.13.2. Ubuntu's packages: `libfmt-dev` 9.1.0+ds1-2,
  `libgtest-dev` and `libgmock-dev` 1.14.0-1.
- **Images roll out gradually.** In the first run, the Release job ran on image `20261026.354.1`
  and the other jobs on `20261023.334.3`. Two jobs of one run can therefore see different images;
  the version step in every job shows which.
- **Times.** Debug 35–40 s, Release 46–67 s, the job without fmt 9–27 s, `ci-ok` 3–4 s; a whole
  run 52–72 s from start to finish.
- **`ci-ok` turns red.** The temporary commit `78a4a3f` installed fmt in the job that must not have
  it. That job's guard failed ("g++-14 finds <fmt/format.h>"), and `ci-ok` printed
  `gcc-14-without-fmt: failure` and failed. The next commit reverted it, and the run was green.

**GitHub behaviour:**
- **Concurrency.** Within one `concurrency` group, GitHub keeps one running and one pending run.
  When another run arrives, the pending one is cancelled, even with `cancel-in-progress` off. A
  new optional setting, `queue: max`, keeps up to 100 pending runs, but cannot be combined with
  `cancel-in-progress: true`. `actionlint` 1.7.12 does not know it yet ("unexpected key
  \"queue\""). Source: GitHub's workflow-syntax documentation.
- **Shell.** A `run` step without a `shell` runs `bash -e {0}`: a failure on the left of a pipe is
  lost. With `shell: bash` set explicitly, it runs `bash --noprofile --norc -e -o pipefail {0}`
  (seen in the logs).
- **Dependabot** applies a 3-day cooldown to version updates by default, even when none is
  configured; security updates are never delayed. Source: the Dependabot options reference.

**Repository settings** (read through the API on 2026-10-27; nothing was changed):
- The default branch is `main`. Squash merges, merge commits and rebase merges are all allowed;
  "Automatically delete head branches" is off.
- One ruleset, "Prohibit Push n stuff", is active, with no bypass. It blocks deletion and force
  pushes on **the default branch**, whatever that is (`~DEFAULT_BRANCH`), not on `main` by name.
  That is why `main` shows as protected. Once `develop-release_1` is the default branch, the
  ruleset would protect it instead, and `main` would have no protection left. The settings steps
  in PR #21 therefore protect `main` by name before switching the default branch.
- Classic branch protection rules and the Dependabot settings cannot be read with this session's
  token (HTTP 403), so the steps ask you to look.

**Tools** (run locally; none is added to the repository):
- `actionlint` 1.7.12, with `shellcheck` 0.9.0: no findings.
- `zizmor` 1.31.1, a security linter for workflows, offline and online audits: one finding, the
  missing Dependabot cooldown (it recommends at least 7 days). T5 adds it (D-035).
- `check-jsonschema` validates `ci.yml` and `dependabot.yml` against their published schemas.

**Runner images** (software lists dated 2026-10-23; F-8 is the earlier snapshot):

| Label | Image | GCC | Clang | CMake |
|---|---|---|---|---|
| `ubuntu-22.04` | 20261023.322.3 | 10.5, 11.4, 12.3 | 13.0.1, 14.0.0, 15.0.7 | 3.31.6 |
| `ubuntu-24.04` | 20261023.334.3 | 12.4, 13.3, 14.2 | 16.0.6, 17.0.6, 18.1.3 | 3.31.6 |
| `ubuntu-24.04-arm` | 20261023.149.3 | 12.4, 13.3, 14.2 | 16.0.6, 17.0.6, 18.1.3 | 3.31.6 |
| `ubuntu-26.04` | 20261023.163.3 | 13.4, 14.3, 15.2 | 20.1.8, 21.1.8, 22.1.2 | 4.4.4 |

- Every one has Ninja 1.13.2, `jq` and `yq` 4.54.1, and Docker.
- Labels listed by `actions/runner-images`: `ubuntu-22.04`, `ubuntu-24.04`, `ubuntu-26.04` and
  their `-arm` variants, `ubuntu-slim`, `macos-15` and `macos-26` (`macos-14` is deprecated),
  `windows-2022`, `windows-2025-vs2026` (= `windows-latest`), `windows-11-arm` and
  `windows-11-vs2026-arm`.
- **CMake releases.** The newest is 4.4.5 (2026-10-27), and 4.5.0-rc1 is out. The newest 3.21 is
  3.21.7.
- **`actions/checkout`.** v7.0.1 (commit `3d3c42e5…`) is still the newest release.

[40583728552]: https://github.com/cpsusie/ct_string/actions/runs/40583728552
[40584049573]: https://github.com/cpsusie/ct_string/actions/runs/40584049573
[40584323698]: https://github.com/cpsusie/ct_string/actions/runs/40584323698

## F-17 Linux toolchains, fmt and CMake for T6 (prototype, 2026-10-27)

A throwaway prototype in Docker containers, with the repository mounted read-only. Builds use the
T4 presets with `-Werror`. "Full build" means the C++20 checks, the C++23 suite and the demo.

**`gcc:11.1`** (the official image, digest
`sha256:02eaa5ab926f4dd4e4eeaec2ded6d24e3f3921b1797e4d48d0ad95e9b17f7d90`, built 2021-07-22):
- Debian 11 (bullseye), glibc 2.31, GCC 11.1.0. It has `git` 2.30.2, `curl`, `unzip` and `make`,
  but no CMake and no Ninja. The official Node 24.11.1 binary runs in it, so `actions/checkout`
  can run there.
- Pulling it took 16 s (449 MB compressed).
- CMake from Kitware's release archive, checked against Kitware's SHA-256 file, works there, and
  so does Ninja 1.13.2's release binary (`ninja-linux.zip`, SHA-256
  `5749cbc4e668273514150a80e387a957f933c6ed3f5f11e03fb30955e2bbead6`).
- `apt-get update` still works against bullseye, but `bullseye-backports` is already gone, and
  bullseye's long-term support ended in August 2026: its packages may move to
  `archive.debian.org`.
- The C++20 checks build without fmt, and with every fmt from 5.3.0 to 12.0.0, including Debian's
  `libfmt-dev` 7.1.3.

**fmt: the real minimum is 8.0.**
- The C++20 checks never format a view, so they pass with any fmt. A small program that formats
  one fails with fmt 6.0.0 to 7.1.3: `ct_string_view.hpp:857` calls the base formatter's `format`
  from a `const` member, and before fmt 8 that function is not `const` ("discards qualifiers").
- fmt 8.0.1 and every later version tested format narrow and wide views.
- fmt before 8 has no `<fmt/xchar.h>`, so the header silently leaves out wide formatting.

**`ubuntu:22.04`:** `g++-11` 11.4.0, `g++-12` 12.3.0, CMake 3.22.1, `libfmt-dev` 8.1.1,
`libgtest-dev` 1.11.0.
- The C++20 checks build with GCC 11.4 and 12.3, with fmt and without it.
- With GoogleTest installed, the suite builds there too, but runs 169 tests, not 176: its
  libstdc++ has no `<format>`, so the seven `CtSvFormatter` tests are left out. The demo does not
  build (`<format>` not found).

**`ubuntu:24.04`:** `g++-13` 13.3.0, `g++-14` 14.2.0, Clang 16.0.6, 17.0.6 and 18.1.3, and
`clang-19` 19.1.1 from the Ubuntu archive (`clang-20` 20.1.2 is there too); fmt 9.1.0, GoogleTest
1.14.0. Every Clang uses GCC 14's libstdc++.

| Build | Result |
|---|---|
| C++20 checks with fmt: GCC 13, GCC 14, Clang 17, 18, 19 | pass |
| Full build, GCC 14 and Clang 19, Debug and Release | pass: 176 tests, demo exits with 0 |
| Full build, GCC 13 | passes (D-008 does not promise it) |
| Full build, Clang 18 | the demo fails (`std::expected` missing); the suite passes |
| Suite, Clang 17 | fails in libstdc++'s `<tuple>`, from `tests/cstr_view_tests.cpp:1035` |
| C++20 checks, Clang 16 | fails, as in F-2 |

**`ubuntu:26.04`:** `g++-15` 15.2.0 (also 13.4 and 14.3), Clang 20.1.8, 21.1.8 and 22.1.2,
CMake 4.2.3, `libfmt-dev` **10.1.1**, GoogleTest 1.17.0. Every Clang uses GCC 15's libstdc++.
- GCC 15, full build, Debug and Release: 176 tests pass, and the demo exits with 0.
- **Clang 20, 21 and 22 fail every build with Ubuntu's fmt 10.1.1**, inside fmt's own header:
  `format-inl.h:61: error: call to consteval function … is not a constant expression`. A file that
  only includes `<fmt/format.h>` fails the same way when `FMT_HEADER_ONLY` is defined, with no
  warning flags; without `FMT_HEADER_ONLY` it compiles. The C++20 checks and the suite link
  `fmt::fmt-header-only`, so they fail; the demo links the compiled `fmt::fmt`, and works.
- In header-only mode, fmt 10.2.1 and 11.0.2 fail with Clang 20–22 too; fmt 11.1.4, 11.2.0 and
  12.2.0 work. With fmt 12.2.0, the newest release (2026-06-16), built from its release archive,
  Clang 20, 21 and 22 and GCC 15 pass the full build in Debug and Release, with no new warnings.

**CMake** (`ubuntu:24.04`, GCC 14, full Debug build, archives from Kitware checked against their
SHA-256 files): 3.21.0 (both presets, and the C++20 checks alone), 3.21.7, 4.4.5 (the newest,
released 2026-10-27) and 4.5.0-rc1 all build the project and pass the 176 tests; the demo exits
with 0. Ubuntu's own CMakes (3.22.1, 3.28.3 and 4.2.3) work with the presets too. CMake 4.4.5's
summary line no longer says "0 tests failed", so nothing should look for that phrase.

**arm64** could not be tried: this machine cannot run arm64 containers (no emulation).
`ubuntu-24.04-arm` has the same compilers and CMake as `ubuntu-24.04` (F-16).

**Sources:** the prototype's scripts and logs are kept with the Copilot session, not in the
repository.
