# ADR: OTA Documentation Strategy

## Decision

Expand the existing `docs/design_specification.md` §13.3 from 9 subsections to 19 subsections covering all production OTA features, and create a new `docs/OTADeveloperGuide.md` as a standalone developer-facing guide.

## Context

The design specification previously documented the OTA system at a basic level (security & blessing ceremony). With PR-14 WO1-WO7 completing the production OTA implementation, the documentation needed to cover:

- SemVer engine with packed 32-bit comparison
- Release channels (stable/beta/dev) with implicit downgrade
- Multi-architecture manifest with BOARD_ID injection
- Fail-closed signature enforcement
- Task watchdog supervision
- Auto-rollback (bootloader) and anti-rollback (NVS security version)
- Config lifecycle (three-tier Active/Previous/Golden)
- CI/CD pipeline (GitHub Actions + local signing)
- Expanded REST API (6 endpoints including setChannel and rollback confirmation)
- Web portal OTA tab integration

## Rationale

1. **Two-document approach**: The design specification is the architectural truth-source (for maintainers); the developer guide is the task-oriented how-to (for consumers). Separating concerns avoids bloating either document.

2. **§13.3 subsection numbering**: Kept contiguous numbering (13.3.1 through 13.3.19) rather than creating a new top-level section. This preserves existing cross-references from §9 (boot sequence mentions §13.3) and maintains the security architecture grouping.

3. **Restored §12.6/§13.1/§13.2**: The initial replacement inadvertently consumed the §12.6 LDF Constraints text, §13 heading, and §13.1/§13.2 sections. These were restored in a follow-up edit.

4. **Old content cleanup**: The replacement left duplicate old §13.3.8/§13.3.9 content at the end. A second edit removed the leftover text.

## Consequences

- All internal cross-references (§13.3.6, §13.3.9, §13.3.10, §13.3.11) validated via grep
- No C++ modifications, no build system changes, no git operations
- The OTADeveloperGuide.md is self-contained — it can be read without the design specification
