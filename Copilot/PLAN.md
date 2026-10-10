# Development and release plan: `ct_string`

> **Status:** Task 1 deliverable, for your review. **Last updated:** 2026-10-10.
> **Approver:** @cpsusie. **Author:** Copilot.
> **Brief:** [develop-release-project.md](develop-release-project.md).
> **Companion documents:**
> - [DECISIONS.md](DECISIONS.md): every decision and the reason for it.
> - [QUESTIONS.md](QUESTIONS.md): questions for you, each with a recommended default.
> - [FINDINGS.md](FINDINGS.md): the measurements behind this plan.

## 1. Goal

Publish `ct_string` as a **1.0 release that can be installed with vcpkg and Conan**. At that point:

- **The C++20 library floor is enforced by CI** on every supported toolchain:
  - GCC 11.1+ and Clang 17+ on Linux;
  - MSVC from VS 2019 16.11 or VS 2022, through VS 2026;
  - Visual Studio's clang-cl.
- **The C++23 test suite runs in CI** on modern GCC, Clang and Visual Studio, and stays optional
  for users.
- **The test suite has its own vcpkg and Conan configurations.**
- **Every pull request is tested as it would be merged.**
- **README.md (English) and LEGENDUM.md (Latin) describe it all**, in parallel.

The library itself stays header-only, standard C++20, and free of third-party dependencies.

## 2. What Task 1 found

Details and reproduction steps are in [FINDINGS.md](FINDINGS.md).

| # | Finding | Consequence |
|---|---|---|
| F-2 | **Earliest compilers that build the library:** GCC **11.1**; Clang **17** with libstdc++. Clang 16 hits a compiler bug. Clang ≤ 15 lacks P0634 (implicit `typename`). GCC 10 lacks heterogeneous unordered lookup. | These become the CI floors (decisions D-006 and D-007; questions Q-04 and Q-05). |
| F-3 | **Full C++23 suite:** passes on GCC 13 and 14 and on Clang 19 and 20. Clang 18 cannot build the demo (`std::expected`); Clang 17 cannot build the tests. | Matches your statement "GCC 14+, Clang 19+" (D-008). |
| F-4 | **Bug:** the fmt tests are silently compiled out. The tests check a stale macro `CJM_CT_STRING_VIEW_HAS_FMT`; the header defines `CPS_…`. | Fix first (T2). |
| F-5 | **Bug:** with **libc++**, `view == "literal"` does not compile. The code relies on a vendor-specific `noexcept` on `string_view`'s constructor. | Blocks macOS, libc++ users and Conan libc++ profiles. Fix in T3. |
| F-6 | **CMake:** the 4.0.2 minimum excludes most runner and distribution CMakes. fmt and GoogleTest are required even by consumers. C++ module scanning breaks some Clang setups. | T4. |
| F-7 | **fmt is auto-included** whenever it is on the include path, and there is no off-switch. | Deterministic control needed for packaging (T11). |
| F-8, F-9 | **Runners.** GitHub runners cover GCC 10–15 and Clang 13–22, plus VS 2022 17.14 and VS 2026. The MSVC STL fixes the minimum Clang for clang-cl (19 for VS 2022 17.14). | Shapes the CI matrix (D-009, D-010). |
| F-10 | **Registries.** Neither is a home for a test-suite package: ConanCenter recipes must not build or run tests, and vcpkg ports switch tests off. | The test suite gets in-repo configurations (D-022, Q-08). |

## 3. Working agreement

### 3.1 How every task runs

1. **Plan.** The task's plan is already written in this file and was approved when you merged the
   previous PR.
2. **Branch.** Copilot creates a branch from the latest `develop-release_1`.
3. **Implement.** Copilot implements the task in small commits, with tests and documentation.
   From T5 onward, CI runs on the PR's merge commit.
4. **Plan ahead.** The same PR updates this plan (status, plus the **next** task in full detail)
   and the decision log.
5. **Review.** You review the code, the docs (including the Latin) and the plan for the next task.
   Then you approve and merge.
6. **Next task.** Only then does the next task start. No two task PRs are open at once.

### 3.2 Definition of done (applies to every task, on top of the task's own)

- [ ] The PR targets `develop-release_1`, covers one topic, and is small.
- [ ] It builds without warnings (`-Wall -Wextra -Werror` or `/W4 /WX`). All tests pass locally,
      and in CI once CI exists.
