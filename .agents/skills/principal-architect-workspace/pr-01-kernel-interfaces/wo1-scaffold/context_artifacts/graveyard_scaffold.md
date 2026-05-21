# Graveyard: Repo Scaffold & PlatformIO Baseline

## Branch Naming with Directory Slashes
Initially, the worker agent attempted to use branch names like `pr-01-kernel-interfaces/wo1-scaffold`. However, git treats the slash as a directory separator, which prevents creating a branch of that name while `pr-01-kernel-interfaces` is an active feature branch.
- **Lesson Learned**: Use flat naming conventions (e.g., `pr-01-wo1-scaffold`) to avoid git reference namespace collisions.

## Untracked CMake and Cache Artifacts
PlatformIO with ESP-IDF generates a lot of files at the root (such as `CMakeLists.txt`, `dependencies.lock`, `sdkconfig.esp32dev`, and `managed_components/`). Initially, some were tracked, which polluted the commit space.
- **Lesson Learned**: Add them to `.gitignore` and untrack them using `git rm --cached` immediately.
