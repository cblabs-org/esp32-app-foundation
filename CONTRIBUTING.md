# Contributing to the ESP32 App Foundation

Thank you for helping. Bug reports, fixes, documentation, board reports and new
modules are all welcome. This guide covers how to get set up, what a pull
request needs, and where the deeper documentation lives.

By taking part you agree to follow the [Code of Conduct](CODE_OF_CONDUCT.md).
Security problems go through [SECURITY.md](SECURITY.md), not public issues.

## Ways to contribute

| What you want to do | Where to go |
|---|---|
| Report a bug | [Open an issue](https://github.com/cblabs-org/esp32-app-foundation/issues/new/choose) using the bug template. [Reporting Framework Issues](docs/developer/issue-management.md) explains the labels. |
| Tell us how a board works | The *Board report* issue template |
| Fix a bug or improve docs | Fork, branch, and open a pull request (below) |
| Propose a new capability | Open a *Feature request* first if it changes a public interface, adds a module or touches an architectural invariant, so the shape can be agreed before you write it. See the [Contribution Protocol](.agents/skills/framework-expert/references/contribution-protocol.md). |
| Write a module for your own app | [Creating a Module](docs/developer/creating-a-module.md) and [Coding Patterns](docs/developer/coding-patterns.md) |

## Development setup

You need:

- [PlatformIO](https://platformio.org/install) (CLI or the VS Code extension)
- Python 3 (build scripts and doc checks)
- Node.js 20+ for the portal tests
- Optionally an ESP32 or ESP32-S3 board; most changes can be checked with the
  host build alone

```bash
git clone --recurse-submodules https://github.com/<you>/esp32-app-foundation.git
cd esp32-app-foundation
```

The submodules are agent skills under `.agents/skills/`; the firmware builds
without them.

### Build

```bash
pio run -e esp32s3nano          # ESP32-S3 reference build
pio run -e esp32dev             # ESP32 reference build
pio run -e crowpanel-128-rotary # LVGL colour display build
```

`platformio.ini` lists every environment. The sample and template apps build
from their own folders (`cd sample-app && pio run -e esp32s3nano`).

### Test

```bash
pio test -e native              # GoogleTest suites on the host, no hardware
pio test -e native_ha           # Home Assistant module tests

cd test/web && npm ci && npx playwright install chromium && npm test   # portal tests
```

How the host build stubs out ESP-IDF and Arduino is explained in
[Testing](docs/developer/testing.md).

### Documentation checks

```bash
python3 scripts/doc_lint.py                 # stale identifiers and broken references
python3 scripts/check_doc_cpp_samples.py    # compiles the C++ samples in docs
```

## Pull requests

1. Branch from `main` and keep the PR to one change. Name branches and PR
   titles in [Conventional Commits](https://www.conventionalcommits.org/)
   style: `fix(wifi): …`, `feat(ota): …`, `docs: …`.
2. Link the issue in the description (`Resolves #123`).
3. Before you open it:
   - it builds for the environments your change affects
   - `pio test -e native` passes, and new behaviour has a test where the host
     build can reach it
   - portal changes pass the web tests
   - docs are updated with the code: a change to a public interface also
     updates its page in `.agents/skills/framework-expert/interfaces/` and the
     developer guide
   - breaking changes are listed in `CHANGELOG.md` under `[Unreleased]` with a
     migration note
   - the diff contains no secrets, private keys, Wi-Fi details or personal
     paths
4. A maintainer reviews and merges. Releases and version tags are cut by the
   maintainers; contributors don't need to tag anything (see the
   [Versioning Policy](docs/developer/versioning-policy.md)).

### Code style

C++17, following the house rules in
[`style-guide.md`](.agents/skills/framework-expert/coding-patterns/style-guide.md):
file header block, `_camelCase` members, `UPPER_SNAKE` constants, Doxygen on
public APIs, and no heap allocation or blocking calls in `tick()`. The
[Embedded Safety](docs/developer/embedded-safety.md) guide covers threading and
memory rules for code that runs on more than one task.

### Licensing

The project is licensed under [Apache 2.0](LICENSE). Under section 5 of that
licence, anything you submit for inclusion is contributed under the same
terms. Only submit work you have the right to license this way.

## Documentation layout

The [Documentation Authority Map](docs/README.md) describes the four
documentation trees and which wins when they disagree.

- Every new design document in `docs/` or `docs/design/` carries a STATUS
  banner: `WIP`, `FINALIZED`, `IMPLEMENTED` or `SUPERSEDED`.
- Design documents fold into the [Design Specification](docs/design_specification.md)
  only once the design has shipped; superseded designs move to `docs/archive/`.
  See the [Folding Rule](docs/README.md#folding-rule-for-design-documents).

## Working with AI agents

The repository is developed with AI coding agents, and its agent
configuration is part of the source tree:

- `AGENTS.md` is the harness-neutral bootstrap; `CLAUDE.md` and `GEMINI.md`
  add harness-specific notes.
- `.agents/rules/` holds operating rules every agent session reads first.
- `.agents/skills/` holds the skills, including `framework-expert`.

If you use an agent to contribute, point it at `AGENTS.md`. The rules expect
builds and tests to go through the PlatformIO MCP server: copy
`.mcp.example.json` to `.mcp.json` (it is gitignored) to give your agent one.
The GitHub-reading rule applies only with the maintainers' bug-fixer plugin and
says what to do without it. You are still responsible for every change the
agent makes. [AI-Native Development](docs/developer/ai-native-development.md)
explains how the agent setup and the `.memory/` and planning records fit
together.