- [ ] Changed or new behaviour has tests.
- [ ] If anything user-visible changed (behaviour, build, packaging, support), README.md and
      LEGENDUM.md are updated **in the same PR**, with the same structure and examples. The Latin
      keeps the existing Caesarian register.
- [ ] New decisions are logged in DECISIONS.md, and answered questions are recorded in
      QUESTIONS.md.
- [ ] PLAN.md has the updated status table, and the next task is planned in full.
- [ ] You have approved and merged it.

### 3.3 What every PR description contains

- **Summary.** The task ID and what changed.
- **How it was verified.** The toolchains and commands used, and the results.
- **Documentation touched.** README and LEGENDUM sections.
- **Decisions and questions.** The D- and Q- IDs involved.
- **Risks and follow-ups.**
- **What you can learn from this PR.** Short notes on the CI, DevOps or packaging concepts used.

### 3.4 Re-planning

- **After every task:** the next task is planned in full, in the task's own PR.
- **After every milestone:** a short retrospective in §10:
  - what went well;
  - what changed;
  - risks updated;
  - the timeline re-baselined.

  Changes in priority go through DECISIONS.md. Old decisions are superseded, not deleted.

## 4. Requirements traceability

Every requirement in the brief maps to the tasks and decisions that satisfy it.

| # | Requirement from the brief | Where it is handled |
|---|---|---|
| R-1 | Public release through **vcpkg** | T9, T12, T19 · D-019, D-021 |
| R-2 | Public release through **Conan** | T9, T13, T20 · D-019, D-021 |
| R-3 | Guidance while learning CI and DevOps | "What you will learn" in each milestone; a learning section in every PR; §12 glossary |
| R-4 | README (English) and LEGENDUM (Latin) updated together, same style | Definition of done §3.2 · D-025 |
| R-5 | Library is C++20, usable with GCC 11.3+ | T4, T6 · D-005, D-006 |
| R-6 | Find the **earliest** working GCC and Clang on Linux and use them in CI | F-2 · T6 · D-006, D-007 |
| R-7 | CI enforces every MSVC with (nearly) full C++20 | T7 · D-009 |
| R-8 | The same for Visual Studio's clang-cl | T7 · D-010 |
| R-9 | Cross-platform, standard C++ only | T3, T8 · D-024 |
| R-10 | Full C++23 suite in CI on GCC, Clang and VS | T5–T8 · D-008 |
| R-11 | Building the test suite is optional for users | T4, T10 · D-018 |
| R-12 | **Separate** vcpkg and Conan configurations for the library and for the test suite | T10, T12, T13 · D-022 · Q-08 |
| R-13 | Hyper-modern style, header-only, no third-party dependencies in the library | All code tasks · D-018, D-020 |
| R-14 | The test project is a separate, buildable project | T10 |
| R-15 | Branch from `develop-release_1`; PRs target it | §3.1 · D-003 |
| R-16 | CI tracks `develop-release_1` and `main`, later every `develop`/`release` branch | T5, T16 · D-014 |
| R-17 | PRs are tested as merged with their base | T5 · D-013 |
| R-18 | One small PR at a time; approval before the next; tests and docs with every change | §3 · D-003 |
| R-19 | A plan with timeline, milestones, tasks, definitions of done, and a decision log in `Copilot/` | This PR (T1) · D-001 |
| R-20 | Plan the following tasks at the end of each task or milestone | §3.4 · D-002 |
| R-21 | "If you require additional information, now is the time to ask" | [QUESTIONS.md](QUESTIONS.md) |

## 5. What the repository looks like at 1.0

Names assume the defaults in Q-01, Q-02 and Q-15.

### 5.1 Repository layout

