"""
ESP32 App Foundation (c) 2026 CB Labs

PlatformIO pre-build script: scripts/merge_framework_data.py
Description: Copies framework-owned LittleFS data files into the project's
             local data/ directory at build time. This ensures sub-apps
             (template-app, sample-app) receive framework data files like
             timezones.json without maintaining duplicate copies.

    Resolution strategy:
        1. Locate the framework's data/ directory relative to this script
           (../data/ from scripts/).
        2. Copy each non-dotfile into the project-local data/ directory.
        3. Skip files that already exist in the destination (app takes
           priority over framework defaults).
        4. No-op when run from the root project (source == destination).

    Supported layouts:
        - Root project:   scripts/ and data/ are siblings → no-op
        - Sub-app:        scripts/ is at ../scripts/, data/ is at ../data/
"""

Import("env")

import os
import shutil


def merge_framework_data():
    """Copy framework-owned LittleFS files into the project's data/ directory.

    PlatformIO sets CWD to the project directory. For sub-apps, the framework's
    data/ is at ../data/ (one level up). For the root project, source and
    destination are the same directory — detected and skipped.

    Skips:
      - Files that already exist in the destination (app takes priority)
      - Dotfiles (.gitkeep, .DS_Store, etc.)
    """
    framework_data_dir = os.path.normpath(os.path.join("..", "data"))
    project_data_dir = os.path.normpath("data")

    # No-op if source == destination (root project build)
    if os.path.abspath(framework_data_dir) == os.path.abspath(project_data_dir):
        return

    if not os.path.isdir(framework_data_dir):
        print(f"[merge-data] Framework data dir not found: {framework_data_dir}")
        return

    os.makedirs(project_data_dir, exist_ok=True)

    copied = 0
    for filename in os.listdir(framework_data_dir):
        if filename.startswith("."):
            continue
        src = os.path.join(framework_data_dir, filename)
        dst = os.path.join(project_data_dir, filename)
        if not os.path.isfile(src):
            continue
        if os.path.exists(dst):
            # App-specific file takes priority — don't overwrite
            continue
        shutil.copy2(src, dst)
        copied += 1
        print(f"[merge-data] Copied framework data: {filename}")

    if copied:
        print(f"[merge-data] Merged {copied} framework file(s) into {project_data_dir}")


merge_framework_data()

