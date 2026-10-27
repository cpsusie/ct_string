# Open questions for @cpsusie

The brief says: *"If you require additional information, now is the time to ask."* These are the
questions.

- **Every question has a recommended default.** Replying **"accept defaults"** in the PR accepts all
  of them. You can override any single question, for example *"Q-04: advertise 11.3"*.
- **Unanswered questions.** The default is a working assumption only. If a question is still open
  when the task in its "Needed by" column starts, Copilot asks again in that task's PR before
  relying on the default.
- **Where answers go.** Copilot records each answer below, in the next task's PR, and updates the
  matching decision in [DECISIONS.md](DECISIONS.md). The answers you gave after T1 was merged
  (Q-01, Q-06, Q-16 and Q-21) are recorded in the T2 PR. Your answers to Q-22 and Q-23, given when
  T3 started, and to Q-24, given in its review, are recorded in the T3 PR.
- **New in T3:** Q-24, Q-25 and Q-26. Q-24 and Q-25 were found while verifying T3; you answered
  Q-24 in the T3 PR, and T3 applies it. Q-26 was found while applying Q-24. Q-25 and Q-26 are needed
  by T8.
- **In T4:** you answered Q-09 when T4 started; T4 applies it (D-017). The next questions needed
  are Q-14, by T5, and then Q-04, Q-05, Q-17 and Q-18, by T6.
- **In T5:** you answered Q-14 when T5 started (D-036). Planning T6 found Q-28. The next questions
  needed are Q-04, Q-05, Q-17, Q-18 and Q-28, all by T6. If they are still open when T6 starts, T6
  uses their defaults and asks again in its PR.

## Summary

| ID | Topic | Needed by | Recommended default | Status |
|---|---|---|---|---|
| Q-01 | Package, CMake and target names | T4 | vcpkg `ct-string`, Conan `ct_string`, CMake `find_package(ct_string)` → `ct_string::ct_string`, `cstr_view` kept as an alias, option prefix `CT_STRING_` | **Answered:** default |
| Q-02 | Include directory, macro prefix, include guards | T9 | Keep `ct_str/`; new macros use `CPS_CT_STRING_`; make include guards consistent | Open |
| Q-03 | Version of the first public release | T14 | `1.0.0`, preceded by a `v1.0.0-rc.1` pre-release that dry-runs the release pipeline | Open |
| Q-04 | GCC floor to advertise | T6 | GCC 11.1 (tested); 10.x is impossible | Open |
| Q-05 | Clang floor | T6 | Clang 17, no workarounds | Open |
| Q-06 | MSVC toolsets to test | T7 | Every toolset from 14.29 (VS 2019 16.11) to the newest, on every PR; you decide before any is dropped | **Answered:** default |
| Q-07 | macOS / AppleClang | T8 | Yes: after the libc++ fix, add a macOS arm64 job | Open |
| Q-08 | Meaning of "separate test-suite packages" | T10 | In-repo vcpkg manifest and Conan recipe for the test suite; not published to the registries | Open |
| Q-09 | CMake minimum | T4 | 3.21 minimum, newest tested 4.x as the policy maximum | **Answered:** default (D-017) |
| Q-10 | fmt opt-in/opt-out | T11 | Keep auto-detection by default; add a force on/off switch; package managers default to off | Open |
| Q-11 | Release and branch flow | T15 | You promote `develop-release_1` into `main` (merge commit) and push tag `vX.Y.Z` there → workflow publishes; pre-release tags go on `develop-release_1` | Open |
| Q-12 | Who submits to vcpkg and ConanCenter | T19 | You, using files and steps Copilot prepares | Open |
| Q-13 | Which new documents need Latin | T14 | Only README and LEGENDUM are bilingual; everything else is English | Open |
| Q-14 | Code formatter (clang-format) | T5 | Not before 1.0; revisit afterwards | **Answered:** default (D-036) |
| Q-15 | Role of `test_console_app` | T10 | A demo that checks itself: move it to `examples/`, behind an option; CI builds and runs it | Open |
| Q-16 | GitHub repository settings | T5 | Two rulesets requiring the one `ci-ok` check: `develop-release_1` (up to date, squash) and `main` (merge commit); delete merged branches; Dependabot security PRs off | **Answered:** default, applied by you after T5 |
| Q-17 | CI budget and Docker Hub | T6 | Full matrix (about 50 jobs) on every PR and push; anonymous pulls of the official `gcc` images | Open |
| Q-18 | CPU architectures | T6 | x64 everywhere, plus one Linux arm64 job (GCC 14, full suite) | Open |
| Q-19 | Community files and documentation hosting | T17 | Add CONTRIBUTING and SECURITY files and issue templates; no separate docs site | Open |
| Q-20 | Visual Studio flags for the C++23 suite | T7 | "/cpplang" means `/std:c++latest`; VS 2026 required, VS 2022 17.14 best-effort | Open |
| Q-21 | Default branch during the project | T5 | Make `develop-release_1` the default branch from T5 until the release, so weekly and manual runs, Dependabot and the PR template work | **Answered:** default (a), switched by you when T5 merges |
| Q-22 | How T3 makes comparisons portable to libc++ | T3 | Accept arrays of, and pointers to, the view's own character type by name; keep `noexcept`; reject `nullptr` everywhere | **Answered:** default (D-030) |
| Q-23 | A misnamed public concept, and three unused ones | T3 | Fix the misnamed one in T3; keep the three unused ones | **Answered:** (a), Copilot's reading of your note, which you confirmed (D-031) |
| Q-24 | `nullptr` in the comparators and container lookups | T8 | Reject it there too, as D-030 does for the comparison operators | **Answered:** (a), applied in T3 at your request (D-032) |
| Q-25 | `std::formatter` with libc++ 17 and 18 | T8 | Keep the standard feature test, and document the gap; T8 makes the `std::format` tests impossible to lose silently | Open |
| Q-26 | `nullptr` in the view's members that mirror `std::basic_string_view` | T8 | Reject it in the 11 overloads where it is always undefined behaviour | Open |
| Q-27 | Checking that the deleted `nullptr` overloads are the ones chosen | T8 | A compile-fail check run by CTest, added in T8 with Q-26 | Open |
| Q-28 | The oldest fmt the library supports | T6 | fmt 8.0; CI formats a view with it; T11 makes the header ignore older fmt | Open |