| Path | Contents |
|---|---|
| `CMakeLists.txt` | Library project: options, install and export rules |
| `CMakePresets.json` | Developer and CI presets |
| `cmake/` | Package-config template |
| `inc/ct_str/*.hpp` | Public headers (unchanged location), plus a new `version.hpp` |
| `tests/` | Standalone C++23 test project: its own `CMakeLists.txt`, vcpkg manifest and Conan recipe |
| `examples/console_app/` | The demo, formerly `test_console_app/` |
| `ports/ct-string/` | vcpkg overlay port for the library |
| `conanfile.py`, `test_package/` | Conan recipe for the library, and Conan's consumer smoke test |
| `.github/workflows/` | `ci.yml` and `release.yml` |
| `.github/dependabot.yml`, templates | Dependabot configuration, PR and issue templates |
| `CHANGELOG.md`, `CONTRIBUTING.md`, `SECURITY.md` | New project documents |
| `README.md`, `LEGENDUM.md` | Updated in parallel |
| `Copilot/` | Plan, decisions, questions, findings |

### 5.2 Ways to consume the library

All of them give the same target, `ct_string::ct_string`, and none needs fmt or GoogleTest unless
you ask for them:

- `FetchContent` or `add_subdirectory`;
- `find_package(ct_string CONFIG)` against an installed tree;
- the vcpkg port;
- the Conan package.

### 5.3 Target CI matrix

Explicit runner labels throughout (D-011).

| Group | Runner | Toolchains | Runs |
|---|---|---|---|
| Linux, C++20 floor and range | `ubuntu-22.04`, `ubuntu-24.04`, `ubuntu-26.04`, and the `gcc:11.1` container | GCC 11.1, 11.4, 12, 13, 14, 15; Clang 17, 18, 19, 20, 21, 22 (libstdc++) | C++20 smoke with and without fmt; install-and-consume |
| Linux, C++23 suite | `ubuntu-24.04`, `ubuntu-26.04` | GCC 14, 15; Clang 19, 20, 21, 22 | Full suite and demo, Debug and Release |
| Linux, libc++ | `ubuntu-24.04` | Clang 17 with libc++ 17; Clang 20 with libc++ 20 | Smoke; suite if feasible |
| Sanitizers | `ubuntu-24.04` | GCC 14, Clang 20 | Suite under AddressSanitizer and UndefinedBehaviorSanitizer |
| Linux arm64 (Q-18) | `ubuntu-24.04-arm` | GCC 14 | Full suite (where `char` is unsigned) |
| CMake versions | `ubuntu-24.04` | CMake 3.21 and newest 4.x | Configure, build, install, consume |
| Windows, MSVC | `windows-2022`, `windows-2025-vs2026` | v142 14.29 (Q-06), v143 14.44, v145 14.5x | Smoke on all; suite on v145 (and v143 while it passes) |
| Windows, clang-cl | `windows-2022`, `windows-2025-vs2026` | Bundled clang-cl for VS 2022 and VS 2026; standalone LLVM 20 | Smoke on all; suite on the newest |
| macOS (Q-07) | `macos-15` | AppleClang | Smoke; suite if feasible |
| Packaging | `ubuntu-24.04`, `windows-2025-vs2026` | — | vcpkg overlay port and `conan create`; test suite built against each package |

### 5.4 Release flow (D-026, Q-11)

1. Reviewed PRs accumulate on `develop-release_1`.
2. A release PR merges it into `main`.
3. You push a tag `vX.Y.Z` on `main`.
4. The release workflow creates the GitHub Release, with notes and checksums.
5. Updates to the vcpkg port and the ConanCenter recipe reference that tag.

## 6. Milestones and tasks

Sizes: **S** is under about 150 changed lines, **M** is about 150–400. Anything larger is split.
"You" marks steps only you can perform.

| Task | Title | Milestone | Size | Depends on | You |
|---|---|---|---|---|---|
| T1 | Development and release plan (this PR) | M0 | S | — | review |
| T2 | Restore the silently disabled fmt tests | M1 | S | T1 | review |
| T3 | Make comparisons portable to libc++ | M1 | M | T2 | review |
| T4 | CMake hygiene: version range, optional tests and dependencies, presets | M2 | M | T3 | review |
| T5 | CI skeleton and security baseline | M2 | S | T4 | **set up ruleset** |
| T6 | Linux compiler matrix (floors and C++23 suite) | M2 | M | T5 | review |
| T7 | Windows matrix (MSVC and clang-cl) | M2 | M | T6 | review |
| T8 | libc++, sanitizers, macOS | M2 | M | T7 | review |
| T9 | Install and export: package config, version header, consumer test | M3 | M | T8 | review |
| T10 | Standalone test-suite project; move the demo | M3 | M | T9 | review |
| T11 | Deterministic fmt integration | M3 | M | T10 | review |
| T12 | vcpkg: library port and test-suite configuration | M4 | M | T11 | review |
| T13 | Conan 2: library recipe and test-suite configuration | M4 | M | T12 | review |
| T14 | Versioning and CHANGELOG | M5 | S | T13 | review |
| T15 | Release workflow, with a dry run | M5 | M | T14 | review |
| T16 | Broaden CI triggers to `develop`/`release` branches | M5 | S | T15 | review |
| T17 | Release documentation pass | M5 | M | T16 | review |
| T18 | Release 1.0.0 | M5 | S | T17 | **merge to `main`, push tag** |
| T19 | Submit the port to `microsoft/vcpkg` | M6 | S | T18 | **fork and open PR** |
| T20 | Submit the recipe to ConanCenter | M6 | S | T18 | **sign CLA, open PR** |
| T21 | Post-release follow-up | M6 | S | T19, T20 | review |

