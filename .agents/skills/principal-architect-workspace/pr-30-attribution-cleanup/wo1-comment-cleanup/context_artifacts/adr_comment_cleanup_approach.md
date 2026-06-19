# ADR: Comment Cleanup Approach

## Context

The ESP32 App Foundation was forked/refactored from a project called
"departures-board" by Gadec Software. Several shipped source files retained
provenance comments referencing the original project name, GitHub URL, and
Gadec copyright. These needed removal to establish clean attribution for the
standalone foundation project.

## Decision

**Replace, don't delete.** Every comment containing a departures-board or gadec
reference was replaced with a generic equivalent that preserves the *technical
meaning* of the original comment. This follows the work order's explicit
constraint: "preserve the meaning of each comment."

### Replacement strategy by category

| Category | Original phrasing | Replacement rationale |
|---|---|---|
| Copyright header | "Departures Board (c) Gadec Software" | Replaced with "ESP32 App Foundation (c) CB Labs" — the correct legal entity |
| Protocol comments | "matching departures-board GitHub asset protocol" | Simplified to "for GitHub release asset API" — the protocol is GitHub's, not departures-board's |
| Pattern references | "per departures-board pattern", "departures-board's proven init sequence" | Replaced with generic descriptions ("Standard U8G2 SPI display initialisation sequence") |
| Design comparisons | "Unlike the departures-board reference (which reads a hardcoded global key)" | Rewritten to describe the design choice without naming the predecessor |
| Style references | "Adapted from departures-board v3 design language" | Replaced with "Custom design system for ESP32 App Foundation" |

## Boundary Constraints Respected

- `.agents/` directory: untouched (internal planning docs, not shipped)
- `lib/MD5Utils/`: untouched (deferred to WO2 — different attribution concern)
- No logic changes: only comments and header blocks modified
- No branch creation: worked on the pre-existing `pr-30-attribution-cleanup-wo1-comment-cleanup` branch