---

## Details

### Q-01 Canonical names
- **Context.** The repository is `ct_string`, the CMake project and target are `cstr_view`, the
  namespace is `cps::ct_string`, and the include directory is `ct_str/` (FINDINGS F-1).
  - Names must be fixed before packaging: once published, renaming is costly.
  - vcpkg forbids renaming a port within a year of its last rename.
  - vcpkg port names allow only lowercase letters, digits and hyphens.
- **Recommendation:**

| Where | Name |
|---|---|
| vcpkg port | `ct-string` |
| Conan package | `ct_string` |
| CMake package and target | `find_package(ct_string CONFIG)` → `ct_string::ct_string` |
| CMake project name | `ct_string` |
| CMake option prefix | `CT_STRING_` |
| C++ namespace | `cps::ct_string` (unchanged) |

  - Keep `cstr_view` as an alias target so that existing in-tree users keep building.
  - If vcpkg maintainers think `ct-string` is ambiguous, fall back to `cpsusie-ct-string`. Their
    guide explicitly accepts the owner-plus-repository form.
- **Answer (2026-10-11, in reply to T1):** the recommendation is accepted: "package, CMake and
  target names. (accept defaults)". D-019 is now accepted. T4 renames the CMake project and adds
  the `CT_STRING_` options; T9 exports `ct_string::ct_string`.

### Q-02 Include directory, macro prefix and include guards
- **Context.**
  - Public includes are `<ct_str/...>`.
  - The macros use `CPS_CT_STRING_VIEW_...`.
  - Include guards mix the `CPS_` and `CSTR_VIEW_` prefixes.
- **Recommendation:**
  - Keep `ct_str/`. It is short, already documented, and already a unique subdirectory, which vcpkg
    requires.
  - New macros (version, fmt control) use the `CPS_CT_STRING_` prefix.
  - Existing public macros stay as they are.
  - Make the include guards consistent as part of T9. This is cosmetic and can be skipped.
- **Answer:** _pending_

### Q-03 Version of the first public release
- **Context.** CMake already says `1.0.0`, and SemVer promises API stability from 1.0.0 onwards.
- **Recommendation:** Release `1.0.0`. First publish a GitHub pre-release `v1.0.0-rc.1` to dry-run
  the release workflow (T15).
  - If you expect API changes soon, start with `0.9.0` instead.
- **Answer:** _pending_

