# Workorder 2: CI Workflow

## Objective
Implement a GitHub Actions workflow for continuous integration.

## Tasks
1. Create `.github/workflows/ci.yml`.
2. Configure it to trigger on pull requests and pushes to `main`.
3. Set up a dual-environment build using PlatformIO for `esp32dev` and `esp32s3nano`.
4. Add native test execution (`pio test -e native`).
5. Add an include-graph linting step to enforce architectural boundaries.
