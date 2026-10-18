# ADR: §13.3 Specification Rewrite Approach

## Decision: 9-Subsection Hierarchical Structure

**Context:** The original §13.3 was a 19-line stub covering OTA in 3 bullet points plus the blessing ceremony. WO1–WO3 delivered a complete three-layer OTA architecture that needed comprehensive documentation.

**Decision:** Decomposed §13.3 into 9 sub-sections (§13.3.1–§13.3.9) using `####` headers, matching the spec's existing style where subsections use this pattern (e.g., §11A has §11A.1–§11A.9).

**Rationale:**
- The OTA subsystem has 3 distinct architectural layers, each requiring its own documentation
- The state machine, error codes, and REST API each have enough complexity to warrant tables
- Inline code blocks show the IOtaVerifier contract and OtaProgressEvent struct — these are the key API surfaces future developers need

## Decision: Preserved Blessing Ceremony Verbatim

**Context:** WO4 instructions specified "Keep the existing blessing ceremony content (lines 1435–1446) exactly as-is" but update the introductory sentence.

**Decision:** Preserved the numbered list and prohibition blockquote exactly. Added a new introductory sentence establishing the ceremony's relationship to the new pipeline, and appended a cross-reference to `sdkconfig.defaults`.

**Rationale:** The blessing ceremony is an architectural invariant (Decision D20 in the spec). It's the Application hub's responsibility, not the OTA manager's. Changing the wording could introduce confusion about ownership.

## Decision: Full OtaError Enum Table

**Context:** The OtaError enum has 16 values. Could have summarized to "see OtaError.hpp for details".

**Decision:** Included the complete table with all 16 error codes, their integer values, and descriptions.

**Rationale:** The design spec is the single source of truth for the framework's architecture. Cross-referencing to code headers defeats the purpose. The table occupies ~20 lines but provides complete error surface documentation — essential for app developers diagnosing OTA failures without reading source code.

## Decision: 236 Lines (Over 200 Target)

**Context:** WO4 specified ~150–200 lines target.

**Decision:** Delivered 236 lines.

**Rationale:** The 16-row error code table and 5-row REST API table account for ~25 extra lines. These tables are high-density information that would be worse as prose. The content is factual and architectural throughout — no tutorial padding.
