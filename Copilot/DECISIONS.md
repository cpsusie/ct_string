# Decision log

This file records every project decision and the reason for it, as the brief
(`develop-release-project.md`) requires. The evidence behind the decisions is in
[FINDINGS.md](FINDINGS.md), and the plan that carries them out is in [PLAN.md](PLAN.md).

**Statuses**

- **Proposed:** written by Copilot. It becomes **Accepted** when you merge the PR that contains it,
  unless it is marked *pending*.
- **Pending Q-nn:** waits on your answer to that question in [QUESTIONS.md](QUESTIONS.md). Until
  then, the recommended default shown is only a working assumption. Your answer may change the
  decision.
- **Accepted:** approved by you.
- **Superseded by D-nnn:** replaced by a later decision. Entries are never deleted, only
  superseded, so the history stays readable.

Each entry gives the context, the decision, the reason, the alternatives considered, and the
related tasks (Tn) and questions (Q-nn). Entries are grouped by topic. IDs are permanent and never
reused, so they may appear out of numeric order.

---

## Process

### D-001 Planning documents live in `Copilot/` and are updated in every PR
- **Date:** 2026-10-10. **Status:** Proposed. **Related:** T1, all tasks.
- **Context:** The brief asks for decisions and their reasons to be tracked in markdown under
  `Copilot/`, and for the following tasks to be planned at the end of each task.
- **Decision:**
  - Four documents:
    - [PLAN.md](PLAN.md): roadmap, tasks and status.
    - DECISIONS.md: this log.
    - [QUESTIONS.md](QUESTIONS.md): open questions and your answers.
    - [FINDINGS.md](FINDINGS.md): evidence.
  - Every task PR updates PLAN.md (status, plus the next task in full detail) and adds to this log.
  - The brief, `develop-release-project.md`, is never edited by Copilot.
- **Why:** One concern per file. An append-only log keeps the history. Keeping the evidence
  separate keeps decisions short.
- **Alternatives:**
  - GitHub Issues or Projects: not versioned with the code, and the brief asks for markdown.
  - One large document: hard to review in small PRs.

### D-002 Rolling-wave planning with re-plan checkpoints
- **Date:** 2026-10-10. **Status:** Proposed. **Related:** all tasks.
- **Context:** The brief asks for a concrete plan that can still adapt to new priorities, with the
  following tasks planned at the end of each task or milestone.
- **Decision:**
  - The next task is always planned in full: steps, deliverables and definition of done.
  - Later tasks are outlined with scope and definition of done.
  - Each task's PR details the task after it.
  - Each milestone ends with a short re-plan: retrospective, risk review and timeline update.
- **Why:** Detail about distant tasks goes stale. What T3 teaches us (for example, how libc++
  behaves) changes T8. Planning one step ahead keeps every step reviewable without pretending to
  certainty.
- **Alternative:** Detail every task now. That is brittle and creates rework.

### D-003 Branch and PR workflow
- **Date:** 2026-10-10. **Status:** Proposed. **Related:** all tasks, D-026, Q-11, Q-16.
- **Decision:**
  - Every task branches from the latest `develop-release_1`, and its PR targets `develop-release_1`.
  - One PR is open at a time.
  - You review, approve and merge. Task *n* starts only after task *n-1* is merged.
  - Branch names follow `copilot/<task>-<slug>` where the tooling allows.
  - Each PR is a single topic and stays small; split it if it grows past about 400 changed lines of
    code.
  - **The one exception is promotion into `main`** (D-026). It is a PR from `develop-release_1`
    itself into `main`, opened and merged by you. It contains nothing you have not already
    approved.
- **Why:**
  - These are the brief's work rules. Small PRs are easier to review and to learn from.
  - A release needs the code on `main`, and promotion gets it there without Copilot ever targeting
    another branch.
- **Note:** Squash merging is recommended for task PRs, to keep the history linear (Q-16).
  Promotion PRs need a merge commit instead. A squashed promotion would give `main` and
  `develop-release_1` different histories, and every later promotion would conflict. The choice is
  yours, since you merge.

