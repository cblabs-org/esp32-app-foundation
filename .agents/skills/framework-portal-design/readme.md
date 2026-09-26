# ESP32 App Foundation — Design System

A design system for the **web configuration portal** of the ESP32 App Foundation
firmware framework. The portal is a single-page app served *directly from an
ESP32 chip* over HTTP: users connect to the device to configure WiFi, view
telemetry, manage security, and push OTA firmware updates.

Every design decision here is shaped by that constraint. The portal's HTML/CSS/JS
is embedded in the firmware as C++ raw-string literals and must fit in flash
alongside the application, so the system is **deliberately minimal**: zero
webfonts (native system stack), inline SVG icons, no CSS framework, a single
small stylesheet, and a token layer built for downstream **rebranding** (change
`--primary` + the title and the whole portal reskins).

This is a mobile-first product — the portal is a **480px phone-width column**,
even on desktop.

## Sources

This system was reverse-engineered from real product code — build against these,
not from memory:

- **Portal codebase** (ground truth): `web/portal/` — `index.html`, `app.css`
  (the ~1,200-line design system), and the core per-tab fragments (`wifi.js`,
  `system.js`, `auth.js`, `credits.js`, …). **Module-owned tabs are not in
  `web/portal/`** — they live beside their module as `modules/<Name>/web/<name>.js`
  (e.g. `modules/OTAUpdateManager/web/ota.js`, `modules/MCPServer/web/mcp.js`), and a
  new module's tab goes there too; `scripts/portalBuilder.py` discovers them. All token values, radii,
  spacing and component styling in this system are lifted verbatim from `app.css`.
- **Framework skill** (context): `mm-skills/framework-expert`
  → https://github.com/mm-skills/framework-expert — the firmware framework docs.
  Especially `systems/web-portal/` (SPA architecture, `IWebContributor` tab
  contribution, `tab-changed` fragment contract, and the shared `utils.js`
  helpers: `apiFetch`, `showToast`, `showRebootOverlay`, `armButton`,
  `renderSignalBars`, `renderProgressBar`, …).

> Explore `framework-expert` further to design new portal tabs correctly: every
> tab is a module implementing `IWebContributor` that injects its own markup into
> a container div on a `tab-changed` event. New UI should follow that fragment
> pattern and the four-tier save policy (toast vs. reboot-overlay feedback).

No logo asset exists in the sources — see **Brand** below.

---

## CONTENT FUNDAMENTALS

**Voice.** Terse, technical, reassuring. Written for a maker/engineer configuring
their own hardware. No marketing language, no exclamation, no personality quirks.

