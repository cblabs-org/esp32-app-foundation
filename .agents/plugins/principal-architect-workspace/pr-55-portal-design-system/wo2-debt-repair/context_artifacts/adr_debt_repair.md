# ADR: Debt Repair (WO2)

## CSS Components
- **`.card`:** Created to mirror `<article>` styles. `<article>` retained for backward compatibility.
- **`.alert`:** Unified error feedback box. Modifiers: `--success`, `--error`, `--warning`, `--info`.
- **Migrations:**
  - Migrated `.form-error` and `.login-error` directly to `.alert--error`.
  - Re-evaluated `.ap-banner`: Kept as a standalone class since its visibility is body-state driven (`body.ap-mode`), making it fundamentally structural rather than just a styling modifier.

## JS Architecture
- Extended `ApiQueue` to serialize both UI (`highQueue`) and Background tasks (`lowQueue`). UI requests immediately pre-empt background delays, keeping the SPA perfectly responsive.