### M0: Plan

#### T1: Development and release plan
- **Deliverables:** `Copilot/PLAN.md`, `Copilot/DECISIONS.md`, `Copilot/QUESTIONS.md` and
  `Copilot/FINDINGS.md`. No code changes; README and LEGENDUM are untouched (D-004).
- **Done when:** you have reviewed it, answered the questions (or accepted the defaults), and
  merged it.

### M1: Correctness fixes found while planning (week 1)

*What you will learn:* how a green test run can hide tests that never compiled, and why "works on
my standard library" is not the same as standard C++.

#### T2: Restore the silently disabled fmt tests (NEXT TASK, planned in full)
- **Why.** Three `CtSvFmtFormatter` tests never compile, because they check a stale macro name
  (FINDINGS F-4). The fmt integration is effectively untested.
- **Branch:** `copilot/t2-fix-fmt-test-macro`, from `develop-release_1`.
- **Steps.**
  1. In `tests/cstr_view_tests.cpp` (lines 1181 and 1203), replace `CJM_CT_STRING_VIEW_HAS_FMT`
     with `CPS_CT_STRING_VIEW_HAS_FMT`.
  2. Make this class of bug impossible to miss:
     - CMake tells the test program that fmt is expected (a compile definition on the test target,
       which always links fmt).
     - The test source then refuses to compile, with a clear message, if fmt was expected but the
       header did not detect it.
  3. Add a wide-character fmt test, guarded by `CPS_CT_STRING_VIEW_HAS_WFMT`. That path is
     untested today.
  4. Correct README "Use case 5" (around lines 595–598):
     - use the right macro name;
     - explain that detection uses `__has_include`, so the header includes fmt itself and include
       order does not matter.
  5. Make the same correction in LEGENDUM (around line 492), in the same Latin register.
  6. Update this plan (status, plus T3 in full detail) and the decision log if needed.
- **Definition of done.**
  - With fmt 9.1 on GCC 14 and on Clang 20 (libstdc++):
    - every test passes: the 156 existing tests, the three restored tests, and the new
      wide-character test;
    - `CtSvFmtFormatter.*` appears in the test list and runs.
  - A build that hides fmt from the header, while the build still expects it, fails with the clear
    message. This is verified locally, and the PR explains how.
  - README and LEGENDUM are corrected in parallel.
  - The library headers are unchanged.
- **Out of scope:** changing how fmt is detected (T11).

#### T3: Make comparisons portable to libc++
- **Goal.** Comparisons between `basic_ct_string_view` or `basic_fixed_string` and string
  literals, arrays, pointers, `std::basic_string` and `std::basic_string_view` compile with
  **libc++ 17+**, as well as libstdc++ and the MSVC STL. Existing meaning and `noexcept` stay as
  they are (FINDINGS F-5, D-024).
- **Scope.** The 12 uses of the `nothrow_convertible_to` concept in `fixed_string.hpp` and
  `ct_string_view.hpp`.
- **Design choice, presented in the T3 PR and logged as a decision:**
  - (a) **Recommended.** Keep the concept's intent and also accept same-character-type arrays and
    pointers explicitly. Their conversion cannot throw; it only has a precondition (a terminating
    null).
  - (b) Accept anything convertible to the string view, with a conditional `noexcept`.