### Q-04 GCC floor to advertise
- **Context.** You wrote "GCC 11.3+". The probe shows GCC 11.1 works and GCC 10.5 does not
  (FINDINGS F-2).
- **Options:**
  - (a) Advertise and test **11.1**. *(Recommended: the brief says to use the earliest that works.)*
  - (b) Advertise 11.3 and test 11.3. 11.1 then works but is not promised.
- **Answer:** _pending_

### Q-05 Clang floor
- **Context.**
  - With libstdc++, Clang 17 is the earliest that compiles the library unchanged.
  - Clang 16 hits a compiler bug with a constrained `friend` declaration.
  - Clang 15 and earlier need `typename` added back throughout the code (FINDINGS F-2).
- **Options:**
  - (a) Clang 17, no workarounds. *(Recommended.)*
  - (b) Clang 16, with a targeted workaround in `ct_string_view.hpp`.
  - (c) Earlier versions. Not recommended, because it conflicts with the modern style.
- **Answer:** _pending_

### Q-06 Which MSVC toolsets to test
- **Context.**
  - The brief asks CI to enforce every MSVC that fully or nearly fully supports C++20. That is v142
    14.29 (VS 2019 16.11), the first MSVC complete for `/std:c++20`, and every later toolset:
    14.30–14.44 (VS 2022) and 14.5x (VS 2026).
  - The runners preinstall only 14.44 and 14.5x. The other 15 toolsets, including v142, can be
    installed side by side during a job (FINDINGS F-13). That costs some minutes per job, which T7
    measures.
  - VS 2019 is out of mainstream support.
- **Options:**
  - (a) **Recommended.** Test every toolset from 14.29 to the newest, on every PR. If a toolset
    needs non-trivial workarounds, Copilot comes back to you before dropping it.
  - (b) PRs test 14.29, 14.44 and 14.5x. The full sweep runs after every merge and weekly. PR runs
    are faster, but a problem with a toolset in between is found just after merging, not before.
  - (c) VS 2022 (v143) and later, newest version of each Visual Studio only. Cheapest, but it does
    not meet the brief's "any version".
- **Answer (2026-10-11, in reply to T1):** option (a): "Yes, accept defaults". D-009 is now
  accepted. T7 still measures the cost; if the sweep makes PR runs much slower, T7's PR asks you
  again before switching to (b).

### Q-07 macOS / AppleClang in scope?
- **Context.**
  - AppleClang always uses libc++.
  - The libc++ defect (FINDINGS F-5) is fixed in T3 regardless (D-024).
  - macOS runners are free for public repositories.
- **Recommendation:** Yes. Add a macOS arm64 job (`macos-15`): the C++20 smoke, plus the full suite
  if it compiles there. Advertise macOS as supported only once that job is green.
- **Answer:** _pending_

### Q-08 What "the full test suite should be separate vcpkg and conan packages" means
- **Context.** Neither registry is built for test-suite packages (FINDINGS F-10):
  - ConanCenter forbids building or running tests in recipes.
  - vcpkg ports switch tests off.
- **Options:**
  - (a) **In-repo configurations** *(recommended)*:
    - `tests/` becomes a standalone C++23 project.
    - It has its own vcpkg manifest and Conan consumer recipe, which depend on the library package
      plus GoogleTest and fmt.
    - Users who want to validate their toolchain build the suite from this repository against the
      packaged library.
  - (b) Option (a), plus publishing the test suite through a custom vcpkg registry and a private
    Conan remote. This is more infrastructure to run, so it is only worth it if others need to
    consume the suite as a binary package.
  - (c) One package with a "tests" feature or option. ConanCenter forbids it explicitly, and it goes
    against vcpkg's practice of switching tests off.
- **Answer:** _pending_

### Q-09 CMake minimum version
- **Context.**
  - The current minimum is 4.0.2, but nothing needs CMake 4 (FINDINGS F-6).
  - Ubuntu 22.04 ships CMake 3.22 together with GCC 11.4, a typical user of a "GCC 11.3+" library.
  - Was 4.0.2 a deliberate requirement, for example to match your IDE?
- **Recommendation:** Minimum **3.21** (needed for `PROJECT_IS_TOP_LEVEL`), with the newest tested
  4.x as the policy maximum. CMake presets for development may require a newer CMake (≥ 3.25),
  which does not affect consumers.
