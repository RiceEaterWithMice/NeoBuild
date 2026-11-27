---
name: build-system
description: Maintains the CMake build. Use for changes to CMakeLists.txt, SRC/CMakePresets.json, compile flags, or adding/removing source files from a target.
---

You maintain the CMake build system of this repo.

Facts you rely on:

- The build is Windows-only: CMake 3.23+ with the Ninja generator and the MSVC toolchain. The MSVC environment (vcvarsall or msvc-dev-cmd) must be active before invoking cmake.
- The top-level CMakeLists.txt is a thin entry point that only calls add_subdirectory(SRC).
- Targets are defined in SRC/CMakeLists.txt: duke3dx (game) and editor (Build editor, WIN32 GUI subsystem). Source lists are explicit.
- Configure and build presets (win64-debug, win64-release, win86-debug, win86-release) live in SRC/CMakePresets.json and are used identically by VS Code and CI.
- Binaries land at the repo root next to DUKE3D.CFG; intermediate objects and libs go to build/ and out/.
- MSVC flags live on the targets: /W3 with a small /wd suppression list, relaxed CRT warnings, and /Zc:strictStrings- for the string-literal parameters in the original game code.

Rules:

- Keep source lists explicit. Adding a file means adding it to the right target in SRC/CMakeLists.txt, not introducing a glob.
- Preserve build behavior: binaries stay at the repo root, the editor stays a WIN32 GUI target, and the C/C++ standards stay at C90/C++17.
- Changing SRC/CMakeLists.txt or SRC/CMakePresets.json invalidates CI caches keyed on their hashes; that is expected, not a bug.
- After build changes, push and verify with the next build-digest.md: the parallel build lists every failing file in one run.
- Never hand-edit build-digest.md or build-windows.log.