- **Tests:**
  - every operand kind, for every character type;
  - compile-time checks of `noexcept`;
  - local builds of the smoke target with Clang 17 + libc++ 17 and Clang 20 + libc++ 20;
  - the full suite with libc++ 20 where possible. Problems that come only from test code are
    recorded for T8.
- **Docs.** The README and LEGENDUM requirements sections say that libc++ (17 or later) is
  supported.
- **Done when:**
  - the smoke target compiles with libc++ 17–20;
  - libstdc++ and MSVC STL behaviour is unchanged;
  - tests and docs are updated.
- **M1 checkpoint:** re-plan (§3.4).

### M2: Build foundation and CI (weeks 2–4)

*What you will learn:* CMake presets and options; GitHub Actions workflows, jobs, matrices,
runners and containers; required status checks and rulesets; Dependabot; least-privilege tokens.

#### T4: CMake hygiene
- **Scope.**
  - Replace the 4.0.2 minimum with a version range (Q-09).
  - Rename the CMake project per Q-01, keeping `cstr_view` as an alias.
  - Add options (prefix per Q-01) that build:
    - the C++20 smoke check;
    - the C++23 test suite;
    - the demo.

    They default to ON only when this is the top-level project.
  - Look up fmt and GoogleTest only when something enabled needs them.
  - Make the smoke check buildable without GoogleTest and without a C++23 compiler, in both a
    with-fmt and a without-fmt variant.
  - Turn off module scanning for the project's targets.
  - Add `CMakePresets.json` with developer presets (GCC, Clang, MSVC and clang-cl; Debug and
    Release).
  - In README's "Building & integrating" section and LEGENDUM's "Aedificatio":
    - replace the `FetchContent` placeholder URL with `https://github.com/cpsusie/ct_string.git`;
    - document the options and presets.
- **Done when:**
  - The default top-level build still builds and tests everything.
  - A library-only configure succeeds on a machine without fmt or GoogleTest.
  - A scratch consumer using `add_subdirectory` builds with neither installed.
  - The project is verified with CMake 3.21 and the newest 4.x.
  - Clang 19 builds without `clang-scan-deps`.
  - README and LEGENDUM are updated.

#### T5: CI skeleton and security baseline
- **Scope.**
  - `.github/workflows/ci.yml`:
    - Triggers (phase 1, D-014): pushes to and PRs into `develop-release_1` and `main`; manual
      runs; a weekly schedule.
    - Read-only default permissions, concurrency control and job timeouts.
    - Actions pinned to commit SHAs (D-012).
  - One job: `ubuntu-24.04` with GCC 14 and Ubuntu's GoogleTest and fmt. It builds through the
    presets and runs the smoke check (both variants), the full suite and the demo.
  - `.github/dependabot.yml`, for GitHub Actions updates.
  - `.github/pull_request_template.md`, containing the §3.2 checklist.
  - A CI badge in README and LEGENDUM.
- **You:** create the branch ruleset for `develop-release_1` and `main` (Q-16). The PR includes
  click-by-click steps.
- **Done when:**
  - the workflow is green on the PR's merge commit, and again on `develop-release_1` after merging;
  - the ruleset requires the check.

#### T6: Linux compiler matrix
- **Scope.**
  - C++20 floor matrix:
    - GCC 11.1 (official container), 11.4, 12, 13, 14 and 15;
    - Clang 17, 18, 19 (Ubuntu archive), 20, 21 and 22.
  - C++23 suite on GCC 14 and 15 and on Clang 19–22.
  - Jobs for the minimum and newest CMake.
  - Linux arm64 (Q-18).
  - `fail-fast` off, and every job logs its toolchain version.
  - README and LEGENDUM get a Linux toolchain table, stated as "verified by CI".
- **Done when:**
  - all jobs are green;
  - the documented table matches the matrix;
  - the floor jobs are shown to turn red on a scratch branch that adds a C++23-only construct to a
    header.

#### T7: Windows matrix
- **Scope.**
  - The test suite's vcpkg manifest (GoogleTest and fmt, pinned baseline), which T12 extends.
  - MSVC v145, v143 14.44, and v142 14.29 if available (Q-06).
  - clang-cl: the clang-cl bundled with VS 2022 and with VS 2026, plus standalone LLVM 20.
  - The smoke check runs on every one of these. The C++23 suite runs on the newest (Q-20).
  - Each job prints the toolsets it finds.
  - vcpkg binary caching.
  - README and LEGENDUM get a Windows toolchain table.