- **Answer (2026-10-18, when T4 started):** the recommendation is accepted: "I am fine with your
  suggested cmake min version (you suggested 3.21, I think)." T4 applies it as `3.21...4.4`
  (D-017). The presets turned out not to need a newer CMake: they use a schema that CMake 3.21
  reads (D-034).

### Q-10 fmt opt-in and opt-out
- **Context.** If fmt is anywhere on the include path, the header includes it, and there is no
  off-switch (FINDINGS F-7).
- **Options:**
  - (a) **Recommended.** Keep auto-detection as the default, and add a macro to force fmt support
    on or off, with a matching CMake option. The vcpkg feature `fmt` and the Conan option
    `with_fmt` (both off by default) set it explicitly.
  - (b) Make fmt strictly opt-in. This breaks current users who rely on auto-detection.
  - (c) Leave it as it is. This blocks clean packaging.
- **Answer:** _pending_

### Q-11 Release and branch flow
- **Context.** Copilot's PRs always target `develop-release_1` (D-003), but releases come from
  `main`.
- **Recommendation:**
  1. Work accumulates on `develop-release_1`.
  2. A release-preparation PR into `develop-release_1` finalises the version and the CHANGELOG.
  3. You promote: a PR from `develop-release_1` into `main`, merged with a merge commit (not
     squashed), so the two branches keep a shared history. It is the only kind of PR into `main`.
  4. You push an annotated tag `vX.Y.Z` on `main`.
  5. The release workflow builds and verifies the release, then creates the GitHub Release with
     notes and checksums.
  6. Pre-release tags, such as the `v1.0.0-rc.1` dry run (T15), go on `develop-release_1`. They
     need no promotion.
  7. After 1.0.0, the next cycle uses a new branch (for example `develop-release_2`) or keeps
     `develop-release_1`. Your choice. CI picks up either name automatically after T16.
- **Answer:** _pending_

### Q-12 Who submits to the registries
- **Context.** Copilot can push only to this repository.
- **Recommendation:**
  - Copilot prepares the exact files, the version database entries and step-by-step instructions
    (T19, T20).
  - You fork `microsoft/vcpkg` and `conan-io/conan-center-index`, open the PRs (as drafts first, as
    vcpkg recommends), sign the ConanCenter CLA, and answer reviewers.
- **Answer:** _pending_

### Q-13 Which new documents need a Latin counterpart
- **Recommendation.** Only README.md and LEGENDUM.md are bilingual. These stay English only:
  - `Copilot/` documents;
  - CHANGELOG, CONTRIBUTING and SECURITY;
  - workflow comments;
  - PR descriptions.

  Badges and install instructions in README are mirrored in LEGENDUM.
- **Answer:** _pending_

### Q-14 Code formatter
- **Context.** There is no `.clang-format` today. Adopting one means a one-off reformatting PR that
  touches nearly every line.
- **Recommendation.** Do not adopt one before 1.0, to keep diffs reviewable. Revisit afterwards with
  a configuration that matches your current style.
- **Answer (2026-10-27, when T5 started):** the recommendation is accepted: "I agree ... defer to
  1.0 to maintain legible diffs." There is no `.clang-format` and no format check before 1.0
  (D-036). A configuration that matches the current style, with a CI check, is in the backlog
  (PLAN.md §11).

### Q-15 Role of `test_console_app`
- **Context.** It is a C++23 program using `std::expected` and fmt. It demonstrates the library, and
  it also checks itself (FINDINGS F-1):
  - `static_assert`s (`main.cpp` lines 291–316);
  - runtime `assert`s, active only in builds without `NDEBUG` (lines 273–276);
  - a non-zero exit code on failure (lines 325 and 333).
  - It needs the `en_US.UTF-8` locale. Where that locale is not installed, as in a plain
    `ubuntu:24.04` container, it stops at once with exit code 255 (F-15). T5's CI jobs install the
    locale. This question does not change that.
  - Since T4, it has its own option, `CT_STRING_BUILD_EXAMPLES`, separate from the suite's
    (D-033).
- **Options:**
  - (a) Move it to `examples/console_app/`, build it when `CT_STRING_BUILD_EXAMPLES` is on, and have
    the C++23 CI jobs build **and run** it, including a Debug build so its `assert`s are active.
    *(Recommended.)*
  - (b) As (a), but also move its checks into the test suite, so they run wherever the suite runs.
  - (c) Keep it where it is, behind the option T4 added.
  - (d) Remove it. Its checks would be lost unless they move into the suite first.
