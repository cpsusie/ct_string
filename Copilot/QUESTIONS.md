# Open questions for @cpsusie

The brief says: *"If you require additional information, now is the time to ask."* These are the
questions.

- **Every question has a recommended default.** Replying **"accept defaults"** in the PR accepts all
  of them. You can override any single question, for example *"Q-04: advertise 11.3"*.
- **Unanswered questions.** The default is a working assumption only. If a question is still open
  when the task in its "Needed by" column starts, Copilot asks again in that task's PR before
  relying on the default.
- **Where answers go.** Copilot records each answer below and updates the matching decision in
  [DECISIONS.md](DECISIONS.md). Answers given while you review this PR (T1) are recorded in the T2
  PR, so T1 can be merged while questions are still open.
- **None of these blocks the next task (T2).**

## Summary

| ID | Topic | Needed by | Recommended default |
|---|---|---|---|
| Q-01 | Package, CMake and target names | T4 | vcpkg `ct-string`, Conan `ct_string`, CMake `find_package(ct_string)` → `ct_string::ct_string`, `cstr_view` kept as an alias, option prefix `CT_STRING_` |
| Q-02 | Include directory, macro prefix, include guards | T9 | Keep `ct_str/`; new macros use `CPS_CT_STRING_`; make include guards consistent |
| Q-03 | Version of the first public release | T14 | `1.0.0`, preceded by a `v1.0.0-rc.1` pre-release that dry-runs the release pipeline |
| Q-04 | GCC floor to advertise | T6 | GCC 11.1 (tested); 10.x is impossible |
| Q-05 | Clang floor | T6 | Clang 17, no workarounds |
| Q-06 | MSVC toolsets to test | T7 | Every toolset from 14.29 (VS 2019 16.11) to the newest, on every PR; you decide before any is dropped |
| Q-07 | macOS / AppleClang | T8 | Yes: after the libc++ fix, add a macOS arm64 job |
| Q-08 | Meaning of "separate test-suite packages" | T10 | In-repo vcpkg manifest and Conan recipe for the test suite; not published to the registries |
| Q-09 | CMake minimum | T4 | 3.21 minimum, newest tested 4.x as the policy maximum |
| Q-10 | fmt opt-in/opt-out | T11 | Keep auto-detection by default; add a force on/off switch; package managers default to off |
| Q-11 | Release and branch flow | T15 | You promote `develop-release_1` into `main` (merge commit) and push tag `vX.Y.Z` there → workflow publishes; pre-release tags go on `develop-release_1` |
| Q-12 | Who submits to vcpkg and ConanCenter | T19 | You, using files and steps Copilot prepares |
| Q-13 | Which new documents need Latin | T14 | Only README and LEGENDUM are bilingual; everything else is English |
| Q-14 | Code formatter (clang-format) | T5 | Not before 1.0; revisit afterwards |
| Q-15 | Role of `test_console_app` | T10 | A demo that checks itself: move it to `examples/`, behind an option; CI builds and runs it |
| Q-16 | GitHub repository settings | T5 | Rulesets on `develop-release_1` and `main` requiring the one `ci-ok` check; squash for task PRs, merge commit for promotions; delete merged branches |
| Q-17 | CI budget and Docker Hub | T6 | Full matrix (about 50 jobs) on every PR and push; anonymous pulls of the official `gcc` images |
| Q-18 | CPU architectures | T6 | x64 everywhere, plus one Linux arm64 job (GCC 14, full suite) |
| Q-19 | Community files and documentation hosting | T17 | Add CONTRIBUTING and SECURITY files and issue templates; no separate docs site |
| Q-20 | Visual Studio flags for the C++23 suite | T7 | "/cpplang" means `/std:c++latest`; VS 2026 required, VS 2022 17.14 best-effort |
| Q-21 | Default branch during the project | T5 | Make `develop-release_1` the default branch from T5 until the release, so weekly and manual runs, Dependabot and the PR template work |

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
- **Answer:** _pending_

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
- **Answer:** _pending_

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
- **Answer:** _pending_

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
- **Answer:** _pending_

### Q-15 Role of `test_console_app`
- **Context.** It is a C++23 program using `std::expected` and fmt. It demonstrates the library, and
  it also checks itself (FINDINGS F-1):
  - `static_assert`s (`main.cpp` lines 291–316);
  - runtime `assert`s, active only in builds without `NDEBUG` (lines 273–276);
  - a non-zero exit code on failure (lines 325 and 333).
- **Options:**
  - (a) Move it to `examples/console_app/`, build it when `CT_STRING_BUILD_EXAMPLES` is on, and have
    the C++23 CI jobs build **and run** it, including a Debug build so its `assert`s are active.
    *(Recommended.)*
  - (b) As (a), but also move its checks into the test suite, so they run wherever the suite runs.
  - (c) Keep it where it is, built together with the tests.
  - (d) Remove it. Its checks would be lost unless they move into the suite first.
- **Answer:** _pending_

### Q-16 GitHub repository settings (you apply these; T5 includes click-by-click steps)
- **Recommendation:**
  - A branch ruleset on `develop-release_1` and `main` that:
    - requires a PR;
    - requires the single status check `ci-ok`, the CI gate job (D-013);
    - requires branches to be up to date;
    - blocks force pushes and deletion.
  - Merge methods: squash for task PRs into `develop-release_1`; a merge commit for promotions into
    `main` (D-003). A ruleset can enforce each.
  - Turn on "Automatically delete head branches". The deletion block keeps `develop-release_1`
    after a promotion (FINDINGS F-12).
  - Use the merge queue if GitHub offers it for this repository.
- **Answer:** _pending_

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
- **Answer:** _pending_
