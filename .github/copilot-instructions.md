# Duke3DX — Copilot instructions

Duke3DX is a cleanroom reimplementation of the Build engine running the
original Duke Nukem 3D 1.5 game code with minimal modifications, ported
to modern C++ and Win32. Windows only. The build is CMake with the Ninja
generator and the MSVC toolchain; configure and build presets for
x86/x64 and debug/release live in SRC/CMakePresets.json.

## Build

    cmake --preset win64-debug
    cmake --build --preset win64-debug
    cmake --preset win64-release
    cmake --preset win86-debug

- Presets: win64-debug, win64-release, win86-debug, win86-release.
- The MSVC environment (vcvarsall or msvc-dev-cmd) must be active before
  invoking cmake; Ninja finds cl.exe through it.
- Run cmake from the repo root; the top-level CMakeLists.txt defers to SRC.
- Binaries (duke3dx.exe, editor.exe) land at the repo root next to
  DUKE3D.CFG; objects and intermediate libs go to build/ and out/.

## Layout

- CMakeLists.txt (repo root) — entry point, only calls add_subdirectory(SRC).
- SRC/CMakeLists.txt — target definitions: duke3dx (game) and editor
  (Build editor, WIN32 GUI subsystem). Sources are listed explicitly.
- SRC/CMakePresets.json — configure and build presets shared by VS Code
  and CI.
- SRC/ — all engine and game sources (Build engine + Duke Nukem 3D 1.5
  game code, GPL).

## CI

One workflow builds per push, then a second publishes its results:

- build-windows (ci-windows.yml) — matrix of x86/x64 times debug/release,
  built with CMake presets under MSVC.
- publish (ci-publish.yml) — after the build finishes, commits
  build-digest.md and build-windows.log to the repo root.

Working rules:

1. Read build-digest.md first: per-leg status, errors, failure context and warning kinds. Open build-windows.log only when the digest trims a context block you need.
2. Fix every error AND every warning. The build compiles with /W3 and a small suppression list; a change is not done until the digest shows zero warnings.
3. Never hand-edit build-digest.md or build-windows.log. They are bot outputs regenerated every run.
4. [skip ci] in a commit message skips all workflows for that commit. Use it only for pushes that cannot affect the build (docs, notes). Never put it on a code push.
5. Digest files in the repo root can lag the branch head: the publish job also pushes. Match the commit sha printed at the top of build-digest.md against the branch head before diagnosing.

## Code style

- No formatter is configured. Keep new code consistent with the
  surrounding style, and keep formatting changes in their own commit,
  never mixed into a functional change.
- Comments: plain English, only where the code is not self-explanatory.
  No line-by-line noise, no commented-out code left behind.
- Edits are surgical and behavior-preserving. No drive-by rewrites or
  style-only refactors of engine code.

## Workflow

- Work on a feature branch, open a PR into main, squash merge when CI is
  green.
- Commit messages: plain-English bullet lists. One logical change per
  commit.
- After pushing, the publish job may push a digest commit on the same
  branch; rebase on it instead of discarding it.