- **Answer:** _pending_

### Q-16 GitHub repository settings (you apply these; T5 includes click-by-click steps)
- **Recommendation:**
  - Two branch rulesets. Both require a PR and the single status check `ci-ok`, the CI gate job
    (D-013), and both block force pushes and deletion.
    - **`develop-release_1`:** also requires the branch to be up to date before merging. The only
      merge method allowed is squash.
    - **`main`:** the only merge method allowed is a merge commit, for promotions (D-003). It does
      not require up to date: `main` changes only through promotions, so that rule would force
      merging `main` back into `develop-release_1` before every promotion.
  - In the repository's merge settings, allow both squash and merge commits. The rulesets then
    restrict each branch.
  - Turn on "Automatically delete head branches". The deletion block keeps `develop-release_1`
    after a promotion (FINDINGS F-12).
  - Dependabot: alerts on, automatic security-update PRs off (D-012).
  - No merge queue: with one PR at a time it adds nothing over the up-to-date rule.
- **Answer (2026-10-11, in reply to T1):** the recommendation is accepted, to be applied after T5:
  "Yes, after after T5. (accept defaults)". You apply the settings once T5 is merged, using the
  click-by-click steps in T5's PR. The `ci-ok` check they require exists only from T5 on. D-003,
  D-012 and D-013 already assume these settings.

### Q-17 CI budget and Docker Hub
- **Context.**
  - Hosted-runner minutes are free for public repositories.
  - The finished matrix will have an estimated 50 jobs, more than 20 of them on Windows because
    every MSVC toolset is tested (Q-06). GitHub Free runs up to 20 jobs at once, so a full run is
    estimated at 20–30 minutes of wall time. T7 measures it.
  - The GCC floor job pulls the official `gcc:11.1` image from Docker Hub, which rate-limits
    anonymous pulls.
- **Recommendation.**
  - Run the full matrix on every PR and push.
  - Start with anonymous pulls. If rate limits bite, add a Docker Hub access token as a repository
    secret, used only on pushes and scheduled runs, or use a public mirror.
- **Answer:** _pending_

### Q-18 CPU architectures
- **Context.** The library does not depend on the architecture, but plain `char` is unsigned on
  Linux arm64. That is a useful check for the ASCII case-folding code.
- **Recommendation.** x64 on Linux and Windows, plus one `ubuntu-24.04-arm` job running the full
  suite with GCC 14. macOS is arm64 if Q-07 is yes.
- **Answer:** _pending_

### Q-19 Community files and documentation hosting
- **Recommendation.** For 1.0:
  - Add `CONTRIBUTING.md` (build, test and PR rules).
  - Add `SECURITY.md`, using GitHub private vulnerability reporting.
  - Add issue and PR templates.
  - Do not create a separate documentation site: README and LEGENDUM are the documentation.
- **Answer:** _pending_

### Q-20 Visual Studio and the C++23 suite
- **Context.** You wrote that the suite builds with "the latest version of Visual Studio (with
  /cpplang)".
- **Recommendation.**
  - Read that as `/std:c++latest`, which is what CMake uses for C++23 on MSVC.
  - The suite must pass on VS 2026 (MSVC and clang-cl).
  - Run it on VS 2022 17.14 too, for as long as it passes there.
- **Answer:** _pending_

### Q-21 Default branch during the project
- **Context.**
  - `main` is the default branch. GitHub reads some files only from the default branch: the weekly
    scheduled run, the manual "Run workflow" button, Dependabot's configuration, and PR and issue
    templates (FINDINGS F-12).
  - All work lands on `develop-release_1`. Its push and PR runs work as soon as T5 merges, but
    nothing reaches `main` before the release.
- **Options:**
  - (a) **Recommended.** When T5 merges, make `develop-release_1` the default branch, and make
    `main` the default again at the release (T18). Every feature works from T5 on, and GitHub
    proposes `develop-release_1` as the base of new PRs. Until the release, visitors and fresh
    clones see `develop-release_1`.
  - (b) Keep `main` as the default, and promote `develop-release_1` into it early, after T5 and
    again whenever the workflow, Dependabot or template files change. `main` then carries
    unreleased work.
  - (c) Keep `main` as the default, and accept that the features above start only at the release.
    PR and push CI still runs from T5.
