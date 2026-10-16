# web/portal

## Invariants & contracts

### [docseed-0f58107f] **Modifiers:** Use double-hyphens (`block--modifier` or `block__element--modifier`) to indicate variant states or types (e.g., `.badge--ok`, `.toast--success`, `.rag--green`, `.tab-content--active`, `.login-overlay--hidden`).
`invariant · kind: semantic · anchors: web/portal/app.css · source_doc: docs/developer/adr/adr-css-naming-convention.md · evidence: doc:docs/developer/adr/adr-css-naming-convention.md`

### [docseed-26ab3648] **Benefits:** Eliminates class collision risks, establishes clear conventions for future RFC #67 design system components, and ensures consistent state manipulation via BEM modifiers across all web portal modules.
`contract · kind: semantic · anchors: web/portal/app.css · source_doc: docs/developer/adr/adr-css-naming-convention.md · evidence: doc:docs/developer/adr/adr-css-naming-convention.md`

### [docseed-4c92f07a] **Updates Required:** All JavaScript files (`app.js`, `utils.js`, `auth.js`, `system.js`, `wifi.js`, `display.js`, `screenshot.js`), HTML markup (`index.html`), developer documentation, and Playwright integration tests updated.
`invariant · kind: semantic · anchors: web/portal/app.css · source_doc: docs/developer/adr/adr-css-naming-convention.md · evidence: doc:docs/developer/adr/adr-css-naming-convention.md`

### [docseed-57b14b96] **Element selectors:** `article`, `dialog`, `button` (base element resets)
`invariant · kind: semantic · anchors: web/portal/app.css · source_doc: docs/developer/adr/adr-css-naming-convention.md · evidence: doc:docs/developer/adr/adr-css-naming-convention.md`

### [docseed-5bd0d3af] **Static Asset Footprint**: Static web portal assets are compiled into PROGMEM gzip byte arrays by `scripts/portalBuilder.py`. The canonical asset inventory is maintained in `web/portal/` and bundled into `include/portal_assets/` during build time.
`invariant · kind: semantic · anchors: scripts/portalBuilder.py, web/portal · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-5ff8b8e5] **Elements & Blocks:** Standard block names (`.status-badge`, `.info-row`, `.compact-row`) and element reset selectors (`article`, `dialog`, `button`) are exempt from modifier renaming as they represent standard BEM blocks or base HTML element resets.
`invariant · kind: semantic · anchors: web/portal/app.css · source_doc: docs/developer/adr/adr-css-naming-convention.md · evidence: doc:docs/developer/adr/adr-css-naming-convention.md`

### [docseed-62cf4a46] **Single-class modifiers for state:** Compound selectors such as `.dot.green` become single BEM modifier classes `.dot--green` applied alongside base block classes (`<span class="dot dot--green">`).
`invariant · kind: semantic · anchors: web/portal/app.css · source_doc: docs/developer/adr/adr-css-naming-convention.md · evidence: doc:docs/developer/adr/adr-css-naming-convention.md`

### [docseed-6342cafb] **Compound/state classes:** `.dot.green`, `.tab-content.active`, `.pill-btn.active`, `.login-overlay.hidden`
`invariant · kind: semantic · anchors: web/portal/app.css · source_doc: docs/developer/adr/adr-css-naming-convention.md · evidence: doc:docs/developer/adr/adr-css-naming-convention.md`

### [docseed-98985e5a] **Hyphen-suffix variants:** `.badge-ok`, `.toast-success`, `.rag-green`, `.btn-delete`
`invariant · kind: semantic · anchors: web/portal/app.css · source_doc: docs/developer/adr/adr-css-naming-convention.md · evidence: doc:docs/developer/adr/adr-css-naming-convention.md`

### [docseed-9c35cccc] **Prefixed components:** `.status-badge`, `.info-row`, `.compact-row`
`invariant · kind: semantic · anchors: web/portal/app.css · source_doc: docs/developer/adr/adr-css-naming-convention.md · evidence: doc:docs/developer/adr/adr-css-naming-convention.md`

### [docseed-c3484375] **Renamed Classes:** 23 non-conforming legacy CSS classes across `app.css` have been refactored to BEM modifier syntax (`--`).
`invariant · kind: semantic · anchors: web/portal/app.css · source_doc: docs/developer/adr/adr-css-naming-convention.md · evidence: doc:docs/developer/adr/adr-css-naming-convention.md`

### [docseed-ca11ebbb] **Standalone block names:** `.badge`, `.dot`, `.toast`, `.container`
`invariant · kind: semantic · anchors: web/portal/app.css · source_doc: docs/developer/adr/adr-css-naming-convention.md · evidence: doc:docs/developer/adr/adr-css-naming-convention.md`

### [docseed-feaa0b1b] **Legacy Migration:** All ~24 existing non-conforming classes (hyphen-suffix variants and compound state classes) are migrated to BEM modifier syntax in a single breaking migration (WO1), updating all associated CSS definitions, HTML templates, JS dynamic class mutations, documentation, and Playwright tests.
`invariant · kind: semantic · anchors: web/portal/app.css · source_doc: docs/developer/adr/adr-css-naming-convention.md · evidence: doc:docs/developer/adr/adr-css-naming-convention.md`

## Ruled out, failure signatures and gotchas

### [seed-7018411d] Worker used `pio run` CLI instead of the `platformio` MCP `build_project` tool, violating rule 04 and the WORKORDER Tooling Constraints. PA re-verified via MCP `build_project` after worker completion. Future WORKORDERs should reinforce MCP constraints in the Line-Item Instructions section.
`ruled-out · kind: episodic · anchors: web/portal/index.html, web/portal/app.js`

### [seed-7c4e3aa1] None. Initial build verification timed out on initial cold build, resolved by background execution (`background: true`).
`ruled-out · kind: episodic · anchors: scripts/portalBuilder.py, web/portal, web/display, web/portal, web/display`

### [seed-b77a2383] None. All optional module scripts (`ota.js`, `mcp.js`, `display.js`, `time.js`, `homeassistant.js`) are now dynamically discovered via `/api/tabs`.
`ruled-out · kind: episodic · anchors: web/portal/app.js, web/display/display.js, test/web/server.js`

## Decisions

### [seed-6963d3bb] Updated mock web server (`test/web/server.js`) so Playwright web test suite correctly locates relocated asset files during `build_project` pre-build actions.
`decision · kind: episodic · anchors: test/web/server.js, web/portal, web/display`

### [seed-ea52b7b7] None — all changes followed the WORKORDER specification exactly.
`decision · kind: episodic · anchors: web/portal/index.html, web/portal/app.js`

## Open issues

### [seed-93ec9f5b] None. Web application tests run during full PlatformIO test suites will need WO2 route updates to serve moved assets from module endpoints.
`open-issue · kind: semantic · anchors: scripts/portalBuilder.py, web/portal, web/display, web/portal, web/display`