### D-004 The Task 1 PR changes documentation only, and README/LEGENDUM stay unchanged
- **Date:** 2026-10-10. **Status:** Proposed. **Related:** T1, D-025.
- **Context:** The brief says any work should update README.md and LEGENDUM.md in the same style.
- **Decision:**
  - Task 1 adds only the planning documents in `Copilot/`.
  - README and LEGENDUM are untouched, because nothing user-visible changes.
  - The README/LEGENDUM rule applies to every later task that changes behaviour, build, packaging
    or support.
- **Why:** The plan is internal project management. Users of the library see no difference.

### D-023 Fix the defects found during planning before building CI on top of them
- **Date:** 2026-10-10. **Status:** Proposed. **Related:** T2, T3, findings F-4 and F-5.
- **Decision:**
  - The two code defects found while planning are milestone M1, one PR each, with tests and docs:
    - the stale `CJM_` test macro;
    - libc++ comparison portability.
  - The CMake findings (F-6) go to T4.
- **Why:**
  - Both defects are small and well understood.
  - Fixing the macro first means the CI built later actually runs the fmt tests.
  - Fixing libc++ first lets the libc++ CI job (T8) start green.

---

## Supported toolchains

### D-005 Enforce the C++20 floor with the C++20-pinned smoke target in CI
- **Date:** 2026-10-10. **Status:** Proposed. **Related:** T4, T6, T7, T8, F-1, F-2.
- **Decision:**
  - The library stays C++20.
  - The existing `cstr_view_cxx20_smoke` target, pinned to C++20 and including every header, is the
    portable floor check. It must build without GoogleTest and without a C++23 compiler.
  - CI builds it on every supported toolchain, with and without fmt.
  - Each floor job also gets an install-and-consume check once packaging exists (T9).
- **Why:** The target already exists and catches C++23 constructs leaking into the headers. Pinning
  the language standard is stronger than `cxx_std_20`, which only sets a minimum.

### D-006 GCC floor: 11.1, tested in CI
- **Date:** 2026-10-10. **Status:** Pending Q-04 (whether to advertise 11.1 or 11.3).
- **Related:** T6, F-2.
- **Context:** The brief says "GCC 11.3+". The probe shows GCC 11.1 works and GCC 10.5 does not:
  heterogeneous unordered lookup first shipped in GCC 11.
- **Decision (default):**
  - Advertise and CI-test **GCC 11.1** as the floor, using the official `gcc:11.1` container.
  - Also test the runner GCCs: 11.4, 12, 13, 14, 15.
- **Why:** The brief asks for "the earliest version … and use that for the CI". What we test should
  be exactly what we advertise.