- **Answer (2026-10-11, in reply to T1):** option (a): "make develop-release_1 the default branch
  from T5 until the release. (yes please do this)". D-028 is now accepted.
  - Copilot cannot change repository settings: it can only push branches to this repository. So
    **you** make the switch, when T5 is merged. T5's PR gives the steps, and T18's PR gives the
    steps for switching back to `main` at the release.
  - Until T5 is merged, nothing changes: `main` stays the default.

### Q-22 How T3 makes comparisons portable to libc++
- **Context.** With libc++, comparing a view or a fixed string with a string literal, an array or a
  pointer does not compile (FINDINGS F-5).
  - The cause is four operator templates, `operator==` and `operator<=>` in `basic_ct_string_view`
    and in `basic_fixed_string`. They accept only operands that are *nothrow*-convertible to the
    string view.
  - Literals, arrays and pointers convert through `std::basic_string_view`'s constructor from a
    pointer. The standard does not make that constructor `noexcept`, since it has a precondition:
    the string must end in a null. libstdc++ and the MSVC STL add `noexcept` anyway, but libc++
    does not.
  - Of the operand kinds the documentation lists, only these depend on the standard library.
    `std::basic_string`'s conversion is `noexcept` in the standard itself, and the library's own
    types convert with `noexcept`.
  - `nullptr` differs by library too. `view == nullptr` compiles with libstdc++ in C++20 mode, and
    then has undefined behaviour at run time. C++23 (P2166) and libc++ reject it. This was checked
    with GCC 14 and Clang 20.
- **Options:**
  - (a) **Recommended.** Keep today's rule, and also accept arrays of, and pointers to, the view's
    own character type, by name.
    - The operators stay unconditionally `noexcept`, which is accurate: the conversion cannot
      throw.
    - The accepted operands and `noexcept` are then the same on every standard library, once
      `nullptr` is rejected too (below).
    - A throwaway prototype of this option compiled the C++20 smoke check with libc++ 17 and 20.
      With libc++ 20, all 160 tests passed, and so did the demo with fmt used header-only.
      PLAN.md (T3) has the details.
  - (b) Accept anything convertible to the string view, with a `noexcept` that depends on the
    conversion. `ctsv_comparators.hpp` already does this for its lookup keys. It is simpler, but:
    - `noexcept(view == "x")` would be false with libc++ only;
    - types whose conversion can throw would become comparable on every library;
    - `view == nullptr` would also start compiling with libc++ in C++20 mode, unless excluded.
- **Also recommended, with either option:** reject `nullptr` on every library and language
  version, since comparing with it is never valid.
- **Answer (2026-10-11, when T3 started):** the default. In your words: "Accept arrays of, and
  pointers to, the view's own character type by name; keep noexcept; reject nullptr everywhere."
  Recorded as D-030. "Everywhere" is applied to the four operators named in the context above.
  The comparators and container lookups, which also accept `nullptr` in C++20 mode, are Q-24.
  T3 applies Q-24 as well (D-032).

### Q-23 A misnamed public concept, and three unused ones
- **Context.** `fixed_string.hpp` (lines 52–63) defines four concepts in the public namespace
  `cps::ct_string`. Nothing in the library, the tests or the demo uses them, except that one is
  defined in terms of another.
  - One of them, `not_exact_same_nothrow_convertible_to`, does not check `noexcept`, despite its
    name. It is identical to `not_exact_same_convertible_to`.
  - After 1.0, Semantic Versioning makes changing or removing a public name a breaking change.
- **Options:**
  - (a) **Recommended.** In T3, which changes the code right next to them, make the misnamed one
    check `noexcept` as its name says, with a compile-time test. Keep the other three.
  - (b) Remove all four before 1.0.
  - (c) Leave them as they are.
- **Answer (2026-10-11, when T3 started):** you wrote: "Also note that in the nothrow_convertible_to
  concept, presently used is nothrow_convertibe_to". Copilot reads this as pointing at the defect
  above, the `nothrow` concept that does not check `noexcept`, and so as option (a). No identifier
  spelled `convertibe` exists in the repository. T3 applies (a), recorded as D-031. Copilot asked
  you to confirm this reading in the review.
  - **Confirmed (2026-10-11, in the T3 PR):** you answered "yes".

