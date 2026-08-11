# Portal Module Tab — template

A starting scaffold for an **optional portal tab** contributed by a firmware
module at runtime (the `IWebContributor` → `tab-changed` fragment pattern).

- `ModuleTab.dc.html` — the tab: a live **Status** card, a **Settings** card
  (toggle + interval), and a save action. Composes the design-system components
  (`Card`, `InfoRow`, `ProgressBar`, `ToggleSwitch`, `Badge`, `Button`).
- `ds-base.js` — loads this design system. In a consuming project, edit the one
  `base` line to point at the bound `_ds/<folder>`.

## Use it
Copy the folder, rename `ModuleTab` to your module, and replace the status/
settings content. Keep to the conventions:
- Prefix all element IDs with your module name to avoid collisions in the shared
  `#tab-content` container.
- Use the 4-tier feedback policy: silent → `window.showToast()` → inline alert →
  reboot overlay. `save()` here demonstrates the toast tier.
- Read/write device state via the portal's `apiFetch` helper (stubbed here).

See the **Portal Extensibility** card in the Design System tab and
`framework-expert/systems/web-portal` for the full contract.