- **Done when:**
  - all jobs are green;
  - the table matches the matrix;
  - the v142 result is recorded as a decision.

#### T8: libc++, sanitizers and macOS
- **Scope.**
  - libc++ jobs: Clang 17 with libc++ 17 (floor), and Clang 20 with libc++ 20.
  - The suite under AddressSanitizer and UndefinedBehaviorSanitizer, with GCC 14 and Clang 20.
  - macOS `macos-15` with AppleClang, if Q-07 is yes.
  - README and LEGENDUM rows for libc++ and macOS.
- **Done when:**
  - all jobs are green;
  - any exclusions are documented with the reason.
- **M2 checkpoint:** re-plan.

### M3: Packaging-ready CMake (weeks 4–5)

*What you will learn:* install trees, CMake package-config files, namespaced and exported targets,
and consumer tests.

#### T9: Install and export
- **Scope.**
  - Install the headers.
  - Export `ct_string::ct_string`, with a config file and an architecture-independent version file
    (D-019).
  - Install the LICENSE.
  - Add `ct_str/version.hpp`, kept consistent with the CMake project version.
  - An install option, ON when top-level.
  - Make the include guards consistent (Q-02).
  - A CTest consumer test that installs into a scratch prefix and builds a small C++20 program with
    `find_package`. CI runs it on every floor job.
  - README and LEGENDUM document the `find_package` route.
- **Done when:** the installed tree is as designed, the consumer test is green everywhere, and the
  docs are updated.

#### T10: Standalone test-suite project
- **Scope.**
  - `tests/` gets its own C++23 `CMakeLists.txt`. It can be used two ways:
    - (a) from the top-level build, with the in-tree target;
    - (b) on its own, against an installed `ct_string`.
  - Move the demo to `examples/` (Q-15).
  - CI builds the suite in mode (b) against the T9 install tree.
  - README and LEGENDUM gain a "Running the test suite" section.
- **Done when:** both modes work locally and in CI, warning settings are not duplicated, and the
  docs are updated.

#### T11: Deterministic fmt integration
- **Scope.**
  - A macro that forces fmt support on or off, with auto-detection as the default (Q-10).
  - A CMake option (AUTO, ON or OFF) that passes the macro to consumers.
  - CI configurations for "forced off while fmt is present" and "forced on".
  - README and LEGENDUM: the fmt section.
- **Done when:** all three modes are tested in CI, the default behaviour is unchanged, and the docs
  are updated.
- **M3 checkpoint:** re-plan.

### M4: Package-manager configurations (week 6)

*What you will learn:* vcpkg ports, overlay ports, manifests, baselines and features; Conan
recipes, profiles, `test_package`, and the `CMakeDeps`/`CMakeToolchain` generators.

#### T12: vcpkg
- **Scope.**
  - An overlay port `ports/ct-string/`:
    - header-only;
    - uses the upstream CMake config, so no `unofficial-` prefix is needed;
    - a `fmt` feature (Q-10);
    - a usage file.
  - The test suite's manifest consumes the library through the overlay port.
  - CI on Linux and Windows:
    - install the port;
    - build a C++20 consumer;
    - build and run the full suite against the port.
  - README and LEGENDUM gain "Install with vcpkg", using the overlay port until the curated registry
    accepts it.
- **Done when:**
  - the port installs on Linux and Windows;
  - it passes vcpkg's own format and version checks locally;
  - CI is green and the docs are updated.

#### T13: Conan 2
- **Scope.**
  - `conanfile.py`:
    - a header-only library, with no settings in the package ID;
    - `validate()` enforces C++20 and the compiler minimums from D-006, D-007 and D-009;
    - exposes the `ct_string` CMake package and the `ct_string::ct_string` target;
    - option `with_fmt`, off by default.
  - `test_package/`: a C++20 consumer.
  - `tests/conanfile.py`: the test-suite configuration (the library, GoogleTest and fmt; C++23).
  - CI: `conan create` and the suite through Conan, with GCC, Clang and MSVC.
  - README and LEGENDUM gain "Install with Conan".
- **Done when:**
  - all three compiler families are green;
  - `validate()` rejects unsupported compilers with a clear message;
  - the docs are updated.