### Q-24 `nullptr` in the comparators and container lookups
- **Context.** D-030 makes the four comparison operators reject `nullptr` on every standard
  library and language version. Two other parts of the library take a `std::basic_string_view`,
  so they follow its rules. In C++20 mode they accept `nullptr`, with libstdc++ and with libc++,
  and the call then has undefined behaviour. C++23 (P2166) rejects it. T3 checked this with GCC 14
  and with Clang 20 + libc++ 20.
  - **Comparators** (`ctsv_comparators.hpp`): `std::nullptr_t` satisfies `ctsv_lookup_key`
    (line 94), so `ctsv_less<char>{}(view, nullptr)` compiles.
  - **Container lookups** (`ctsv_containers.hpp`): `find`, `at`, `at_if`, `contains`, `count`,
    `erase`, `equal_range`, `lower_bound` and `upper_bound` take the key as a string view by value,
    so `set.contains(nullptr)` compiles. There are 40 such declarations across the four wrappers.
- **Options:**
  - (a) **Recommended.** Reject it there too, so no comparison or lookup in the library accepts
    `nullptr`. In the comparators this is one line in the concept. In the containers it is one
    deleted `std::nullptr_t` overload per lookup member. Both get compile-time tests.
  - (b) Reject it in the comparators only, which is one line, and document that container lookups
    follow `std::basic_string_view`.
  - (c) Leave both, and document it. C++23 code is already protected.
- **When.** In T8, which also runs the suite under the sanitizers, unless you would rather have it
  in the T3 PR.
- **Answer (2026-10-11, in the T3 PR):** (a), in T3. In your words: "Why not take care of Q-24 now
  as part of this PR. Reject nullptr comparisons." Recorded as D-032.
  - `std::hash<basic_ct_string_view>` is included. It is a hasher for the same lookups, and it
    accepted `nullptr` in every language version, through its overload for `const char_type*`.
  - A literal `0` is rejected too.
  - The view's own members that mirror `std::basic_string_view` are neither comparators nor
    container lookups. They are Q-26.
  - **Revised in review (2026-10-12).** Codex's review of PR #17 found that the container twins,
    as first written with a concrete `std::nullptr_t` parameter, also rejected `m.contains({})`, a
    valid lookup of the empty key, and keys that convert to `std::nullptr_t` as well as to a string
    view. The twins now deduce their parameter, which fixes both. The answer is unchanged: see
    D-032, "Revised in review".

### Q-25 `std::formatter` with libc++ 17 and 18
- **Context.** `ct_string_view.hpp` provides `std::formatter` only when
  `__cpp_lib_format >= 201907L`, the standard feature test.
  - libc++ 17 and 18 ship `<format>`, but do not define that macro. libc++ 19 and later do. T3
    checked libc++ 17 and 20 locally, and libc++ 18 and 19 in the LLVM sources.
  - So with libc++ 17 or 18, `std::format("{}", view)` does not compile. The suite's seven
    `CtSvFormatter.*` tests are then silently left out, as the fmt tests were in F-4. With Clang
    20 and libc++ 17, 168 of the 175 tests ran, and the run was green.
  - T3 documents the gap in README and LEGENDUM ("Requirements").
- **Options:**
  - (a) **Recommended.** Keep the standard feature test. In T8, apply D-029 to `std::format`: the
    build states where it expects `std::format`, so the tests cannot vanish there. In the planned
    jobs the suite runs with libc++ 20 only, which defines the macro.
  - (b) Also provide `std::formatter` with libc++ 17 and 18, by checking the libc++ version. This
    is a vendor-specific check, against the brief's "rely on standard C++" (D-024), and needs a
    suite job with those versions.
- **Answer:** _pending_

### Q-26 `nullptr` in the view's members that mirror `std::basic_string_view`
- **Context.** `basic_ct_string_view` offers the search and comparison members of
  `std::basic_string_view`, with the same overloads. Several take a `const char_type*`. Found while
  applying Q-24 in T3, and checked with GCC 14 and with Clang 20 + libc++ 20:
  - In 11 overloads, the pointer is a null-terminated string: `compare(s)`,
    `compare(pos1, count1, s)`, `starts_with(s)`, `ends_with(s)`, `contains(s)`, and `find`,
    `rfind`, `find_first_of`, `find_last_of`, `find_first_not_of` and `find_last_not_of` with
    `(s, pos)`. Passing `nullptr` selects that overload, in every language version, C++23
    included, since P2166 deleted only the constructor. The call always has undefined behaviour.
    Exactly the same holds for `std::basic_string_view`. The doc comments already say so.
  - In the other 8 overloads, the pointer comes with a count, as in `find(s, pos, count)`. There,
    `nullptr` with a count of 0 is valid, as it is for `std::basic_string_view`. Their doc
    comments say that a null `s` is always undefined behaviour, which is stricter than the
    standard. Either option corrects that.
  - `basic_fixed_string` has no such members.
