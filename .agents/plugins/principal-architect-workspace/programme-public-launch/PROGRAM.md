---
schema_version: 1
programme_id: prog-public-launch
title: Public Launch v1.0.0-beta.1
base_branch: launch/pr-d-readme-community
status: DRAFT
prs:
- id: PR-144
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
  status: PLANNED
  github_pr: null
---

# Public Launch v1.0.0-beta.1

## 1. Architectural Vision & System Invariants

## 2. Cross-PR Contracts & Interfaces

## 3. Programme Session Log