- **Casing.** Titles and headers are **Title Case** ("WiFi Settings", "Available
  Networks", "Danger Zone", "Check for Updates"). Bottom-nav labels and small
  section eyebrows are **UPPERCASE** with letter-spacing ("WIFI", "SECURITY",
  "INTERNAL DRAM"). Body and descriptions are sentence case.
- **Buttons** are imperative verb phrases: "Scan", "Save & Connect", "Test
  Connection", "Forget Network", "Rollback to Previous", "Refresh Bundle".
- **Person.** The device is third person ("The device will restart", "allow
  firmware to be applied to **this device**"). The user is addressed implicitly
  via imperatives; occasional direct "you"/"your" in help text ("Check your
  settings"). Never "we".
- **Destructive actions** get progressive friction, and the copy escalates with
  it: an armed button ("Confirm Restart?", "⚠ Confirm Forget? (3s)") for
  reversible actions; type-to-confirm ("Type **RESET** to confirm") for
  irreversible ones. Danger zones are labelled literally ("Danger Zone").
- **Feedback** is short and factual. Success: "Configuration saved.",
  "Channel set to beta". Error: "Connection failed. Check password and try
  again.", "Upload timed out after 120 seconds." Reboot overlays explain the
  wait: "Applying settings…" / "The device will restart."
- **Emoji / symbols.** Essentially none as decoration. Only functional glyphs:
  ⚠ for danger arming, ✓ / ✗ for inline pass/fail results. Do not add emoji.
- **Spelling.** Source code uses **British spelling** in comments/identifiers
  ("colour", "optimisation", "initialise"). User-facing copy is neutral.
- **Numbers & units.** Always unit-suffixed and human-formatted: "142 KB",
  "2d 5h 13m", "-47 dBm (Excellent)", "48.2°C", "v2.4.1". RSSI is paired with a
  qualitative label (Excellent / Good / Fair / Weak).

---

## VISUAL FOUNDATIONS

**Overall vibe.** Clean, dense, utilitarian "settings app". Card-based, flat,
high-contrast, no ornamentation. It reads like a well-built native mobile
preferences screen, not a website.

- **Colour.** One brand accent — **Material Blue `#2196F3`** (`--primary`, hover
  `#1976D2`) — used for the title, active nav/pills, primary buttons, focus
  rings, and progress fills. Three semantic colours: success green `#2e7d32`,
  error/danger red `#c62828`, warning amber `#f9a825` (also the "advanced/expert"
  accent). Everything else is neutral greys. **Two themes** (light default +
  `[data-theme="dark"]`); an auto/light/dark toggle lives in the top bar. Max one
  accent + neutrals per screen — colour is reserved for state and action.
- **Type.** Native system sans stack (`-apple-system, "Segoe UI", Roboto…`) —
  **no webfonts shipped**. `"Courier New"` monospace for MACs, IPs, RSSI, logs,
  and code. Scale runs 0.65rem (nav labels) → 1.5rem (title). Weights lean heavy:
  600–700 for labels/headers, **800** for the title only. The title carries tight
  `-0.5px` tracking; uppercase labels carry positive `0.3–0.5px` tracking.
- **Backgrounds.** Flat solid fills only. `--bg` behind the page, `--card-bg`
  (a hair off-white / off-black) for surfaces. **No gradients, no images, no
  patterns, no texture, no blur** — except overlay scrims (see below).
- **Cards.** The core surface: `--card-bg`, **1px** hairline border (`--border`),
  **16px** radius, `1.25rem` padding, `1rem` bottom margin. Flat by default (no
  resting shadow). Variants recolour the border + header: `danger` (red, tinted
  `rgba(198,40,40,.04)`), `advanced` (amber, tinted `rgba(249,168,37,.04)`).
  Interactive cards lift `translateY(-2px)` with a soft shadow on hover; empty
  cards use a 2px dashed border.
- **Corner radii.** Soft and consistent: 6px (progress tracks, micro badges),
  8px (alerts/callouts/terminal), 10px (inputs, buttons, drawers), 12px
  (banners, dialogs, status header), 16px (cards), 20px (pills), fully round
  (toggle switches, status dots, theme knobs).
- **Borders & shadows.** Structure comes from **hairline borders**, not shadows.
  The system is shadow-light: elevation appears only on hover
  (`0 4px 12px rgba(0,0,0,.1)`, deeper in dark), on the toggle/theme knobs
  (`0 1px 3px`), and on floating surfaces — dialogs (`0 10px 25px`), login/reboot
  cards (`0 8px 32px`).
- **Transparency & blur.** Only for full-screen overlays: scrims at
  `rgba(0,0,0,.6)` (dialog) → `.85` (reboot), with `backdrop-filter: blur(4–8px)`.
  Tints (`color-mix` / low-alpha rgba) fill alert and danger-zone backgrounds.
- **Animation.** Quick and functional, `ease`, 0.15–0.5s. Tab content
  `slideUp` (fade + 10px rise, 0.3s). Buttons shrink to `scale(0.98)` on press.
  Toggles/toasts slide 0.3s. Progress fills transition width 0.5s. Status dots
  can `pulse` (scale 1→1.3, 1s loop). Spinners rotate 0.8s linear. Honours
  `prefers-reduced-motion`.
- **Hover / press states.** Buttons: press = `scale(0.98)` + slight opacity drop
  (no colour change). Outline/pill/secondary: border + text go `--primary` on
  hover. Icon buttons: `--primary-alpha` background wash on hover. Nav/pills:
  active = solid blue fill (pill) or blue text + 2px icon rise (nav).
- **Layout rules.** Fixed 480px max-width column, centred. Bottom navigation is
  `position: fixed`, 65px tall, respecting `env(safe-area-inset-bottom)`; body
  has 90px bottom padding to clear it. Sub-pills scroll horizontally (hidden
  scrollbar). Toasts dock bottom-centre above the nav; overlays cover everything.
- **Imagery.** None. There is no photography or illustration — the product is
  chrome and data. Iconography is the only visual vocabulary.

---

## ICONOGRAPHY

- **System.** [Feather Icons](https://feathericons.com) — inline SVG, `24×24`
  viewBox, **stroke-only** (`stroke-width: 2`, round caps/joins, `fill: none`),
  coloured via `currentColor`. No icon font, no PNG icons, no emoji-as-icons.
- Icons are embedded inline in the firmware's HTML/JS (they cost bytes in flash,
  so the set is small and reused). The real set used by the portal is extracted
  to `assets/icons/`: `wifi`, `lock`, `settings` (gear), `info`, `sun`, `moon`,
  `monitor` (auto-theme), `eye`, `eye-off`, `x` (close), `copy`.
- Bottom-nav icons: WiFi arcs, padlock (Security), gear (System). App-contributed
  tabs supply their own inner-SVG path via `registry.addTab(…, icon, …)` in
  `registerTabs()`; otherwise a generic grid icon is used.
- **Substitution note:** icons are the genuine Feather paths from the codebase —
  no substitution was needed. Match new icons to Feather's 2px-stroke style.

---

## BRAND

**There is no logo.** The sources ship no logo/mark asset. The product identity
is purely typographic: the name **"ESP32 App Foundation"** is set in the system
sans at `1.5rem / 800`, tight `-0.5px` tracking, in `--primary` (blue), top-left
in the header. Downstream products rebrand by overriding the title text and the
`--primary` token. Do **not** invent or draw a logo. See `guidelines/brand-*`.

---

## Foundations & tokens

- `styles.css` — the single entry point consumers link. Import manifest only.
- `tokens/colors.css` — base palette + the semantic contract (`--primary`, `--bg`,
  `--card-bg`, `--text`, `--text-muted`, `--border`, `--success/error/warning`, …)
  + the `[data-theme="dark"]` scope.
- `tokens/typography.css` — families (no `@font-face` by design), scale, weights.
- `tokens/spacing.css` — rem spacing scale + layout constants (480px, 65px nav).
- `tokens/radii.css` — the radius scale.
- `tokens/effects.css` — shadows, scrims, blur, transitions, press-scale.
- `tokens/motion.css` — shared `@keyframes` (`af-spin`, `af-pulse`, `af-slide-up`).

Specimen cards live in `guidelines/` (Colors, Type, Spacing, Effects, Brand).

## Components

React primitives in `components/<group>/`, each with a `.d.ts` contract,
`.prompt.md` usage note, and a showcase card. Styling references the CSS tokens.

- **Forms** — `Button` (primary / outline / secondary / danger, busy, disabled),
  `IconButton`, `Input`, `Select`, `Textarea`, `ToggleSwitch`,
  `PasswordInput` (eye toggle).
- **Data Display** — `Card` (default / danger / advanced), `StatusDot`, `Badge`
  (ok / warn / subtle / mode), `ProgressBar` (RAG), `SignalBars`, `InfoRow`,
  `DiagTable`, `TerminalLog`.
- **Feedback** — `Alert`, `Callout`, `Toast`, `Dialog`, `RebootOverlay`,
  `LoginCard`.
- **Navigation** — `PillBar`, `ThemeToggle`, `Drawer` (advanced / info).

### Intentional additions
`IconButton` and `Textarea` correspond to `app.css` classes (`.btn-icon`,
`textarea`) used across fragments but not previously componentized — added for
completeness against the source, not invented.

## Templates

`templates/<slug>/` folders consuming projects copy directly:

- **`templates/module-tab/`** — scaffold for an optional portal tab a firmware
  module contributes at runtime (`IWebContributor` pattern). See the
  **Portal Extensibility** guideline card for the contract.

## UI kits

- **`ui_kits/portal/`** — interactive recreation of the full device portal
  (WiFi, System status, OTA, Security), with the real bottom-nav + sub-pill
  navigation, armed/type-to-confirm flows, theme switching and toasts. See its
  own README.

> **Why only the core tabs?** WiFi/System/Security are the *mandatory* tabs the
> firmware always serves. Every other tab (OTA aside) is an *optional module*
> that injects itself at runtime — represented by the `templates/module-tab/`
> scaffold and the Portal Extensibility card rather than hard-coded here.

## Index / manifest (root)

| Path | What |
|---|---|
| `styles.css` | Consumer entry point (imports all tokens) |
| `tokens/` | colors, typography, spacing, radii, effects, motion |
| `guidelines/` | Foundation specimen cards (Colors/Type/Spacing/Effects/Brand) |
| `components/forms/` | Button, Input, Select, ToggleSwitch, PasswordInput |
| `components/data-display/` | Card, StatusDot, Badge, ProgressBar, SignalBars, InfoRow |
| `components/feedback/` | Alert, Callout, Toast, Dialog |
| `components/navigation/` | PillBar, ThemeToggle, Drawer |
| `assets/icons/` | Feather SVG icon set used by the portal |
| `ui_kits/portal/` | Interactive portal recreation |
| `thumbnail.html` | Design-system homepage tile |
| `SKILL.md` | Agent Skill wrapper (for use in Claude Code) |
| `github.md` | Source-repo association for one-click sync |

## Rebranding downstream

The portal is built to be reskinned. To brand it for a product: override
`--primary` (and `--primary-hover`, `--primary-alpha`), swap the header title
text, and optionally adjust `--card-bg`/`--border`. Because every fragment reads
the semantic tokens, that reskins the entire portal with no component changes.
