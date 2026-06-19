# ADR: Documentation Lifecycle Decisions

## Decision 1: Design Spec as Navigable Index (Not a Standalone Doc)

**Context:** The design specification (2700+ lines, 38 ADRs) was the original architectural blueprint but became increasingly disconnected from the 15 developer guides in `docs/developer/`.

**Decision:** Rather than retire the design spec, turn it into a navigable architectural index by adding deep links at both the `##` section level (callout boxes) and `###` subsection level (inline links). 64 total links were added.

**Rationale:** The design spec answers "why" (rationale, ADRs, design decisions). The developer guides answer "how" (tutorials, examples, checklists). Linking them creates a two-way navigation system — readers can go from rationale → tutorial or browse the spec as a discovery index.

**Rejected alternative:** Merging design spec content into developer guides. This would have destroyed the ADR structure and made the developer guides too long and philosophical.

## Decision 2: OTA Guide Belongs in Developer Tree

**Context:** `OTADeveloperGuide.md` was the only tutorial-style doc living at `docs/` root instead of `docs/developer/`.

**Decision:** Move it to `docs/developer/ota-update-system.md` (guide #14).

**Rationale:** Consistency — every other tutorial is in `docs/developer/`. The OTA guide's unique content (state machine, config lifecycle, error codes, deploy procedures) is tutorial material, not architectural rationale.

**Deduplication:** Signature verification, anti-rollback, and key rotation content was already in `security.md`. The OTA guide now cross-links to those sections instead of duplicating them.

## Decision 3: Dense Links vs Section-Level Only

**Context:** The initial WO2 added 17 links at `##` headings only. User feedback: "we should have way more than 17... ITickable should have links directly to the correct parts."

**Decision:** Added 47 subsection-level links (64 total) using a compact inline format: `> 📖 See [Guide § Section](developer/guide.md#anchor)`.

**Format rationale:** Section-level links use the heavier `📖 **Developer Guide:**` callout box. Subsection links use the lighter inline `📖 See` format to avoid visual clutter at the `###` level.

## Decision 4: MCP API Documentation Fix

**Context:** PR-28 eliminated `std::function` from the MCP tool registry, replacing it with `MCPToolHandler` (static function pointer + `void* ctx`). The `mcp-server.md` developer guide still documented the old API.

**Decision:** Updated all API signatures and examples to match the current `MCPToolHandler` pattern, referencing `template/src/myApp.cpp` as the canonical example.

## Decision 5: Legacy Doc Triage

| Doc | Decision | Rationale |
|---|---|---|
| `build_and_release_process.md` | **MERGE + DELETE** | §12 Key Rotation → `security.md`, §11 CI Workflow → `build-system.md`. All unique content absorbed. |
| `security-evaluation-report.md` | **ARCHIVE** | Point-in-time audit, no tutorial content |
| `embedded-best-practices-assessment.md` | **ARCHIVE** | Point-in-time review, superseded by developer guides |
| `comparative_analysis_report.md` | **ARCHIVE** | Agent comparison, historical provenance only |
| `design_specification.md` | **KEEP + ENHANCE** | Contains 38 ADRs, now serves as navigable index |
| `OTADeveloperGuide.md` | **MOVE** | Tutorial content → `docs/developer/ota-update-system.md` |
