---
schema_version: 1
charter:
  pr_id: PR-144
  programme_id: prog-public-launch
  slug: pr-144-ci-workflows
  title: 'ci: GitHub Actions quality gates and fail-closed web test runner (Resolves
    #144)'
  branch: launch/pr-e-ci-workflows
  base_branch: launch/pr-d-readme-community
  depends_on: []
  allowed_modules:
  - .gitleaks.toml
  - .github/workflows/
  - README.md
  - platformio.ini
  - sample-app/platformio.ini
  - template-app/platformio.ini
  - scripts/run_web_tests.py
  - scripts/tests/
  - test/web/playwright.config.ts
  required_skills: []
  verification_floor:
  - pytest scripts/tests/ && python3 scripts/doc_lint.py
  charter_locked: true
  charter_hash: 038cb30232b08217e3f4ac10e61d5d790820d8fc7c8c45716a15c59d69a87447
status: PLANNED
approved_at: null
github_pr: null
work_orders: []
---

# PR-144 — ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144)

## 1. PR Charter: Scope, Goals, & Non-Goals

## 2. Architectural Requirements, Cross-PR Interfaces, & Required Skills/Tools

## 3. Implementation Plan & Work Order Breakdown

## 4. Outcome & Session Log
