# Development and Release Project

## Introduction

I would like to professionally polish this project enough to be usefully released to the public via vcpkg and CONAN.  I lack experience with managing CI pipelines, packaging and DEVOPS generally so I will greatly be relying upon your advice and guidance.  I will be using this project as a learning experience to gain experience in these areas.

This project contains a README.md (English) and LEGENDUM.md (Classical Latin as close to Caesarian style and diction that a modern topic can be) file, which describe the benefits of this project and its use-cases.  Any work done should update both documents accordingly using the same style in both Latin and English.  As a former classicist, this amuses me and it is my project after all.

## Project Generally Works Well

As far of the code goes, this project currently works very well and seems to be well-tested.  Most of the work herein will be related to getting it ready for a true public release.

## Must Work On

The header-only library itself is should be buildable with C++20, and should be usable with GCC 11.3+ on Linux.  I have been building it with Clang 19+ on Linux as well.  Figure out the earliest version of Linux clang that can build this library and use that for the CI. (Same with GCC for linux).  For windows, I have been using MSVC 2026, but the library CI should make sure that it is buildable with any version that fully (or nearly so) supports C++20 and this should be enforced by CI.  Do same for Visual Studio clang.  This project should assume a cross-platform build environment and rely on standard C++, not any specific compiler or platform target (assuming it complies sufficiently with C++ 20).

The full test suite should be a separate vcpkg and conan packages.  The full set of tests requires a reasonably complete C++23 implementation: currently it builds and passes just fine on GCC 14+, Clang 19+ and the latest version of Visual Studio (with /cpplang).  The CI should build and run the full test suite on all three platforms, but it should be optional for the user to build and run the full test suite.  The library itself should be usable with C++20, but the full test suite requires C++23.  Thus separate CONAN and vcpkg configurations should be available for the library and the full test suite.  The library should be usable with C++20, but the full test suite requires C++23.  Thus separate CONAN and vcpkg configurations should be available for the library and the full test suite.

## Code Style

I prefer a hyper modern C++ style, with heavy use of concepts, compile-time computation, ranges, lazy views and other modern C++ features.  The code should be clean, well-documented, and follow best practices for C++ development.  Use of third-party libraries should be minimized unless justified by overriding concerns.  The library itself should have no third-party dependencies.  The test project obviously requires third-party dependencies (e.g., gtest, obviously).  The library should be header-only, and the test project should be a separate buildable project.

## Branching and PRs

The branch this project should work off of is develop-release_1.  All branches should be made thereform and all pull requests generated be back into this branch. CI should track this branch and main at first, later also including all branches with the words "develop" or "release" therein.  PRs should be run on CI as they would be merged with the base branch.

## Work Rules

Only work on one PR at a time and make them small and discrete.  Each PR should be focused on a single task or issue, and should be reviewed and approved by me before moving on to the next task.  All work should be done in a professional manner, with clear commit messages and documentation and consistent with best modern C++ practices.  Any changes to the codebase should be accompanied by appropriate tests and documentation updates.  Do not start work on part n of a task before n-1 has been approved and merged by me.

## Instructions

### Task 1 Develop a detailed plan for accomplishing my objectives.

Provide a detailed plan for accomplishing my objectives, including a timeline, milestones, and specific tasks to be completed.  Keep track of all decisions made and rationale therefor in markdown documents sub Copilot folder.  This plan should have concrete instructions for discrete tasks and should be broken down into manageable chunks that can be completed in a reasonable amount of time.  Each task should have a clear definition of done, and should be reviewed and approved by me (and merged by me) before moving on to the next task.  The plan should be flexible enough to accommodate changes in priorities or unforeseen obstacles, but should also provide a clear roadmap for achieving my objectives.  At the end of each task/milestone, plans for the following tasks should be created.

So, first provide a detailed executable concrete plan for accomplishing my objectives.

If you require additional information, now is the time to ask.




