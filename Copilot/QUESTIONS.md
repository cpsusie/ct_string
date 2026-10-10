# Open questions for @cpsusie

The brief says: *"If you require additional information, now is the time to ask."* These are the
questions.

- **Every question has a recommended default.** Replying **"accept defaults"** in the PR accepts all
  of them. You can override any single question, for example *"Q-04: advertise 11.3"*.
- **Unanswered questions.** The default is a working assumption only. If a question is still open
  when the task in its "Needed by" column starts, Copilot asks again in that task's PR before
  relying on the default.
- **Where answers go.** Copilot records each answer below and updates the matching decision in
  [DECISIONS.md](DECISIONS.md) in the next PR.
- **None of these blocks the next task (T2).**

## Summary

| ID | Topic | Needed by | Recommended default |
|---|---|---|---|
| Q-01 | Package, CMake and target names | T4 | vcpkg `ct-string`, Conan `ct_string`, CMake `find_package(ct_string)` → `ct_string::ct_string`, `cstr_view` kept as an alias, option prefix `CT_STRING_` |
| Q-02 | Include directory, macro prefix, include guards | T9 | Keep `ct_str/`; new macros use `CPS_CT_STRING_`; make include guards consistent |
| Q-03 | Version of the first public release | T14 | `1.0.0`, preceded by a `v1.0.0-rc.1` pre-release that dry-runs the release pipeline |
| Q-04 | GCC floor to advertise | T6 | GCC 11.1 (tested); 10.x is impossible |
| Q-05 | Clang floor | T6 | Clang 17, no workarounds |
| Q-06 | MSVC v142 (VS 2019 16.11) | T7 | Test it if the runner can provide it; drop it if it needs non-trivial workarounds |
| Q-07 | macOS / AppleClang | T8 | Yes: after the libc++ fix, add a macOS arm64 job |
| Q-08 | Meaning of "separate test-suite packages" | T10 | In-repo vcpkg manifest and Conan recipe for the test suite; not published to the registries |
| Q-09 | CMake minimum | T4 | 3.21 minimum, newest tested 4.x as the policy maximum |
| Q-10 | fmt opt-in/opt-out | T11 | Keep auto-detection by default; add a force on/off switch; package managers default to off |
| Q-11 | Release and branch flow | T15 | `develop-release_1` → PR into `main` → you push tag `vX.Y.Z` → workflow publishes |
| Q-12 | Who submits to vcpkg and ConanCenter | T19 | You, using files and steps Copilot prepares |
| Q-13 | Which new documents need Latin | T14 | Only README and LEGENDUM are bilingual; everything else is English |
| Q-14 | Code formatter (clang-format) | T5 | Not before 1.0; revisit afterwards |
| Q-15 | Role of `test_console_app` | T10 | It is a demo: move it to `examples/`, behind an option, and build it in CI |
| Q-16 | GitHub repository settings | T5 | Rulesets on `develop-release_1` and `main`; squash merge; delete merged branches |
| Q-17 | CI budget and Docker Hub | T6 | Full matrix on every PR and push; anonymous pulls of the official `gcc` images |
| Q-18 | CPU architectures | T6 | x64 everywhere, plus one Linux arm64 job (GCC 14, full suite) |
| Q-19 | Community files and documentation hosting | T17 | Add CONTRIBUTING and SECURITY files and issue templates; no separate docs site |
| Q-20 | Visual Studio flags for the C++23 suite | T7 | "/cpplang" means `/std:c++latest`; VS 2026 required, VS 2022 17.14 best-effort |

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

### Q-06 MSVC v142 (VS 2019 16.11, MSVC 14.29)
- **Context.**
  - v142 is the first MSVC that is complete for `/std:c++20`.
  - VS 2019 is out of mainstream support.
  - GitHub's Windows images may offer v142 only as a side-by-side toolset, which T7 will verify.
- **Options:**
  - (a) Test v142 if it is available. Keep it if it passes; if it needs non-trivial workarounds,
    come back to you before dropping it. *(Recommended.)*
  - (b) Support VS 2022 (v143) and later only.
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
- **Recommendation:**
  1. Work accumulates on `develop-release_1`.
  2. A release PR merges `develop-release_1` into `main`.
  3. You push an annotated tag `vX.Y.Z` on `main`.
  4. The release workflow builds and verifies the release, then creates the GitHub Release with
     notes and checksums.
  5. After 1.0.0, the next cycle uses a new branch (for example `develop-release_2`) or keeps
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
- **Context.** It is a C++23 program using `std::expected` and fmt. It demonstrates the library but
  has no assertions.
- **Options:**
  - (a) Move it to `examples/console_app/`, build it when `CT_STRING_BUILD_EXAMPLES` is on, and have
    the C++23 CI jobs build it. *(Recommended.)*
  - (b) Keep it where it is, built together with the tests.
  - (c) Remove it.
- **Answer:** _pending_

### Q-16 GitHub repository settings (you apply these; T5 includes click-by-click steps)
- **Recommendation:**
  - A branch ruleset on `develop-release_1` and `main` that:
    - requires a PR;
    - requires the CI status checks;
    - requires branches to be up to date;
    - blocks force pushes and deletion.
  - Merge method: squash.
  - Turn on "Automatically delete head branches".
  - Use the merge queue if GitHub offers it for this repository.
- **Answer:** _pending_

### Q-17 CI budget and Docker Hub
- **Context.**
  - Hosted-runner minutes are free for public repositories.
  - The finished matrix will have an estimated 30–35 jobs, run in parallel, taking roughly 10–15
    minutes of wall time.
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