- **Options:**
  - (a) **Recommended.** Reject `nullptr` in the 11 overloads, with a deleted `std::nullptr_t`
    overload each and compile-time tests, as D-032 does for the container lookups. Leave the 8
    counted ones as they are. The view then differs from `std::basic_string_view` only by refusing
    calls that can never be valid. A pointer variable that happens to be null is still undefined
    behaviour at run time, as with the standard type.
  - (b) Keep mirroring `std::basic_string_view` exactly, and leave the documentation as it is.
- **Note (2026-10-12, from the review of PR #17).** D-032's container twins became deduced
  templates, because a concrete `std::nullptr_t` parameter also caught `{}`, which there was a
  valid lookup of the empty key. Here, as for `std::hash`, the concrete form is the right one.
  `v.compare({})` and `v.compare(pos1, count1, {})` compile today, and select the overload for
  `const char_type*` with a null pointer, which is undefined behaviour; a concrete `std::nullptr_t`
  overload makes them ambiguous instead. For the other nine, `{}` is already ambiguous. Checked
  with GCC 14.
- **When.** In T8, with Q-25 and the sanitizers, unless you would rather have it in the T3 PR.
- **Answer:** _pending_

### Q-27 Checking that the deleted `nullptr` overloads are the ones chosen
- **Context.** Codex's review of PR #17 suggested keeping, as a script, the check that every
  container lookup called with `nullptr` or `0` names a deleted overload rather than being
  ambiguous. The compile-time checks cannot see that difference (D-032). The check compiles each
  call in its own translation unit and reads the diagnostic: 104 calls today. T3 ran it by hand,
  with a throwaway script, on GCC 14, Clang 20 and Clang 20 + libc++ 20.
- **Options:**
  - (a) **Recommended.** A compile-fail check run by CTest, added in T8. Each call must fail to
    compile, and its diagnostic must name a deleted function and no ambiguity. The wording differs
    by compiler (MSVC reports C2280 and C2668), so the check carries a pattern per compiler, and
    the CI matrix runs it. T8 is where Q-26 adds 11 deleted overloads to the view, which need the
    same check, so the mechanism is built once.
  - (b) The same check, added to the T3 PR now.
  - (c) Commit the script now, under a `tools/` directory, to be run by hand. Nothing would run it,
    so it could break unnoticed.
  - (d) Neither: the compile-time checks, and the manual run recorded in D-032, are enough.
- **When.** T8, unless you want it in the T3 PR.
- **Answer:** _pending_

### Q-28 The oldest fmt the library supports
- **Context** (FINDINGS F-17, found while planning T6).
  - The header turns on its fmt support whenever it can include `<fmt/format.h>` (F-7).
  - With fmt 6 or 7, a program that formats a view does not compile: the library's formatter
    calls a function that fmt made `const` only in version 8. fmt 8.0.1 and every later version
    tested work.
  - fmt before 8 has no `<fmt/xchar.h>`, so wide formatting is silently left out.
  - Nobody noticed because the C++20 checks never format a view, and the suite in CI uses fmt
    9.1.
  - Separately, fmt 10.1.1, 10.2.1 and 11.0.2 in header-only mode do not compile with Clang 20 or
    later; that is fmt's own header, whatever the library does, and fmt 11.1 fixed it. T6
    documents it.
- **Options:**
  - (a) **Support fmt 8.0 and later.** T6 makes the C++20 checks format a narrow and a wide view
    whenever fmt is found, so CI catches a break; the GCC 11.1 job uses fmt 8.0.1, the oldest
    that works; README and LEGENDUM state the minimum. T11 then makes the header ignore an older
    fmt, as if it were not installed, instead of failing once a view is formatted. *(Recommended:
    fmt 8.0 came out in June 2021, the same year as GCC 11.1, and Ubuntu 22.04 ships 8.1.)*
  - (b) As (a), but T11 makes the header stop with an `#error` when it finds an older fmt. That
    breaks programs that have fmt 7 installed and never format a view.
  - (c) Support fmt 7 as well, with a workaround in the formatter for the missing `const`.
- **Answer:** _pending_