- **M4 checkpoint:** re-plan.

### M5: Release engineering (weeks 7–8)

*What you will learn:* Semantic Versioning, changelogs, tags, release automation, and checksums.

#### T14: Versioning and CHANGELOG
- **Scope.**
  - `CHANGELOG.md` in the Keep a Changelog format. Its "Unreleased" section collects T2–T13.
  - A short versioning-policy section in README and LEGENDUM.
  - A CI check that the version header, CMake, the vcpkg port and the Conan recipe agree.
- **Done when:** a deliberate mismatch is shown to fail the check, and the docs are updated.

#### T15: Release workflow
- **Scope.**
  - `release.yml`, triggered by `v*` tags or manually. It:
    - re-runs the full CI;
    - archives the source;
    - computes SHA-512 (vcpkg) and SHA-256 (Conan) checksums;
    - creates the GitHub Release with notes from the CHANGELOG.
  - Write permission is granted to that one job only.
  - Dry run: a `v1.0.0-rc.1` pre-release (Q-03).
- **Done when:** the workflow has produced the pre-release, with correct notes and checksums.

#### T16: Broaden CI triggers
- **Scope.** Add the `**develop**` and `**release**` branch patterns to the push and PR triggers
  (phase 2 of D-014).
- **Done when:** a scratch branch whose name contains "develop" runs CI.

#### T17: Release documentation pass
- **Scope.**
  - README and LEGENDUM:
    - every install route;
    - the CI-verified toolchain table;
    - badges;
    - versioning.
  - `CONTRIBUTING.md`, `SECURITY.md` and issue templates (Q-19).
- **Done when:** the install instructions are covered by CI jobs (T9, T12, T13), and you have
  approved the docs, including the Latin.

#### T18: Release 1.0.0
- **Steps.**
  1. Finalise the CHANGELOG and version.
  2. Open the release PR from `develop-release_1` into `main`.
  3. **You:** merge it, then push the annotated tag `v1.0.0`.
  4. The workflow publishes the release.
  5. Verify `FetchContent` by tag, the overlay port and the Conan recipe against the tag.
- **Done when:** the GitHub Release `v1.0.0` exists and every consumption route is verified.
- **M5 checkpoint:** re-plan.

### M6: Publication and maintenance (week 9 onwards; external review times vary)

*What you will learn:* contributing to large open-source registries and working with their
reviewers.

#### T19: Submit the vcpkg port
- **Copilot prepares** the port files pinned to `v1.0.0` (with SHA-512), the version database
  entries, a checklist against the maintainer guide, and step-by-step instructions.
- **You** fork `microsoft/vcpkg`, open a draft PR, and answer reviewers.
- **Done when:** the PR is merged upstream, or the remaining feedback is tracked here.

#### T20: Submit the ConanCenter recipe
- **Copilot prepares:**
  - `recipes/ct_string/all/`: `conanfile.py`, `conandata.yml` and `test_package/`;
  - `config.yml`;
  - a local validation run;
  - step-by-step instructions.
- **You** sign the CLA, open the PR, and answer reviewers.
- **Done when:** the PR is merged upstream, or the remaining feedback is tracked here.

#### T21: Post-release follow-up
- **Scope.**
  - README and LEGENDUM install instructions point at the official registries, with badges.
  - Document the maintenance routine: Dependabot PRs, the weekly CI run, and how to cut a patch
    release.
- **Done when:** the docs are updated, and the plan is closed out or the next cycle is planned.

## 7. Timeline

The timeline is **relative and review-gated**: week 1 starts when this PR is merged. It assumes
about two PRs per week, reviews within one to three days, and no major surprises from MSVC v142,
libc++ or macOS. A late review moves everything after it; the **order** matters more than the
dates.

| Week | Tasks | Checkpoint |
|---|---|---|
| 1 | T2, T3 | M1 re-plan |
| 2 | T4, T5 (plus your ruleset) | — |
| 3 | T6, T7 | — |
| 4 | T8, T9 | M2 re-plan |
| 5 | T10, T11 | M3 re-plan |
| 6 | T12, T13 | M4 re-plan |
| 7 | T14, T15, T16 | — |
| 8 | T17, T18 → **v1.0.0 tagged** | M5 re-plan |
| 9+ | T19, T20 (registry reviews take days to weeks), T21 | M6 close-out |

