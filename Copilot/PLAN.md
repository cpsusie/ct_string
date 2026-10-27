# Development and release plan: `ct_string`

> **Status:** T1 (PR #14), T2 (PR #15), T3 (PR #17) and T4 (PR #19) merged; M0 and M1 are done
> (retrospectives in §10). You corrected the docs after T3 (PR #18) and reviewed T4's Latin
> (PR #20). T5 in review (PR #21). T6 planned in full.
> **Last updated:** 2026-10-27.
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
  - every MSVC toolset from VS 2019 16.11 (14.29) through VS 2026;
  - clang-cl from Clang 17 up, including the versions bundled with Visual Studio.
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
| F-8, F-9 | **Runners and clang-cl.** GitHub runners cover GCC 10–15 and Clang 13–22, plus VS 2022 17.14 and VS 2026. The MSVC STL sets its own minimum Clang for clang-cl: 17 up to VS 2022 17.12, 19 for 17.14. | Shapes the CI matrix (D-009, D-010). |
| F-10 | **Registries.** Neither is a home for a test-suite package: ConanCenter recipes must not build or run tests, and vcpkg ports switch tests off. | The test suite gets in-repo configurations (D-022, Q-08). |
| F-12 | **Default branch.** The weekly run, manual runs, Dependabot and PR templates work only from the default branch, `main`. A skipped job counts as a passing required check. | D-028 and Q-21; one gate check (D-013). |
| F-13 | **MSVC toolsets.** Every x64 toolset from 14.29 to 14.44 can be installed side by side during a CI job. | Every toolset is tested (D-009, Q-06). |

## 3. Working agreement

### 3.1 How every task runs

1. **Plan.** The task's plan is already written in this file and was approved when you merged the
   previous PR.
2. **Branch.** Copilot creates a branch from the latest `develop-release_1`.
3. **Implement.** Copilot implements the task in small commits, with tests and documentation.
   From T5 onward, CI runs on the PR's merge commit.
4. **Plan ahead.** The same PR updates this plan (status, plus the **next** task in full detail)
   and the decision log.
5. **Review.** Copilot opens the PR itself as soon as it can meet the task's definition of done
   (§3.2) without further input from you (D-003). You review the code, the docs (including the
   Latin) and the plan for the next task. Then you approve and merge.
6. **Next task.** Only then does the next task start. No two task PRs are open at once; your
   promotion PRs and Dependabot's PRs are outside this rule (D-003).

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
| R-7 | CI enforces every MSVC with (nearly) full C++20 | T7 · D-009 · Q-06 |
| R-8 | The same for Visual Studio's clang-cl | T7 · D-010 |
| R-9 | Cross-platform, standard C++ only | T3, T8 · D-024, D-030 |
| R-10 | Full C++23 suite in CI on GCC, Clang and VS | T5–T8 · D-008 |
| R-11 | Building the test suite is optional for users | T4, T10 · D-018 |
| R-12 | **Separate** vcpkg and Conan configurations for the library and for the test suite | T10, T12, T13 · D-022 · Q-08 |
| R-13 | Hyper-modern style, header-only, no third-party dependencies in the library | All code tasks · D-018, D-020 |
| R-14 | The test project is a separate, buildable project | T10 |
| R-15 | Branch from `develop-release_1`; PRs target it | §3.1 · D-003 (the one exception, promotion into `main`, is yours) |
| R-16 | CI tracks `develop-release_1` and `main`, later every `develop`/`release` branch | T5, T16 · D-014, D-028 · Q-21 |
| R-17 | PRs are tested as merged with their base | T5 · D-013 (one gate check) |
| R-18 | One small PR at a time; approval before the next; tests and docs with every change | §3 · D-003 |
| R-19 | A plan with timeline, milestones, tasks, definitions of done, and a decision log in `Copilot/` | T1 (PR #14) · D-001 |
| R-20 | Plan the following tasks at the end of each task or milestone | §3.4 · D-002 |
| R-21 | "If you require additional information, now is the time to ask" | [QUESTIONS.md](QUESTIONS.md) |

## 5. What the repository looks like at 1.0

Names follow Q-01 (answered) and assume the defaults in Q-02 and Q-15.

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
| Linux, C++20 floor and range | `ubuntu-22.04`, `ubuntu-24.04`, `ubuntu-26.04`, and the `gcc:11.1` container | GCC 11.1, 11.4, 12, 13, 14, 15; Clang 17, 18, 19, 20, 21, 22 (libstdc++) | C++20 smoke with fmt, and without fmt installed; install-and-consume |
| Linux, C++23 suite | `ubuntu-24.04`, `ubuntu-26.04` | GCC 14, 15; Clang 19, 20, 21, 22 | Full suite and demo, Debug and Release |
| Linux, libc++ | `ubuntu-24.04` | Clang 17 with libc++ 17; Clang 20 with libc++ 20 | Smoke on both; suite with libc++ 20 (T3 showed it passes) |
| Sanitizers | `ubuntu-24.04` | GCC 14, Clang 20 | Suite under AddressSanitizer and UndefinedBehaviorSanitizer |
| Linux arm64 (Q-18) | `ubuntu-24.04-arm` | GCC 14 | Full suite (where `char` is unsigned) |
| CMake versions | `ubuntu-24.04` | CMake 3.21 and newest 4.x | Configure, build, install, consume |
| Windows, MSVC | `windows-2022`, `windows-2025-vs2026` | Every toolset: v142 14.29, v143 14.30–14.44, v145 14.5x. Those not preinstalled are added during the job (F-13, Q-06). | Smoke on all; suite and demo on v145, and on 14.44 while they pass |
| Windows, clang-cl | `windows-2022`, `windows-2025-vs2026` | One job per Clang major, 17 to the newest (22 today), each paired with an STL that accepts it: 17 with 14.42, 18 with 14.43, 19+ with 14.44. Also the clang-cl bundled with VS 2022 17.14 and VS 2026. | Smoke on all; suite and demo on VS 2026's clang-cl |
| macOS (Q-07) | `macos-15` | AppleClang | Smoke; suite if feasible |
| Packaging | `ubuntu-24.04`, `windows-2025-vs2026` | — | vcpkg overlay port and `conan create`; test suite built against each package |

A final gate job, `ci-ok`, depends on every job above. It is the only check the rulesets require
(D-013).

### 5.4 Release flow (D-003, D-026, Q-11)

1. Reviewed PRs accumulate on `develop-release_1`.
2. Pre-releases: you push a tag `vX.Y.Z-rc.N` on `develop-release_1`.
3. A release-preparation PR into `develop-release_1` finalises the version and the CHANGELOG.
4. You promote: a PR from `develop-release_1` into `main`, merged with a merge commit. This is the
   only kind of PR into `main`, and only you open it.
5. You push a tag `vX.Y.Z` on `main`.
6. The release workflow creates the GitHub Release, with notes and checksums.
7. Updates to the vcpkg port and the ConanCenter recipe reference that tag.
8. Between releases, you can promote documentation-only changes without a tag, so `main` shows
   current instructions (for example after T21).

## 6. Milestones and tasks

Sizes: **S** is under about 150 changed lines, **M** is about 150–400. Anything larger is split.
"You" marks steps only you can perform.

| Task | Title | Milestone | Size | Depends on | You |
|---|---|---|---|---|---|
| T1 | Development and release plan (PR #14) | M0 | S | — | review |
| T2 | Restore the silently disabled fmt tests | M1 | S | T1 | review |
| T3 | Make comparisons portable to libc++ | M1 | M | T2 | review |
| T4 | CMake hygiene: version range, optional tests and dependencies, presets | M2 | M | T3 | review |
| T5 | CI skeleton and security baseline | M2 | M | T4 | **set up rulesets and Dependabot settings; default branch (Q-21)** |
| T6 | Linux compiler matrix (floors and C++23 suite) | M2 | M | T5 | review |
| T7 | Windows matrix (MSVC and clang-cl) | M2 | M | T6 | review |
| T8 | libc++, sanitizers, macOS | M2 | M | T7 | review |
| T9 | Install and export: package config, version header, consumer test | M3 | M | T8 | review |
| T10 | Standalone test-suite project; move the demo | M3 | M | T9 | review |
| T11 | Deterministic fmt integration | M3 | M | T10 | review |
| T12 | vcpkg: library port and test-suite configuration | M4 | M | T11 | review |
| T13 | Conan 2: library recipe and test-suite configuration | M4 | M | T12 | review |
| T14 | Versioning and CHANGELOG | M5 | S | T13 | review |
| T15 | Release workflow, with a dry run | M5 | M | T14 | **push the rc tag** |
| T16 | Broaden CI triggers to `develop`/`release` branches | M5 | S | T15 | review |
| T17 | Release documentation pass | M5 | M | T16 | review |
| T18 | Release 1.0.0 | M5 | S | T17 | **promote to `main`, push tag** |
| T19 | Submit the port to `microsoft/vcpkg` | M6 | S | T18 | **fork and open PR** |
| T20 | Submit the recipe to ConanCenter | M6 | S | T19 (its PR here) | **sign CLA, open PR** |
| T21 | Post-release follow-up | M6 | S | T19, T20 | review; **promote the docs to `main` (no tag)** |

### M0: Plan

#### T1: Development and release plan (done: PR #14, merged 2026-10-10)
- **Deliverables:** `Copilot/PLAN.md`, `Copilot/DECISIONS.md`, `Copilot/QUESTIONS.md` and
  `Copilot/FINDINGS.md`. No code changes; README and LEGENDUM are untouched (D-004).
- **Done when:** you have reviewed and merged it. Answering the questions now is welcome but not
  required: each is needed only by its "Needed by" task, and answers given during this review are
  recorded in the T2 PR.
- **Outcome.** Right after merging it, you answered Q-01, Q-06, Q-16 and Q-21, accepting the
  defaults. T2 records the answers.

### M1: Correctness fixes found while planning (week 1)

*What you will learn:* how a green test run can hide tests that never compiled, and why "works on
my standard library" is not the same as standard C++.

#### T2: Restore the silently disabled fmt tests (done: PR #15, merged 2026-10-11)
- **Why.** Three `CtSvFmtFormatter` tests never compile, because they check a stale macro name
  (FINDINGS F-4). The fmt integration is effectively untested.
- **Branch:** `copilot/t2-fix-fmt-test-macro`, from `develop-release_1`. The tooling named the
  actual branch `copilot/make-develop-release-1-default-branch`.
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
- **Outcome.**
  - The three tests are restored, and a fourth covers `wchar_t`. The guard is decision D-029:
    `CMakeLists.txt` defines `CPS_CT_STRING_TESTS_EXPECT_FMT` for the test target, and each
    guarded block has an `#elif` that stops the build with `#error`.
  - With fmt 9.1, on GCC 14 and on Clang 20 (libstdc++), in Debug and Release with `-Werror`,
    all **160** tests pass. The four `CtSvFmtFormatter.*` tests are listed and run, and the demo
    exits with 0.
  - The guard was checked by breaking the build on purpose. Hiding fmt from the header, putting
    back the stale `CJM_` name, and renaming the header's macro each stop the build with the
    message. Without the expectation, a build with fmt hidden still compiles.
  - README and LEGENDUM are corrected in two places each. The plan named only Use case 5, but the
    "Optional: {fmt} support" section (LEGENDUM: "`{fmt}` si desideratur") had the same wrong
    claim about include order.
  - The library headers are unchanged.
  - After merging it, you refined the Latin of the two fmt passages yourself (PR #16).

#### T3: Make comparisons portable to libc++ (done: PR #17, merged 2026-10-18)
- **Goal.** Comparisons between `basic_ct_string_view` or `basic_fixed_string` and string
  literals, arrays, pointers, `std::basic_string` and `std::basic_string_view` compile with
  **libc++ 17+**, as well as libstdc++ and the MSVC STL. Existing meaning and `noexcept` stay as
  they are (FINDINGS F-5, D-024). The one intended exception is comparing with `nullptr`, which is
  never valid and is rejected everywhere if you accept Q-22's default.
- **What planning found.**
  - **The cause is four operator templates:** `operator==` and `operator<=>` in
    `basic_ct_string_view` (`ct_string_view.hpp:555` and `:565`) and in `basic_fixed_string`
    (`fixed_string.hpp:352` and `:363`). They accept an operand only if it is nothrow-convertible
    to the string view, and with libc++ arrays and pointers are not (Q-22 explains why).
  - **Correction to the outline.** The outline spoke of "12 uses" of the concept. That counted
    every line that mentions it, definitions included; only the four templates need to change.
    `ctsv_comparators.hpp` already handles libc++ on purpose: its lookup keys use plain
    convertibility, with a conditional `noexcept` (lines 85–103).
  - **What fails today.** The smoke check fails at `tests/cxx20_header_smoke.cpp:50–51`, with
    libc++ 17 and with libc++ 20. In the suite, with libc++ 20, only `cstr_view_tests.cpp` fails:
    10 errors, all comparisons with a literal or an array. The other two test files already
    compile.
  - **`nullptr`.** `view == nullptr` compiles with libstdc++ in C++20 mode (GCC 14, Clang 20), and
    then has undefined behaviour at run time. C++23 and libc++ reject it (Q-22).
  - **Unused concepts.** Four public concepts next to `nothrow_convertible_to` are unused, and one
    is misnamed (Q-23).
- **Prototype.** A throwaway prototype of Q-22 option (a), not committed, showed the approach is
  enough:
  - the smoke check compiles with libc++ 17 and 20;
  - with Clang 20 + libc++ 20, the whole suite builds and passes (160 tests). GoogleTest was
    built against libc++ from Ubuntu's sources in `/usr/src/googletest`;
  - the demo builds and runs with libc++ 20 when fmt is used header-only. The demo links Ubuntu's
    compiled fmt library, which is built for libstdc++ and cannot be linked with libc++.
- **Branch:** `copilot/t3-libcxx-comparisons`, from `develop-release_1`. The tooling named the
  actual branch `copilot/execute-next-phase-plan`.
- **Questions:** Q-22 (the design) and Q-23 (the misnamed concept). Both have defaults, so T3 can
  start without your answers; its PR then asks again. You answered both when T3 started (QUESTIONS).
- **Steps.**
  1. Write the tests first, and see them fail with libc++:
     - A new C++20 source in the smoke target, made only of compile-time checks, so it runs
       wherever the smoke target builds, including toolchains without GoogleTest.
     - Operands: a string literal, a `const` and a non-`const` array, a `const` and a non-`const`
       pointer, `std::basic_string_view`, `std::basic_string` and `basic_fixed_string`.
     - Each is compared with both class templates (and both flavours of view), for all five
       character types, in both operand orders, with `==`, `!=`, `<` and `<=>`.
     - Each comparison is also checked to be `noexcept`.
     - What must not compile is checked too: operands of another character type, and `nullptr`.
     - Run-time tests in the suite cover operands that exist only at run time, such as a
       `std::string` or a `char` buffer filled at run time. GCC 11's `std::string` is not
       `constexpr`, so those cannot be compile-time checks.
  2. Change the four templates as Q-22 decides. Both classes use one shared constraint, so they
     cannot drift apart; it is not added to the public API. Update the operators' doc comments.
  3. Apply Q-23.
  4. Verify, as listed under "Definition of done".
  5. Docs, in README and LEGENDUM together:
     - "Requirements" ("Quae requirantur"): libc++ 17 or later works. Until T8 adds it to CI, it
       is described as verified locally.
     - The comparison example in "The two types at a glance" ("Duo genera breviter"; README around
       line 173, LEGENDUM around line 137): list what a view compares with.
  6. Log the outcomes of Q-22 and Q-23 as decisions. Update this plan: status, and T4 in full.
- **Definition of done.**
  - The smoke target, with the new checks, compiles with `-Werror` on:
    - Clang 17 + libc++ 17 and Clang 20 + libc++ 20;
    - GCC 14, Clang 17 and Clang 20 with libstdc++;
    - GCC 11.1, the floor, in the official `gcc:11.1` container, as in FINDINGS F-2.
  - On GCC 14 and Clang 20 with libstdc++, the full suite (the 160 existing tests plus the new
    ones) and the demo pass, in Debug and Release.
  - With Clang 20 + libc++ 20, the full suite passes, using GoogleTest built against libc++. Any
    problem that comes only from test code is listed for T8 instead.
  - `noexcept` is unchanged for every comparison that compiled before, `nullptr` aside.
  - README and LEGENDUM are updated in parallel.
- **Out of scope.**
  - libc++ and macOS in CI (T8).
  - Building the demo with libc++. It needs fmt built for libc++, or used header-only; T8
    decides.
  - MSVC, which cannot be tested locally. Its STL already accepts these comparisons, and T7 checks
    the result in CI.
- **Outcome.**
  - **Answers.** Q-22: the default, recorded as D-030. Q-23: option (a), as Copilot reads your note,
    recorded as D-031; you confirmed that reading in the review. Q-24: option (a), which you asked
    for in the review, recorded as D-032.
  - **Library.** One non-public constraint, `detail::string_view_comparand` in `fixed_string.hpp`,
    is used by all four templates. It names arrays and pointers of the class's own character type,
    keeps "anything else nothrow-convertible", and rejects `nullptr`. The operators stay
    unconditionally `noexcept`, and their doc comments now list the operands. The misnamed concept
    checks `noexcept` (Q-23). For these, only `fixed_string.hpp` and `ct_string_view.hpp`
    changed, by about 75 lines, most of them doc comments.
  - **Library, Q-24 (D-032).** No comparator, hasher or container lookup takes `nullptr`, nor a
    literal `0`, with any standard library or language version. `ctsv_lookup_key` excludes
    `std::nullptr_t`. `std::hash<basic_ct_string_view>` and each of the 40 container lookups get
    a deleted `std::nullptr_t` overload. Three headers changed, by 68 lines, most of them the
    deleted declarations and their comments.
  - **Codex's review (PR #17).** It found that the 40 concrete `std::nullptr_t` overloads also
    rejected two valid calls: `m.contains({})`, a lookup of the empty key, and a key type that
    converts to `std::nullptr_t` as well as to a string view. The 40 deleted overloads now deduce
    their parameter, constrained to `std::nullptr_t` or an integer type
    (`detail::null_or_integer`), which `{}` cannot deduce. `std::hash` keeps its concrete
    overload, which turns `{}` from undefined behaviour into an error there (D-032, "Revised in
    review"). Also from the review: README and LEGENDUM say that the conversion that must not
    throw is the one from a `const` lvalue, and a compile-time check confirms that such an
    operand compares even when its rvalue conversion is deleted. Tests first: nine new
    container assertions failed at 32af580, and the new run-time test,
    `CtsvEmptyBraces.LookUpTheEmptyKeyInEveryWrapper`, did not compile: 175 tests became 176.
    The review also suggested keeping the by-hand diagnostic check as a script; Q-27 asks how,
    and when.
  - **Tests, written first.** `tests/cxx20_comparison_checks.cpp`, in the smoke target, covers
    every operand kind in step 1, for both classes, both flavours of view and all five character
    types, in both orders, with `==`, `!=`, `<` and `<=>`. It checks `noexcept`, and that other
    character types (including `signed char` and `unsigned char`), `nullptr`, and operands whose
    conversion from a `const` lvalue may throw do not compile. On the old headers it failed with
    libc++ at every literal, array and pointer check, and with every library at the Q-23 and
    throwing-conversion assertions (74 errors in all with libc++). With libstdc++ in C++20 mode it
    also failed at the `nullptr` assertions. The suite gained 15 typed tests,
    `RuntimeOperandComparison/*`, for operands that exist only at run time: 160 tests became 175.
  - **Tests for Q-24, also written first.** The same file checks every comparator and hasher, and
    every lookup member of the four containers, `const` and not, for all five character types. Each
    check confirms that `nullptr` and a literal `0` are rejected, and that the keys that worked
    before still work. On the headers before Q-24, 17 of the 19 new assertions failed in C++20 mode
    with GCC 14, Clang 20 + libc++ 20 and Clang 17 + libc++ 17; in C++23 mode, the five that
    include `std::hash` failed. The run-time suite is unchanged: a lookup with `nullptr` cannot be
    written there any more.
  - **Verified**, all with `-Werror`:
    - smoke target: GCC 11.1 (official container), 12, 13 and 14; Clang 17, 18 and 20 with
      libstdc++; Clang 17 + libc++ 17; Clang 20 + libc++ 20. The new checks also pass in C++23
      mode;
    - suite and demo: GCC 14 and Clang 20 with libstdc++, Debug and Release: 175 tests pass, and
      the demo exits with 0;
    - Clang 20 + libc++ 20: all 175 tests pass, with GoogleTest built against libc++ from
      `/usr/src/googletest`; the demo builds and runs with fmt used header-only. No test-code
      problem was found for T8;
    - `noexcept` is unchanged for every comparison that compiled before, `nullptr` aside: the
      compile-time checks assert it for every operand kind;
    - after Q-24, all of the above again: the smoke target on every toolchain listed, in C++20 and
      C++23 mode, and the suite and demo on GCC 14 and Clang 20 (Debug and Release) and on
      Clang 20 + libc++ 20. All 175 tests pass, and the demo exits with 0;
    - each of the 40 container lookups, `const` or not, called with `nullptr` and with `0`,
      reports a deleted function, never an ambiguity, with GCC 14, Clang 20 and Clang 20 +
      libc++ 20. The compile-time checks cannot see this difference (D-032);
    - after Codex's review, all of the above again, with 176 tests; and all 104 null lookups
      (every member, on a `const` container and a non-const one, with `nullptr` and with `0`)
      report a deleted function (D-032).
  - **Docs.** README and LEGENDUM, in parallel: "The two types at a glance" ("Duo genera
    breviter") now lists what the types compare with; "Language standard" ("Norma linguae")
    describes the smoke target's second translation unit; "Requirements" ("Quae requirantur")
    says libc++ 17 or later works, verified locally, and that libc++ 17 and 18 leave out
    `std::formatter`. For Q-24: "The fix: wrapper containers" ("Remedium: receptacula") says that
    `nullptr` and a literal `0` do not compile as lookup keys; "Comparators and hashers"
    ("Comparatores et hashes") gains a fourth thing worth knowing, "`nullptr` is never a key";
    the smoke-target paragraph in "Language standard" mentions the new checks. The container
    header's opening comment says the same.
  - **Found on the way (F-14).**
    - Ubuntu's libc++ 17 and libc++ 20 packages replace each other, so a local "libc++ 20" result
      can silently come from libc++ 17. T3's results were checked against `_LIBCPP_VERSION`.
    - libc++ 17 and 18 do not define `__cpp_lib_format`, so the library's `std::formatter` and
      its seven tests silently drop out there (Q-25).
    - The comparators and container lookups still accept `nullptr` in C++20 mode, because they
      take `std::basic_string_view` (Q-24). You asked for it to be fixed in T3 (D-032).
    - While doing so: `std::hash<basic_ct_string_view>` took `nullptr` in C++23 mode too (fixed,
      D-032), and the view's members that mirror `std::basic_string_view` take it in every
      language version, as the standard type does (Q-26).
  - **Deviation from the plan.** The plan put the Requirements change and the comparison list in
    the docs. The smoke-target paragraph in "Language standard" also needed updating, since it
    described a single translation unit. Q-24, planned for T8, was done in T3 at your request.
- **M1 checkpoint:** re-plan (§3.4). Done: the M1 retrospective is in §10.

### M2: Build foundation and CI (weeks 2–4)

*What you will learn:* CMake presets and options; GitHub Actions workflows, jobs, matrices,
runners and containers; required status checks and rulesets; Dependabot; least-privilege tokens.

#### T4: CMake hygiene (done: PR #19, merged 2026-10-22)
- **Goal.** A consumer using `add_subdirectory` or `FetchContent` gets only the header-only
  library: no fmt, no GoogleTest, and CMake 3.21 or later. The project's own top-level build still
  builds and tests everything, now through options and presets (D-017, D-018, F-6, Q-01, Q-09).
- **What planning found.** A throwaway prototype, not committed, on 2026-10-11:
  - **CMake 3.21 already works.** With only the minimum changed to `3.21...4.1`, CMake 3.21.4
    (from PyPI) configures and builds the whole project with GCC 14, Ubuntu's GoogleTest 1.14 and
    fmt 9.1, and all 175 tests pass. Nothing needs CMake 4 (F-6). The newest features in use are
    `cxx_std_23` (CMake 3.20) and, once added, `PROJECT_IS_TOP_LEVEL` (3.21).
  - **Module scanning (F-6) can be reproduced without uninstalling anything.** With CMake 4.1.3
    and Clang 20, scanning is on. It works when `clang-scan-deps-20` is installed. When
    `CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS` points at a missing file, configuring fails; with
    scanning off, the same build succeeds.
  - **What makes fmt and GoogleTest mandatory today.** `find_package(fmt CONFIG REQUIRED)` runs
    first, the smoke target links `fmt::fmt-header-only`, and `find_package(GTest CONFIG
    REQUIRED)` runs unconditionally (`CMakeLists.txt:13`, `:58`, `:76`).
  - **The docs name the target `cstr_view`**: README "Building & integrating" (the `FetchContent`
    and `add_subdirectory` examples) and "Language standard", and their LEGENDUM counterparts
    ("Aedificatio et coniunctio", "Norma linguae").
- **Branch:** `copilot/t4-cmake-hygiene`, from `develop-release_1`.
- **Questions:** Q-09 (the CMake minimum) is needed. If it is still open when T4 starts, T4 uses
  the default, 3.21, and asks again in its PR. Q-01 (names) is answered.
- **Steps.**
  1. **Version range.** `cmake_minimum_required` takes 3.21 as the minimum and the newest 4.x
     verified as the policy maximum (Q-09).
  2. **Names (Q-01).** The project and the INTERFACE target become `ct_string`, with two alias
     targets: `ct_string::ct_string`, the name T9 exports, and `cstr_view`, so that existing
     `target_link_libraries(... cstr_view)` keeps working. The smoke, test and demo targets keep
     their names until T10 moves the suite and the demo.
  3. **Options.** Three options with the `CT_STRING_` prefix build the C++20 smoke check, the
     C++23 suite and the demo. Each defaults to `PROJECT_IS_TOP_LEVEL`. T4 logs the final names
     as a decision.
  4. **Dependencies, only when needed.**
     - GoogleTest only when the suite is enabled.
     - fmt, required, when the suite or the demo is enabled. The suite keeps D-029's expectation.
     - The smoke check links fmt's header-only target if fmt is found, and builds without it
       otherwise. Until T11, "without fmt" means fmt is not installed: the header uses fmt whenever
       it can find it (F-7).
  5. **Module scanning** is turned off on the project's own targets. The setting is ignored by
     CMake versions before 3.28.
  6. **`CMakePresets.json`**: configure, build and test presets for GCC, Clang, MSVC and clang-cl,
     each in Debug and Release, with Ninja and build directories under `build/` (already ignored by
     Git). It uses a preset schema that CMake 3.21 reads, so the minimum-CMake jobs in T6 can use
     the same presets; T4 records the choice as a decision.
  7. **Docs**, in README and LEGENDUM together:
     - "Building & integrating" ("Aedificatio et coniunctio"): the real URL,
       `https://github.com/cpsusie/ct_string.git`; the target `ct_string::ct_string`, with
       `cstr_view` still accepted; the options; the presets; and CMake 3.21 as the minimum.
     - "Language standard" ("Norma linguae"): the new target name.
  8. **Plan and decisions.** D-017 follows the answer to Q-09. New decisions for the option names
     and the preset schema. This plan: status, and T5 in full.
- **Definition of done.**
  - The default top-level build, with the presets and without them, still builds the smoke check,
    the suite (176 tests, all passing) and the demo, in Debug and Release with `-Werror`, on GCC 14
    and on Clang 20 with libstdc++.
  - A configure with all three options off succeeds where neither fmt nor GoogleTest is installed.
    This is verified in a container that has neither, and the PR shows how.
  - The smoke check alone builds with fmt and without it, and with GCC 11.1, which has no C++23.
  - A scratch consumer using `add_subdirectory` builds against `ct_string::ct_string` and against
    `cstr_view`, in a container with neither dependency; none of the project's options is on
    there. The same holds for `FetchContent` from a local path.
  - CMake 3.21.4 and the newest 4.x from PyPI both configure, build and test the project.
  - With CMake 4.x, Clang 19 and Clang 20 build the project when `clang-scan-deps` is unavailable,
    simulated as above.
  - README and LEGENDUM are updated in parallel.
- **Out of scope.**
  - Install and export rules, the package-config file and the version header (T9).
  - Moving the suite and the demo into their own project (T10).
  - A switch that turns fmt on or off (T11).
  - CI (T5).
- **Outcome.**
  - **Answer.** Q-09: the default, CMake 3.21, which you accepted when T4 started (D-017).
  - **Branch.** `copilot/ct-string-next-phase`, the name this Copilot session was given, instead of
    `copilot/t4-cmake-hygiene`.
  - **`CMakeLists.txt`.**
    - `cmake_minimum_required(VERSION 3.21...4.4)`. 4.4.4 is the newest CMake on PyPI (D-017).
    - The project and the INTERFACE target are `ct_string`, with the aliases
      `ct_string::ct_string` and `cstr_view` (Q-01, D-019).
    - `CT_STRING_BUILD_CXX20_CHECKS`, `CT_STRING_BUILD_TESTS` and `CT_STRING_BUILD_EXAMPLES`, each
      defaulting to `PROJECT_IS_TOP_LEVEL` (D-033).
    - fmt is required only by the suite and the demo, and GoogleTest only by the suite. The smoke
      check links fmt when CMake finds it, and a status message says whether it did.
    - With all three options off, the file stops right after defining the library (`return()`).
      The plan did not say how a consumer's build would avoid the warnings target; this is the
      simplest way (D-033).
    - `CMAKE_CXX_SCAN_FOR_MODULES` is off.
  - **`CMakePresets.json`.** Schema 3; `gcc`, `clang`, `msvc` and `clang-cl`, each `-debug` and
    `-release`; matching build and test presets (D-034).
  - **Verified**, all with `-Werror`:
    - the default top-level build, with presets (`gcc-*` and `clang-*`, with
      `-D CMAKE_CXX_COMPILER=g++-14` and `clang++-20`) and without (Unix Makefiles), with GCC 14
      and Clang 20 (libstdc++), Debug and Release: the smoke check builds, all 176 tests pass,
      and the demo exits with 0;
    - CMake 3.21.4 and 4.4.4 (PyPI), GCC 14 and Ninja, with and without presets: the same;
    - in an `ubuntu:24.04` container with neither fmt nor GoogleTest (GCC 13; CMake 3.28.3 and
      3.21.4): all three options off configures and builds, and neither dependency appears in the
      cache; the smoke check alone builds, "without fmt"; the default configure stops, asking for
      fmt, as it should;
    - in the same container, a scratch consumer using `add_subdirectory`, and one using
      `FetchContent` (the README's snippet, with `FETCHCONTENT_SOURCE_DIR_CT_STRING` pointing at
      the checkout), build and run against `ct_string::ct_string` and against `cstr_view`. Every
      option is off there, and none of the project's own targets is built;
    - in the official `gcc:11.1` container with CMake 3.21.4: the smoke check alone builds with
      `-std=c++20`, without fmt and with fmt 9.1; the `add_subdirectory` consumer builds and runs;
    - module scanning, simulated as planned with CMake 4.4.4: before T4, Clang 19 and Clang 20
      fail to configure (F-15 says where); with T4, both configure and build, and all 176 tests
      pass. With `clang-scan-deps` present, the build has no scanning rules any more (39 before);
    - the MSVC and clang-cl presets: on Linux, a scratch copy whose condition names Linux is
      listed and applied by CMake 3.21.4, `architecture` setting included. They have not run on
      Windows; T7 will run them.
  - **Docs.** README and LEGENDUM, in parallel: "Building & integrating" ("Aedificatio et
    coniunctio") has the new target name, CMake 3.21, what a consumer gets, new sections "Build
    options" ("Optiones aedificationis") and "CMake presets" ("Praescripta CMake"), and a note
    on module scanning; "Running the tests" ("Probationes") says when fmt and GoogleTest are
    looked up; "Language standard" ("Norma linguae") has the new target name and the option;
    "Requirements" ("Quae requirantur") gains CMake 3.21.
  - **Deviations from the plan.**
    - The `FetchContent` example names `GIT_TAG develop-release_1`, not `main`: `main` still has
      the old CMake project, which would make every consumer find fmt and GoogleTest. A sentence
      says to use the release tag once there is one. T17 makes that change.
    - "Requirements" was not in the plan's list of docs; it now names the CMake minimum.
  - **Found on the way (F-15).** Where module scanning fails; how presets inherit conditions; the
    demo needs the `en_US.UTF-8` locale and exits with 255 without it (T5 installs it; Q-15 notes
    it); what the `ubuntu-24.04` runner provides, for T5.
  - **After merging.** You reviewed the Latin of the build passages, and how closely README and
    LEGENDUM correspond there (PR #20).

#### T5: CI skeleton and security baseline (in review: PR #21)
- **Goal.** Every push to, and PR into, `develop-release_1` and `main` builds and tests the project
  on Linux with GCC 14, and one check, `ci-ok`, reports the result. The workflow follows the
  security baseline, and Dependabot keeps its action current (D-011 to D-014, D-016, D-028, Q-16,
  Q-21).
- **What planning found.** A throwaway prototype, not committed, on 2026-10-18 (F-15):
  - **The runner has almost everything.** `ubuntu-24.04` has GCC 14.2 as `g++-14` (plain `g++` is
    GCC 13), CMake 3.31.6 and Ninja 1.13.2. It has neither GoogleTest nor fmt; per D-016 they come
    from Ubuntu's packages `libgtest-dev`, `libgmock-dev` and `libfmt-dev`.
  - **T4's presets work unchanged.** In an `ubuntu:24.04` container with those packages,
    `cmake --preset gcc-debug -D CMAKE_CXX_COMPILER=g++-14`, then the build and test presets of
    the same name, pass in Debug and Release.
  - **The demo needs the `en_US.UTF-8` locale.** Without it, the demo exits with 255; after
    installing `locales` and generating the locale, it exits with 0. The job therefore generates
    it, rather than relying on the runner image.
  - **"Without fmt" needs no extra package work:** the runner has no fmt. With the suite and the
    demo turned off, the configure output says the C++20 checks build "without fmt". The test
    preset fails there on purpose, because no test is built (D-034), so that job does not run
    `ctest`.
  - **One action is enough.** Everything except the checkout is a shell step, so the only action
    to pin is `actions/checkout`, v7.0.1 (commit `3d3c42e5…`). A draft workflow along the lines
    below passes `actionlint` 1.7.12, with `shellcheck`.
  - **Repository.** `main` is still the default branch, and the only protected one. No branch has
    a `.github/` directory.
- **Branch:** a new branch from `develop-release_1`, for example `copilot/t5-ci-skeleton`; the
  Copilot session may name it differently.
- **Questions:** Q-14 (a code formatter) is needed. If it is still open, T5 uses the default (no
  formatter before 1.0, so no format check) and asks again in its PR. Q-16 and Q-21 are answered:
  you apply them after merging.
- **Steps.**
  1. **`.github/workflows/ci.yml`, the frame.**
     - Triggers (D-014, phase 1): pushes to, and PRs into, `develop-release_1` and `main`; manual
       runs; a weekly schedule, at an odd minute so it does not start with everyone else's.
     - `permissions: contents: read` for the whole workflow; no job asks for more (D-012).
     - Concurrency: one group per PR or branch. A newer run of a PR cancels the older one; runs
       for pushes and the schedule are never cancelled.
     - Every job: `runs-on: ubuntu-24.04`, a timeout, `actions/checkout` pinned to the commit SHA
       with a version comment and `persist-credentials: false`, and a step that prints the
       versions of `g++-14`, CMake and Ninja (D-011).
  2. **Jobs.**
     - **GCC 14, Debug and Release** (a two-entry matrix, `fail-fast` off). Installs the three
       library packages and `locales`, and generates `en_US.UTF-8`. Configures with the `gcc-*`
       preset and `-D CMAKE_CXX_COMPILER=g++-14`; builds; runs the test preset (176 tests); runs
       the demo. All three options are on by default, so this builds the smoke check, the suite
       and the demo. D-029 makes the build fail if fmt goes missing from the suite.
     - **GCC 14, C++20 checks without fmt.** Installs nothing. First fails if `g++-14` can find
       `<fmt/format.h>`, so that a runner image that one day ships fmt cannot make this job
       meaningless. Configures the `gcc-debug` preset with the suite and the demo off, checks that
       the configure output says "without fmt", and builds.
     - **`ci-ok`.** Needs every other job, always runs, and fails unless every one of them
       succeeded; a skipped or cancelled job counts as a failure (D-013, F-12).
  3. **`.github/dependabot.yml`.** GitHub Actions only, weekly, `target-branch: develop-release_1`,
     with every update in one group, so they come as one PR (D-012, D-028).
  4. **`.github/pull_request_template.md`.** The §3.2 checklist and the §3.3 headings.
  5. **Docs**, in README and LEGENDUM together: a CI status badge for `develop-release_1` under
     the title, and one sentence in "Requirements" ("Quae requirantur") saying what CI checks so
     far (GCC 14 on Linux, with and without fmt); T6 widens it.
  6. **Your settings.** The PR gives click-by-click steps, in this order:
     1. merge the PR, and wait for the first run on `develop-release_1` (a ruleset can only
        require a check that has already run);
     2. make `develop-release_1` the default branch (Q-21, D-028);
     3. in the merge settings, allow squash merges and merge commits, and turn on "Automatically
        delete head branches" (Q-16);
     4. look at what protects `main` today, and decide whether the new ruleset replaces it;
     5. create the two rulesets of Q-16, both requiring `ci-ok`;
     6. Dependabot: alerts on, automatic security-update PRs off (D-012);
     7. start a manual run from the Actions tab, and check Dependabot's status page.
  7. **Plan and decisions.** The job names and the gate, as a decision; this plan: status, and T6
     in full.
- **Definition of done.**
  - `actionlint` passes on the workflow. It is run locally, and not added to the repository.
  - The workflow is green on the PR, which builds the PR's merge commit. The logs show 176 tests
    passing, the demo exiting with 0, and "without fmt".
  - `ci-ok` is shown to turn red: Copilot can push only the PR's own branch, so a temporary commit
    there makes one job fail, and the next commit reverts it. The PR links both runs.
  - README and LEGENDUM are updated in parallel.
  - **After merging**, with your settings applied: the workflow is green on `develop-release_1`; a
    manual run starts from the Actions tab; Dependabot reports a successful check; both rulesets
    require `ci-ok`. T6's PR records these results. Anything that cannot work yet is listed in
    T5's PR, with the reason.
- **Out of scope.**
  - More compilers and both CMake versions (T6); Windows (T7); libc++, sanitizers and macOS (T8).
  - Packaging jobs (T9, T12, T13).
  - Wider triggers (T16).
  - A format check (Q-14).
- **Outcome.**
  - **Answer.** Q-14: the default, no formatter before 1.0, which you accepted when T5 started:
    "defer to 1.0 to maintain legible diffs" (D-036).
  - **Branch.** `copilot/t5-ci-skeleton`.
  - **`.github/workflows/ci.yml`**, as planned (D-035):
    - the triggers of D-014's phase 1, with the weekly run on Mondays at 04:37 UTC;
    - `permissions: contents: read`;
    - the jobs "GCC 14, Debug", "GCC 14, Release" and "GCC 14, C++20 checks without fmt", each
      with a timeout, `actions/checkout` v7.0.1 pinned to its commit without credentials, and a
      step printing the runner image and the versions of `g++-14`, CMake and Ninja;
    - the gate `ci-ok`.
  - **`.github/dependabot.yml`**: GitHub Actions, weekly, into `develop-release_1`, one group.
    **`.github/pull_request_template.md`**: the §3.3 headings and the §3.2 checklist.
  - **Verified.**
    - `actionlint` 1.7.12, with `shellcheck`; `zizmor` 1.31.1; the workflow and Dependabot
      schemas. All pass (F-16).
    - Both jobs, replayed in an `ubuntu:24.04` container with the same packages: 176 tests pass
      in Debug and in Release, the demo exits with 0, and the configure output says "without fmt".
    - On PR #21, run 40583728552 built the merge commit `b2ea263` (the PR merged into
      `develop-release_1`), and was green. Its logs show "[  PASSED  ] 176 tests." in Debug and
      in Release, "test_console_app exited with 0" in both, and "the C++20 checks build without
      fmt".
    - `ci-ok` turned red in run 40584049573, on a temporary commit that installed fmt in the job
      that must not have it. The revert's run, 40584323698, was green (F-16).
  - **Docs.** README and LEGENDUM, in parallel: a CI badge for `develop-release_1` under the
    title, and a sentence in "Requirements" ("Quae requirantur") saying what CI checks.
  - **Deviations from the plan.**
    - **Run grouping.** Runs of a PR share a group, but every other run has its own, rather than
      one group per branch: within one group GitHub cancels a pending run when a newer one
      arrives, even when nothing is set to cancel (D-035, F-16).
    - **Dependabot's 7-day cooldown**, which the plan did not have. `zizmor` recommended it
      (D-035).
    - **`ci-ok` has no checkout and no version step.** It builds nothing, so it needs neither.
    - **Shell and matrix values.** Every `run` step uses `bash` with `pipefail`, and matrix values
      reach scripts through environment variables (D-035).
    - **The order of your settings.** The existing ruleset protects whichever branch is the
      default, not `main` by name (F-16). The steps in PR #21 therefore turn it into the `main`
      ruleset, and create the `develop-release_1` ruleset, before you switch the default branch;
      step 6's order would have left `main` unprotected in between.
  - **What can only be checked after merging**, and why (F-12): the weekly run and the manual
    "Run workflow" button need the workflow on the default branch; Dependabot and the PR template
    read the default branch too; the rulesets are yours to create. T6's PR records the results.
  - **Found on the way (F-16).** How `concurrency` treats pending runs; that an explicit
    `shell: bash` adds `pipefail`; Dependabot's default cooldown; that two jobs of one run can
    get different runner images; the runner software of 2026-10-23.
  - **Planning T6 (F-17).** A prototype of T6's matrix found that Clang 20 and later cannot use
    Ubuntu 26.04's fmt header-only, and that formatting a view needs fmt 8 or later (Q-28).

#### T6: Linux compiler matrix (NEXT TASK, planned in full)
- **Goal.** CI checks on Linux what README promises (D-005 to D-008, D-017):
  - the library builds as C++20 with every GCC from the floor (11.1, Q-04) to 15, and every Clang
    from the floor (17, Q-05) to 22, with fmt and without it;
  - the C++23 suite and the demo pass on GCC 14 and 15 and on Clang 19 to 22, in Debug and
    Release;
  - the oldest and the newest CMake both build the project;
  - one arm64 job runs the suite (Q-18).

  README and LEGENDUM then list exactly what CI checks.
- **What planning found.** A throwaway prototype in containers on 2026-10-27 (F-17), and the
  runner software lists of 2026-10-23 (F-16):
  - **Where each compiler is.**
    - `ubuntu-22.04`: GCC 11.4 and 12.3; Ubuntu's fmt 8.1.1.
    - `ubuntu-24.04`: GCC 13.3 and 14.2, Clang 17 and 18, and Clang 19 from the Ubuntu archive;
      fmt 9.1.0.
    - `ubuntu-26.04`: GCC 15.2, Clang 20, 21 and 22, CMake 4.4.4; fmt 10.1.1.
    - GCC 11.1: only the official `gcc:11.1` image (Debian 11, from 2021). Its `apt` still works,
      but Debian 11's support has ended, so its packages may move. The job therefore takes nothing
      from `apt`: CMake and Ninja come from their release archives, and fmt is built from its
      release archive, all checked against a pinned SHA-256. Node 24, which runs
      `actions/checkout`, runs in the image.
  - **All of them build the C++20 checks**, with fmt and without it. Clang 16 still fails (F-2).
  - **Clang 20, 21 and 22 cannot use Ubuntu 26.04's fmt 10.1.1 header-only**: fmt's own header
    does not compile. fmt 11.1 fixed it. With fmt 12.2.0, the newest, built from its release
    archive, everything passes, so those jobs use it.
  - **The real fmt minimum is 8.0.** Formatting a view does not compile with fmt 6 or 7, and the
    C++20 checks cannot notice, because they never format one (Q-28).
  - **The suite and the demo** pass on GCC 14 and 15 and on Clang 19 to 22: 176 tests, and the
    demo exits with 0. On Clang 18 the demo fails (no `std::expected`), and on Clang 17 the suite
    fails; D-008 promises neither.
  - **CMake.** Kitware's 3.21.0, 3.21.7, 4.4.5 and 4.5.0-rc1, and Ubuntu's 3.22, 3.28 and 4.2,
    all build the project and pass.
  - **arm64** could not be tried here. `ubuntu-24.04-arm` has the same software as `ubuntu-24.04`.
- **Branch:** a new branch from `develop-release_1`, for example `copilot/t6-linux-matrix`.
- **Questions:** Q-04, Q-05, Q-17, Q-18 and Q-28 are needed. If any is still open when T6 starts,
  T6 uses its default and asks again in its PR. The steps below assume the defaults: GCC floor
  11.1; Clang floor 17; the full matrix on every run, with anonymous pulls from Docker Hub; one
  arm64 job; fmt 8.0 or later.
- **Steps.**
  1. **T5's results after merging.** T6's PR, and T5's Outcome, record: the first push run on
     `develop-release_1`; a manual run; Dependabot's status (you look at its page); the rulesets
     and the default branch, read back through GitHub's API; and whether GitHub finds the PR
     template.
  2. **The C++20 checks, 12 jobs**, named "*compiler version*, C++20 checks". Each job prints its
     toolchain; checks that the compiler cannot find `<fmt/format.h>`; builds the checks alone and
     checks that the configure output says "without fmt"; installs fmt; then builds them again in
     a second build directory, and checks that it says "with fmt". They replace T5's job "GCC 14,
     C++20 checks without fmt".

     | Job | Runner (image) | Compiler | fmt |
     |---|---|---|---|
     | GCC 11.1 | `gcc:11.1`, pinned by digest, on `ubuntu-24.04` | `g++` | 8.0.1, built |
     | GCC 11.4, GCC 12.3 | `ubuntu-22.04` | `g++-11`, `g++-12` | Ubuntu's 8.1.1 |
     | GCC 13.3, GCC 14.2 | `ubuntu-24.04` | `g++-13`, `g++-14` | Ubuntu's 9.1.0 |
     | Clang 17, 18 and 19 | `ubuntu-24.04` | `clang++-17` to `-19` (19 from the archive) | Ubuntu's 9.1.0 |
     | GCC 15.2 | `ubuntu-26.04` | `g++-15` | Ubuntu's 10.1.1 |
     | Clang 20, 21 and 22 | `ubuntu-26.04` | `clang++-20` to `-22` | 12.2.0, built |

     The GCC 11.1 job uses CMake 3.21.0 and Ninja 1.13.2 from their release archives, so the
     floor compiler is checked with the oldest CMake too.
  3. **The suite and the demo, 14 jobs**, named "*compiler*, Debug" and "*compiler*, Release", as
     T5's GCC 14 jobs are:
     - GCC 14 and Clang 19 on `ubuntu-24.04`; GCC 15 and Clang 20, 21 and 22 on `ubuntu-26.04`,
       with fmt 12.2.0 for the Clangs; each in Debug and in Release;
     - "GCC 14, Debug, CMake 3.21.0", with Kitware's archive, on `ubuntu-24.04`;
     - "GCC 14, Debug, arm64", on `ubuntu-24.04-arm` (Q-18).

     The `ubuntu-26.04` jobs use the runner's CMake, the newest of all the runners (4.4.4 today),
     so the newest CMake is checked with no extra job. Every job logs its versions.
  4. **`.github/scripts/install-fmt.sh`**: downloads one fmt release archive, checks it against
     the SHA-256 that the workflow pins beside the version, builds it with the job's compiler, and
     installs it. The GCC 11.1 job and the CMake 3.21.0 job get CMake (and Ninja) the same way.
  5. **The C++20 checks format a view** whenever fmt is found: a narrow one, and a wide one when
     `<fmt/xchar.h>` is there. About ten lines in `tests/cxx20_header_smoke.cpp` (Q-28).
  6. **`ci-ok` checks its own list** (K-14). It checks out only `.github/workflows/ci.yml`, lists
     the workflow's jobs with `yq` (on every runner), and fails if one is missing from `needs`.
  7. **Docs**, in README and LEGENDUM together. In "Requirements" ("Quae requirantur"): a table of
     the Linux toolchains CI checks, with what each one runs, replacing T5's CI sentence; and the
     fmt notes: 8.0 or later to format a view, and, with Clang 20 or later, 11.1 or later when fmt
     is used header-only. The Windows lines stay until T7.
  8. **Decisions and plan.**
     - D-006 and D-007 follow Q-04 and Q-05; the fmt minimum follows Q-28.
     - D-015 and D-016 gain the exception: where neither the runner nor the Ubuntu archive has a
       usable tool or library, a release archive, pinned by SHA-256 (CMake, Ninja, fmt).
     - D-017: the oldest CMake checked is 3.21.0. D-035: `ci-ok` checks out `ci.yml`.
     - This plan: status, and T7 in full.
- **Definition of done.**
  - `actionlint` and `zizmor` pass, run locally.
  - All jobs are green on the PR. The logs show each job's toolchain and fmt version, and "with
    fmt" and "without fmt" in each C++20 job.
  - The README table matches the matrix.
  - The floor jobs and the self-check are shown to turn red: one temporary commit adds a call to
    `std::ranges::to`, which C++20 does not have, to a header, and drops one job from `ci-ok`'s
    `needs`; the next commit reverts it. The PR links both runs.
  - The wall time of a full run is recorded (Q-17).
  - README and LEGENDUM are updated in parallel.
- **Out of scope.**
  - Windows (T7); libc++, sanitizers and macOS (T8).
  - Making the header ignore an fmt older than 8 (T11, Q-28).
  - Caching fmt builds, unless they make runs too slow.

#### T7: Windows matrix
- **Scope.**
  - The test suite's vcpkg manifest (GoogleTest and fmt, pinned baseline), which T12 extends.
  - MSVC: every toolset from 14.29 to the newest (D-009, Q-06). Toolsets that are not preinstalled
    are installed side by side during the job (F-13). Each job confirms that the requested toolset
    is the one compiling.
  - clang-cl (D-010):
    - one job per Clang major from 17 to the newest (22 today), each pinned to its major and paired
      with an STL that accepts it: 17 with 14.42, 18 with 14.43, 19 and later with 14.44. Majors
      not on the runner come from official LLVM releases (D-015);
    - the clang-cl bundled with VS 2022 17.14 and with VS 2026.
  - The smoke check runs on every one of these. The C++23 suite and the demo run on VS 2026 (MSVC
    and its bundled clang-cl), and on VS 2022 17.14's MSVC while they pass there (D-008, Q-20).
  - Each job prints the toolsets it finds.
  - vcpkg binary caching.
  - README and LEGENDUM get a Windows toolchain table.
- **Done when:**
  - all jobs are green;
  - the table matches the matrix;
  - the time each toolset installation adds is recorded. If the sweep makes PR runs much slower,
    the PR offers Q-06 option (b) for your decision;
  - any toolset that needs a workaround, or is dropped, is recorded as a decision.

#### T8: libc++, sanitizers and macOS
- **Scope.**
  - libc++ jobs: Clang 17 with libc++ 17 (floor), and Clang 20 with libc++ 20.
    - Ubuntu's GoogleTest and fmt libraries are built for libstdc++. These jobs build GoogleTest
      against libc++, and use fmt header-only or build it too (found while planning T3).
    - Ubuntu's libc++ 17 and 20 packages replace each other, so each job installs one and logs
      `_LIBCPP_VERSION` (F-14).
  - `std::format` per Q-25. By default: the build states where it expects `std::format`, so the
    `CtSvFormatter.*` tests cannot silently vanish there (D-029, F-14).
  - `nullptr` in the view's members that mirror `std::basic_string_view`, per Q-26. (Q-24, for
    the comparators and container lookups, was done in T3: D-032.)
  - A compile-fail check that the deleted `nullptr` overloads are the ones chosen, per Q-27.
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
  - Move the demo to `examples/` (Q-15). CI keeps building and running it, including a Debug build
    so its `assert`s are active.
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
  - Dry run (Q-03): after this PR merges, **you** push the tag `v1.0.0-rc.1` on
    `develop-release_1`, and the workflow publishes a pre-release (D-026).
- **Done when:** the pre-release exists, with correct notes and checksums. If the dry run fails, a
  follow-up PR fixes it before T16 starts.

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
    - versioning;
    - the `FetchContent` example names the release tag instead of `develop-release_1` (T4);
    - the title of both documents, still `cstr_view`, against the project's name, `ct_string`.
  - `CONTRIBUTING.md`, `SECURITY.md` and issue templates (Q-19).
- **Done when:** the install instructions are covered by CI jobs (T9, T12, T13), and you have
  approved the docs, including the Latin.

#### T18: Release 1.0.0
- **Steps.**
  1. Copilot's PR into `develop-release_1` finalises the CHANGELOG and version, and gives you
     click-by-click steps for the rest.
  2. **You:** merge it.
  3. **You:** open the promotion PR from `develop-release_1` into `main`. Merge it with a merge
     commit once `ci-ok` is green (D-003).
  4. **You:** push the annotated tag `v1.0.0` on `main`. The workflow publishes the release.
  5. **You:** make `main` the default branch again (Q-21, D-028).
- **Done when:** the GitHub Release `v1.0.0` exists, with notes and checksums, and `main` contains
  exactly what was reviewed on `develop-release_1`. The consumption routes are checked against the
  published release in T19 and T20.
- **M5 checkpoint:** re-plan.

### M6: Publication and maintenance (week 9 onwards; external review times vary)

*What you will learn:* contributing to large open-source registries and working with their
reviewers.

The upstream PRs live in other repositories, so the "one PR at a time" rule does not apply to them
(D-027). Copilot's preparation for each is a normal task PR here.

#### T19: Submit the vcpkg port
- **Copilot prepares**, in a PR into `develop-release_1`:
  - the port pinned to `v1.0.0` (with SHA-512) and the version database entries;
  - CI checks that `FetchContent` by tag and the port both work against the published release;
  - a checklist against the maintainer guide, and step-by-step instructions.
- **You** fork `microsoft/vcpkg`, open a draft PR, and answer reviewers.
- **Done when:** the upstream PR is merged, or you decide to close the task (D-027). Changes the
  reviewers ask for go through normal PRs here.

#### T20: Submit the ConanCenter recipe
- **Starts** once T19's PR here is merged. The two upstream reviews then run in parallel.
- **Copilot prepares**, in a PR into `develop-release_1`:
  - `recipes/ct_string/all/`: `conanfile.py`, `conandata.yml` and `test_package/`;
  - `config.yml`;
  - a local validation run against the published release;
  - step-by-step instructions.
- **You** sign the CLA, open the PR, and answer reviewers.
- **Done when:** the upstream PR is merged, or you decide to close the task (D-027). Changes the
  reviewers ask for go through normal PRs here.

#### T21: Post-release follow-up
- **Scope.**
  - README and LEGENDUM install instructions point at the official registries, with badges.
  - Document the maintenance routine: Dependabot PRs, the weekly CI run, and how to cut a patch
    release.
- **You:** after merging, promote `develop-release_1` into `main` without a tag, so `main` shows the
  new instructions (D-026).
- **Done when:** the docs are updated and promoted to `main`, and the plan is closed out or the
  next cycle is planned.

## 7. Timeline

The timeline is **relative and review-gated**: week 1 started when T1 was merged (2026-10-10). It
assumes about two PRs per week, reviews within one to three days, and no major surprises from the
older MSVC toolsets, libc++ or macOS. A late review moves everything after it; the **order**
matters more than the dates.

| Week | Tasks | Checkpoint |
|---|---|---|
| 1 | T2, T3 | M1 re-plan |
| 2 | T4, T5 (plus your rulesets and default-branch setting) | — |
| 3 | T6, T7 | — |
| 4 | T8, T9 | M2 re-plan |
| 5 | T10, T11 | M3 re-plan |
| 6 | T12, T13 | M4 re-plan |
| 7 | T14, T15 (plus your rc tag), T16 | — |
| 8 | T17, T18 (your promotion and tag) → **v1.0.0 released** | M5 re-plan |
| 9+ | T19, T20 (registry reviews take days to weeks), T21 (plus your documentation promotion) | M6 close-out |

**Progress (2026-10-27).** T4 was merged in week 2. T5 is in review in week 3, the week planned
for T6 and T7, so the plan runs about one week behind. The M2 checkpoint, after T8, re-baselines
the dates.

## 8. Risks and mitigations

Risk IDs use K-n, to avoid clashing with the requirement IDs (R-n) in §4.

| # | Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|---|
| K-1 | An older MSVC toolset or a clang-cl rejects valid C++20 | Medium | Medium | Probe each one in T7. You decide between a workaround and dropping it (Q-06). |
| K-2 | The test code has more libc++ differences | Low: in T3 the whole suite passed with libc++ 20, unchanged | Low–Medium | T3 is limited to the library; test-code issues go to T8. The one difference found, the `std::format` tests with libc++ 17 and 18, is Q-25. |
| K-3 | Runner images change (compilers removed, `windows-2022` retired) | High over a year | Medium | Explicit labels, a weekly run (Q-21), toolchain versions and the image version logged in every job; small PRs to adjust the matrix. A new image reaches runners gradually, so two jobs of one run can differ (F-16). |
| K-4 | Docker Hub rate-limits `gcc:11.1` pulls | Low–Medium | Low | An access-token secret or a mirror (Q-17). |
| K-5 | Registry reviewers ask for changes (naming, layout) | Medium | Medium | Follow their guides from the start (F-10). The in-repo port and recipe work in the meantime. |
| K-6 | Review bandwidth, with one PR at a time | Medium | Schedule only | Small PRs; a relative timeline. |
| K-7 | fmt API changes between versions | Low | Low | CI covers several fmt versions (D-016). |
| K-8 | CMake policy changes | Low | Low | The minimum and newest CMake are both tested. |
| K-9 | The quality of the Latin in LEGENDUM | Medium | Low | You review the Latin in each PR; Copilot keeps the existing register and vocabulary. |
| K-10 | Scope creep (modules, more platforms) | Medium | Medium | Backlog (§11) and the decision log. |
| K-11 | Installing older MSVC toolsets makes PR runs slow, or one is withdrawn from the installer | Medium | Low–Medium | T7 measures the cost, and Q-06 option (b) moves the full sweep to after merging. A toolset that cannot be installed fails its job visibly; a small PR then updates the matrix and the docs. |
| K-12 | Features that read the default branch silently do nothing (weekly run, Dependabot, templates) | Low, once you switch the default branch after T5 (Q-21) | Medium | F-12 and Q-21. T5 checks each feature after merging. |
| K-13 | `main` changes outside promotions, so a promotion conflicts | Low | Medium | Rulesets allow changes to `main` only through PRs; Dependabot's security PRs, which would target `main`, are off (D-012); promotions use a merge commit (D-003). |
| K-14 | A new job is left out of `ci-ok`'s `needs`, so its failure does not block merging | Medium: T6–T8 add dozens of jobs | Medium | T6 adds a step to `ci-ok` that fails when a job of the workflow is missing from its `needs` (D-035). |
| K-15 | The GCC 11.1 job breaks for reasons outside the project: Docker Hub limits pulls, Debian 11's packages move, or a future Node for actions no longer runs on its old C library | Low–Medium | Medium | The job takes nothing from `apt` and pins the image by digest (T6); Q-17 covers the pull limits. If actions stop running in the image, the job runs on the runner and calls `docker run` for the build steps. |

## 9. What only you can do

1. Review and merge every PR.
2. Answer [QUESTIONS.md](QUESTIONS.md), now or by each question's "Needed by" task.
3. **After T5:** following the click-by-click steps in PR #21, create the two branch rulesets, and
   set the merge options and the Dependabot settings (Q-16). Make `develop-release_1` the default
   branch (Q-21).
4. **T6, only if needed:** add a Docker Hub access-token secret (Q-17).
5. **After T15:** push the `v1.0.0-rc.1` tag on `develop-release_1` (the dry run).
6. **T18:** promote `develop-release_1` into `main` (a PR you open, merged with a merge commit),
   push the `v1.0.0` tag on `main`, and make `main` the default branch again (Q-21).
7. **T19 and T20:** fork the registries, open the PRs, sign the ConanCenter CLA, and work with the
   reviewers.
8. **After T21:** promote the updated documentation into `main`, without a tag.

## 10. Status

| Task | Status | PR | Notes |
|---|---|---|---|
| T1 | Merged | #14 | Plan, decisions, questions, findings. You then answered Q-01, Q-06, Q-16 and Q-21. |
| T2 | Merged | #15 | fmt tests restored, with a guard (D-029); the four answers recorded; Q-22 and Q-23 added for T3. You then refined the Latin (#16). |
| T3 | Merged | #17 | Comparisons compile with libc++ 17+ (D-030); misnamed concept fixed (D-031); no comparator, hasher or lookup takes `nullptr` (D-032, Q-24); 176 tests; Q-25, Q-26 and Q-27 added for T8. You then corrected the docs (#18). |
| T4 | Merged | #19 | Q-09 answered: CMake 3.21...4.4 (D-017); `ct_string` target with two aliases; three options (D-033); presets (D-034); module scanning off; F-15. You then reviewed the Latin (#20). |
| T5 | In review | this PR (#21) | Q-14 answered: no formatter before 1.0 (D-036). CI workflow, `ci-ok` gate, Dependabot and PR template (D-035); F-16. You apply Q-16 and Q-21 after merging it. |
| T6 | Next: planned in full | — | Needs Q-04, Q-05, Q-17, Q-18 and Q-28 (defaults: GCC 11.1, Clang 17, anonymous Docker Hub pulls, one arm64 job, fmt 8.0). 26 build jobs, found in a prototype (F-17). Records the results of your T5 settings. |
| T7–T21 | Planned (outline) | — | Each is detailed in the PR of the task before it |

### M0 retrospective (written in T2)

- **What went well.** T1 was merged within a day, after two review rounds. For each of the four
  questions you answered, you accepted the recommended default, so the plan needed no rework.
- **What changed.** Your reviews added the CI gate check, the rulesets, the Dependabot settings,
  clang-cl pinning and the promotion of the docs after the release. Planning T3 then corrected the
  outline: four operator templates need changing, not "12 uses". It also found the `nullptr`
  difference (Q-22) and a misnamed public concept (Q-23).
- **Risks.** K-2 is now Low, since a T3 prototype passed the whole suite with libc++ 20. K-12 is
  Low once you switch the default branch after T5.
- **Timeline.** Unchanged: week 1 started on 2026-10-10, with T2 and T3.

### M1 retrospective (written in T3)

- **What went well.**
  - Both M1 tasks fixed a defect found while planning, and both PRs came with tests that fail
    without the fix: a guard that stops the build (T2, D-029), and compile-time checks that fail
    on the old headers (T3).
  - Planning T3 in full, with a prototype, paid off: the design held, and the four answers needed
    no rework.
  - M1 finished inside week 1, as planned.
- **What changed.**
  - T3 adds a second translation unit to the smoke target. Every toolchain that checks the C++20
    floor now also checks what the two class templates compare with.
  - Verifying T3 found three things that were not planned (F-14): Ubuntu's libc++ packages
    replace each other, libc++ 17 and 18 drop `std::formatter` (Q-25), and the comparators and
    container lookups still take `nullptr` in C++20 mode (Q-24). You asked for Q-24 in T3, which
    applies it (D-032). Doing so found Q-26. Q-25 and Q-26 are needed by T8, so they block nothing
    now.
  - Codex's review of the T3 PR found that the first form of D-032's deleted container overloads
    also rejected `{}`, a valid lookup of the empty key. The tests checked every key that worked
    before, but not `{}`. Lesson for T8 (Q-26) and later: when a deleted overload is added, check
    that `{}`, and class types with more than one conversion, behave as before.
  - Planning T4 with a prototype showed that CMake 3.21 already builds the project as it is. The
    risk in T4 is the option and dependency wiring, not the version.
- **Risks.** K-2 stays Low: the suite passed with libc++ 20 unchanged. No new risk.
- **Timeline.** Unchanged. M2 starts with T4 in week 2.

Later milestone retrospectives will be added here.

## 11. Backlog (after 1.0)

- A C++20 module interface (`import`) alongside the headers.
- An API reference site (Doxygen or similar).
- Measurements of compile-time cost; fuzzing of the case-folding code.
- clang-tidy and coverage reports in CI.
- A `.clang-format` that matches the current style, one PR that reformats everything, and a CI
  format check (Q-14, D-036).
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
- **Ruleset and required check.** Repository rules that block merging until the named CI checks
  pass, and the branch is up to date if required. A skipped job counts as passing, so this plan
  requires a single gate job, `ci-ok`, which fails unless every other job succeeded.
- **Default branch.** The branch GitHub shows first and proposes as the base for new PRs.
  Scheduled runs, the manual run button, Dependabot and templates are read only from it (F-12).
- **Promotion.** Merging the reviewed development branch into `main` for a release. It uses a merge
  commit, so both branches keep one history.
- **Pinning to a SHA.** Referring to a third-party action by commit hash rather than a movable tag.
- **Concurrency group.** Runs that share a group wait for each other. A newer run can cancel an
  older one: CI uses that for the runs of one PR, where only the newest matters (D-035).
- **Cooldown.** How long Dependabot waits after a release before proposing it, so a bad release
  can be withdrawn first.
- **Overlay port.** A vcpkg port kept outside the official registry, used here before submission.
- **Manifest and baseline.** `vcpkg.json` lists the dependencies. The baseline pins the registry
  version, so builds are reproducible.
- **Triplet.** vcpkg's name for a target platform, for example `x64-windows`.
- **Recipe, profile, `test_package`.** In Conan:
  - a recipe (`conanfile.py`) describes how to package the library;
  - a profile describes the compiler and settings;
  - `test_package` is a small consumer that proves the package works.
- **SemVer.** `MAJOR.MINOR.PATCH`: breaking changes bump MAJOR, new features MINOR, fixes PATCH.