- **Alternative:** Floor at 11.3 (the brief's wording), with 11.1 untested. That is equally valid if
  you prefer it.

### D-007 Clang floor (Linux, libstdc++): 17, with no compiler workarounds
- **Date:** 2026-10-10. **Status:** Pending Q-05. **Related:** T6, F-2.
- **Decision (default):** Clang 17 is the earliest supported Clang and is tested in CI, along with
  every newer major version up to the newest on the runners (22).
- **Why:**
  - Clang 16 fails only because of a compiler bug in constrained-friend redeclaration, which would
    need a workaround in the code.
  - Clang 15 and earlier would need `typename` reinstated throughout, which goes against the
    hyper-modern style you asked for.
  - Clang 16 is long out of upstream support.
- **Alternative:** Support Clang 16 with a targeted workaround. Possible if you want it (Q-05).

### D-008 Toolchains for the full C++23 test suite
- **Date:** 2026-10-10. **Status:** Proposed. **Related:** T6, T7, F-3, Q-20.
- **Decision:**
  - CI runs the full suite and the demo on GCC 14 and 15.
  - Also on Clang 19, 20, 21 and 22 with libstdc++.
  - Also on the newest MSVC (VS 2026) and the clang-cl bundled with VS 2026.
  - The suite is also run on VS 2022 17.14 for as long as it passes there; that is a goal, not a
    promise.
  - GCC 13 currently passes too, but is not promised.
- **Why:** These are the brief's stated floors, confirmed by the probe (F-3). Promising less than
  what happens to work keeps the test project free to use newer C++23 features.

### D-009 MSVC: test every toolset from 14.29 (VS 2019 16.11) to the newest
- **Date:** 2026-10-10. **Status:** Pending Q-06. **Related:** T7, F-8, F-13.
- **Context:**
  - The brief asks CI to enforce every MSVC that fully or nearly fully supports C++20.
  - That is VS 2019 16.11 (MSVC 14.29, v142) and every later toolset: 14.30–14.44 (VS 2022) and
    14.5x (VS 2026).
  - The runners preinstall only 14.44 and 14.5x. Every toolset from 14.29 to 14.43 can be installed
    side by side during a job (F-13).
- **Decision (default):**
  - Build the C++20 smoke check with **every** toolset: 14.29, each VS 2022 minor version from 14.30
    to 14.44, and VS 2026's 14.5x. Later VS 2026 versions join as they ship.
  - These jobs run on every PR and push, and the required check covers them (D-013).
  - T7 measures the cost. If the sweep makes PR runs much slower, T7's PR offers Q-06 option (b)
    instead, and you choose.
  - If a toolset needs non-trivial workarounds, you decide whether to drop it. That is recorded as a
    new decision.
- **Why:**
  - Only a full sweep makes "enforced by CI" true for every version the brief covers. Testing just
    the oldest and the newest would leave 14 toolsets (14.30–14.43) unverified.
  - The side-by-side components make it possible on hosted runners, and minutes are free for public
    repositories.
- **Alternatives:** The oldest toolset plus the newest of each Visual Studio, or a sample of the
  versions in between. Both are cheaper, but leave toolsets the brief covers untested.

### D-010 clang-cl: every Clang major from the library's floor, each with an STL that accepts it
- **Date:** 2026-10-10. **Status:** Proposed. **Related:** T7, F-9, F-13.
- **Decision:**
  - CI builds the C++20 smoke check with clang-cl:
    - **Clang 17**, the library's floor (official LLVM release), with the 14.42 toolset's STL;
    - **Clang 18** (official LLVM release), with the 14.43 STL;
    - the clang-cl **bundled** with VS 2022 17.14 (14.44 STL) and with VS 2026 (14.5x STL);
    - the runner's **standalone** LLVM (20 today), which picks up newer majors as the images
      update.
  - The full suite runs with the clang-cl bundled with VS 2026.
- **Why:**
  - The brief asks for the same coverage as MSVC.
  - The MSVC STL sets its own minimum Clang (F-9). Pairing each Clang with an STL that accepts it
    tests the library's own floor on Windows too.
  - The bundled clang-cl is exactly what Visual Studio users get.
- **Alternative:** Only the clang-cl bundled with each Visual Studio. Cheaper, but Clang 17 and 18
  would go untested on Windows.

### D-024 libc++ incompatibility is a bug, fixed whatever the macOS scope
- **Date:** 2026-10-10. **Status:** Proposed. **Related:** T3, T8, F-5, Q-07.
- **Decision:**
  - Fix the comparison operators so they compile with any conforming standard library, keeping
    their meaning and an accurate `noexcept`.
  - Add a libc++ job to CI (T8).
  - macOS CI follows the answer to Q-07.
- **Why:**
  - The current code depends on a vendor-specific `noexcept` strengthening, which contradicts the
    brief's "rely on standard C++".
  - It blocks every libc++ user, including Conan profiles that use libc++.

---

## CI and DevOps

### D-011 GitHub Actions on GitHub-hosted runners, with explicit image labels
- **Date:** 2026-10-10. **Status:** Proposed. **Related:** T5–T8.
- **Decision:**
  - Use GitHub Actions on GitHub-hosted runners.
  - Name images explicitly (`ubuntu-24.04`, `windows-2022`, …), never `*-latest`.
  - Add a weekly scheduled run. It works only once the workflow is on the default branch (F-12,
    D-028).
  - Print every toolchain's version in the job log.
- **Why:**
  - Hosted runners are free for public repositories and need no maintenance.
  - `*-latest` labels move silently, causing surprise failures.
  - The scheduled run catches runner-image changes between PRs.
- **Alternatives:** Self-hosted runners or another CI service. Both add cost and administration.

### D-012 Workflow security baseline
- **Date:** 2026-10-10. **Status:** Proposed. **Related:** T5 onwards.
- **Decision:**
  - Workflows default to `permissions: contents: read`.
  - Third-party actions are pinned to full commit SHAs, with a version comment, and Dependabot keeps
    them current.
  - Checkouts do not persist credentials.
  - No `pull_request_target` workflows.
  - No secrets in pull-request jobs.
  - Every job has a timeout, and superseded PR runs are cancelled.
- **Why:** GitHub's hardening guidance. Pinning to a SHA guards against a compromised or
  re-pointed tag. Least privilege limits the damage if something does go wrong.

### D-013 PRs are tested as they will be merged, behind one required check
- **Date:** 2026-10-10. **Status:** Proposed. **Related:** T5, Q-16, F-12.
- **Decision:**
  - CI runs on the `pull_request` event, which builds GitHub's merge commit of the PR into its base
    branch.
  - A final gate job, `ci-ok`, depends on every other job. It always runs, and it fails unless all
    of them succeeded.
  - You add a ruleset that requires `ci-ok` as the only status check, and requires the branch to be
    up to date before merging. The merge queue can be used if it is available for this repository.
- **Why:**
  - This is exactly the brief's "run as they would be merged with the base branch".
  - The up-to-date rule closes the gap when the base branch moves after CI has run.
  - A skipped job counts as passing (F-12). Without the gate, a job skipped because another job
    failed could let a broken PR through.
  - With one required check, adding or renaming matrix jobs never needs a ruleset change.

### D-014 Trigger scope grows in two phases
- **Date:** 2026-10-10. **Status:** Proposed. **Related:** T5, T16, D-028.
- **Decision:**
  - **Phase 1 (T5).** Pushes to, and PRs into, `develop-release_1` and `main`, plus manual and
    weekly runs. Manual and weekly runs need the workflow on the default branch (D-028).
  - **Phase 2 (T16).** Add every branch whose name contains `develop` or `release`, using the
    `**develop**` and `**release**` patterns.
- **Why:** This is the brief's ordering, and phase 2 is a tiny, separately reviewable change.

### D-028 Make `develop-release_1` the default branch until the release
- **Date:** 2026-10-10. **Status:** Pending Q-21. **Related:** T5, T18, F-12, D-014.
- **Context:**
  - The weekly run, the manual "Run workflow" button, Dependabot and the PR template work only
    from files on the default branch, which is `main` (F-12).
  - All work lands on `develop-release_1`, so none of them would work before the release.
- **Decision (default):**
  - You make `develop-release_1` the default branch when T5 merges, and make `main` the default
    again at the release (T18). It is a repository setting, not a code change.
  - Dependabot's version updates name `develop-release_1` as their target, so they keep landing
    there after the switch back.
  - T5's PR shows which of these features work, and how it was checked.
- **Why:**
  - Every feature works from T5 on, on the branch where the work happens.
  - GitHub then also proposes `develop-release_1` as the base of new PRs, as the brief's rules
    require.
  - `main` stays unchanged until the release.
- **Cost:** Until the release, visitors to the repository and fresh clones see `develop-release_1`.
- **Alternatives:**
  - Promote `develop-release_1` into `main` early, after T5. `main` then carries unreleased work,
    and every later workflow change needs another promotion.
  - Wait for the release. Until then there are no weekly or manual runs, and Dependabot and the PR
    template do nothing.

### D-015 Toolchains are obtained in a fixed order of preference
- **Date:** 2026-10-10. **Status:** Proposed. **Related:** T6–T8.
- **Decision:**
  1. Compilers preinstalled on the runner.
  2. Otherwise, the Ubuntu archive. Example: `clang-19` and libc++ on `ubuntu-24.04`.
  3. Otherwise, an official Docker image. Example: `gcc:11.1` for the GCC floor.
  - No third-party PPAs, and no `curl | bash` installers.
- **Why:** This favours reproducibility, speed and supply-chain hygiene, and every source is
  documented.

### D-016 How CI obtains test dependencies (GoogleTest, fmt)
- **Date:** 2026-10-10. **Status:** Proposed. **Related:** T5–T7, T12, T13.
- **Decision:**
  - **Linux jobs** use Ubuntu packages.
  - **Windows jobs** use the test suite's vcpkg manifest, with a pinned baseline, with installed
    packages cached between runs.
  - **Packaging jobs** use vcpkg and Conan exactly as users would.
- **Why:**
  - Ubuntu packages are fastest.
  - Windows has no system package manager for these libraries.
  - As a bonus, CI covers several fmt versions (Ubuntu's 9.x/10.x and the newest from vcpkg and
    Conan).

---

## Build system and packaging

### D-017 CMake stays the only build system, with a lower minimum version
- **Date:** 2026-10-10. **Status:** Pending Q-09. **Related:** T4, F-6.
- **Decision (default):**
  - Replace `cmake_minimum_required(VERSION 4.0.2)` with a version range: minimum 3.21, maximum the
    newest 4.x version tested in CI.
  - Turn off C++ module scanning for the project's targets.
  - CI tests both the minimum and the newest CMake.
- **Why:**
  - 3.21 is the first version with `PROJECT_IS_TOP_LEVEL`.
  - It still lets Ubuntu 22.04's stock CMake (3.22) build the project. That system's GCC 11.4 is
    exactly the "GCC 11.3+" user the brief describes.
  - No CMake 4 feature is used.
  - Module scanning broke the Clang 19 build (F-6).
- **Alternative:** Minimum 3.23, to install headers with `FILE_SET`. That drops stock Ubuntu 22.04.

### D-018 Tests, demo and their dependencies are optional
- **Date:** 2026-10-10. **Status:** Proposed. **Related:** T4, T10.
- **Decision:**
  - New CMake options build the C++20 smoke check, the C++23 suite and the demo. They default to ON
    only when this is the top-level project, so your current workflow is unchanged.
  - fmt and GoogleTest are looked up only when something that needs them is enabled.
  - Consumers using `add_subdirectory`, `FetchContent` or a package manager get only the INTERFACE
    library, with no dependencies.
- **Why:** The brief says the library has no third-party dependencies and building the tests is
  optional. Today every configure requires fmt and GoogleTest (F-6).

### D-019 Upstream ships an official CMake package
- **Date:** 2026-10-10. **Status:** Pending Q-01 (names). **Related:** T9.
- **Decision (default):**
  - Install headers plus `ct_stringConfig.cmake` and `ct_stringConfigVersion.cmake`, marked
    architecture-independent, with SameMajorVersion compatibility.
  - Export the namespaced target `ct_string::ct_string`, and keep `cstr_view` as an alias for
    existing in-tree users.
  - Add a version header.
- **Why:**
  - With upstream exports, vcpkg needs no `unofficial-` wrapper (F-10), and Conan can match the
    same file and target names.
  - Every consumption route (`find_package`, `FetchContent`, vcpkg, Conan) ends up with the same
    target name.

### D-020 fmt integration becomes explicit and deterministic, with today's default kept
- **Date:** 2026-10-10. **Status:** Pending Q-10. **Related:** T11, F-7.
- **Decision (default):**
  - Keep `__has_include` auto-detection as the default for direct users.
  - Add an explicit override macro to force fmt support on or off, and a matching CMake option.
  - The vcpkg feature `fmt` and the Conan option `with_fmt` set the macro explicitly. Both default
    to off.
- **Why:**
  - Package managers require behaviour that does not depend on what else happens to be installed
    (F-10).
  - Keeping the default avoids breaking current users.

### D-021 Package managers: vcpkg and Conan 2 only
- **Date:** 2026-10-10. **Status:** Proposed. **Related:** T12, T13, T19, T20.
- **Decision:**
  - Provide a vcpkg port and a Conan 2 recipe, maintained in this repository and later submitted
    upstream.
  - Conan 1 is not supported.
- **Why:** These are the brief's two targets, and ConanCenter now publishes new recipes for Conan 2
  only (F-10).

### D-022 The test suite gets its own vcpkg and Conan configurations, kept in this repository
- **Date:** 2026-10-10. **Status:** Pending Q-08. **Related:** T10, T12, T13, F-10.
- **Decision (default):**
  - The test suite becomes a standalone C++23 CMake project in `tests/`.
  - It builds against either the in-tree library or an *installed* library package.
  - It has its own vcpkg manifest and Conan consumer recipe, each depending on the library package
    plus GoogleTest and fmt.
  - It is not submitted to the central registries. Their policies leave no room for a package whose
    only purpose is running tests.
- **Why:**
  - This meets the brief's "separate vcpkg and conan configurations … for the library and the full
    test suite".
  - Building the suite against the packaged library also tests the packaging itself.
- **Alternative:** Publish a test-suite package through a custom vcpkg registry and Conan remote.
  That means more infrastructure to run; see Q-08.

---

## Documentation and release

### D-025 Which documents are bilingual
- **Date:** 2026-10-10. **Status:** Pending Q-13. **Related:** all user-visible tasks.
- **Decision (default):**
  - README.md and LEGENDUM.md are always updated together, in the same PR, with the same
    structure and examples. The Latin keeps the Caesarian register.
  - Planning documents (`Copilot/`), CHANGELOG, CONTRIBUTING, SECURITY, workflow comments and PR
    descriptions are English only.
- **Why:** The brief names exactly those two documents. Translating operational documents would
  slow every PR without helping users.

### D-026 Versioning and release flow
- **Date:** 2026-10-10. **Status:** Pending Q-03 and Q-11. **Related:** T14, T15, T18, D-003.
- **Decision (default):**
  - Semantic Versioning 2.0.0.
  - Annotated tags `vMAJOR.MINOR.PATCH`.
  - CHANGELOG.md in the Keep a Changelog format.
  - Releases come from `main`:
    1. A release-preparation PR into `develop-release_1` finalises the version and the CHANGELOG.
    2. You promote `develop-release_1` into `main`: a PR merged with a merge commit (D-003).
    3. You push the tag on `main`.
    4. A workflow creates the GitHub Release with checksums.
  - The version is held in one source of truth, which CI checks against every place that repeats
    it.
  - A pre-release tag (for example `v1.0.0-rc.1`) dry-runs the pipeline before 1.0.0. You push it
    on `develop-release_1`, so no promotion is needed for it.
- **Why:**
  - Registries reference immutable tags and checksums.
  - SemVer tells users which upgrades are safe.
  - The dry run catches pipeline mistakes before they become a public release.
  - Copilot's PRs keep targeting `develop-release_1`. Only you change `main`.

### D-027 You submit to the central registries, using material Copilot prepares
- **Date:** 2026-10-10. **Status:** Pending Q-12. **Related:** T19, T20.
- **Decision:**
  - Copilot prepares the port and recipe files, the version database entries and step-by-step
    instructions.
  - You fork `microsoft/vcpkg` and `conan-io/conan-center-index`, open the PRs, sign the
    ConanCenter CLA and answer reviewers.
  - Copilot's preparation for each registry is a normal task PR into `develop-release_1`. The
    upstream PRs live in other repositories, so the "one PR at a time" rule (D-003) does not apply
    to them: the two upstream reviews can run in parallel.
  - A submission task is done when its upstream PR is merged, or when you decide to close it,
    recorded as a decision with the reason.
- **Why:**
  - Copilot can only push to this repository.
  - The registries expect the upstream maintainer to own the submission.
  - It is also a valuable part of the learning goal.