## 8. Risks and mitigations

Risk IDs use K-n, to avoid clashing with the requirement IDs (R-n) in §4.

| # | Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|---|
| K-1 | MSVC v142 or a clang-cl rejects valid C++20 | Medium | Medium | Probe it first in T7. You decide between a workaround and dropping it (Q-06). |
| K-2 | The test code has more libc++ differences | Medium | Low–Medium | T3 is limited to the library; test-code issues go to T8. |
| K-3 | Runner images change (compilers removed, `windows-2022` retired) | High over a year | Medium | Explicit labels, a weekly run, toolchain versions logged; small PRs to adjust the matrix. |
| K-4 | Docker Hub rate-limits `gcc:11.1` pulls | Low–Medium | Low | An access-token secret or a mirror (Q-17). |
| K-5 | Registry reviewers ask for changes (naming, layout) | Medium | Medium | Follow their guides from the start (F-10). The in-repo port and recipe work in the meantime. |
| K-6 | Review bandwidth, with one PR at a time | Medium | Schedule only | Small PRs; a relative timeline. |
| K-7 | fmt API changes between versions | Low | Low | CI covers several fmt versions (D-016). |
| K-8 | CMake policy changes | Low | Low | The minimum and newest CMake are both tested. |
| K-9 | The quality of the Latin in LEGENDUM | Medium | Low | You review the Latin in each PR; Copilot keeps the existing register and vocabulary. |
| K-10 | Scope creep (modules, more platforms) | Medium | Medium | Backlog (§11) and the decision log. |

## 9. What only you can do

1. Review and merge every PR.
2. Answer [QUESTIONS.md](QUESTIONS.md), now or by each question's "Needed by" task.
3. **After T5:** create the branch ruleset and set the merge options (Q-16).
4. **T6, only if needed:** add a Docker Hub access-token secret (Q-17).
5. **T18:** merge the release PR into `main` and push the `v1.0.0` tag.
6. **T19 and T20:** fork the registries, open the PRs, sign the ConanCenter CLA, and work with the
   reviewers.

## 10. Status

| Task | Status | PR | Notes |
|---|---|---|---|
| T1 | In review | this PR | Plan, decisions, questions, findings |
| T2 | Next: planned in full | — | No blocking questions |
| T3–T21 | Planned (outline) | — | Each is detailed in the PR of the task before it |

Milestone retrospectives will be added here.

## 11. Backlog (after 1.0)

- A C++20 module interface (`import`) alongside the headers.
- An API reference site (Doxygen or similar).
- Measurements of compile-time cost; fuzzing of the case-folding code.
- clang-tidy and coverage reports in CI.
- MinGW-w64 GCC on Windows, and Windows arm64.
- Other package managers (xmake, build2). CPM.cmake already works through `FetchContent`.

## 12. Glossary (for the DevOps parts)

- **Workflow, job, step.** A workflow is a YAML file in `.github/workflows/`. It contains jobs,
  which run in parallel on separate machines, and each job is a list of steps.
- **Runner and label.** A runner is the virtual machine that runs a job. Its label (for example
  `ubuntu-24.04`) selects the image and therefore the preinstalled compilers.
- **Matrix.** One job definition expanded into many jobs, for example one per compiler.
- **Merge ref.** For a PR, GitHub builds a temporary merge of the PR into its base branch. Testing
  that commit is "testing as merged".
- **Ruleset and required check.** Repository rules that block merging until the named CI jobs
  pass, and the branch is up to date if required.
- **Pinning to a SHA.** Referring to a third-party action by commit hash rather than a movable tag.
- **Overlay port.** A vcpkg port kept outside the official registry, used here before submission.
- **Manifest and baseline.** `vcpkg.json` lists the dependencies. The baseline pins the registry
  version, so builds are reproducible.
- **Triplet.** vcpkg's name for a target platform, for example `x64-windows`.
- **Recipe, profile, `test_package`.** In Conan:
  - a recipe (`conanfile.py`) describes how to package the library;
  - a profile describes the compiler and settings;
  - `test_package` is a small consumer that proves the package works.
- **SemVer.** `MAJOR.MINOR.PATCH`: breaking changes bump MAJOR, new features MINOR, fixes PATCH.
