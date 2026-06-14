# State: Portal Drawer Components — Final CSS

Both components live in `web/portal/app.css` sections 9c and 9d.

## advanced-drawer (sections 9c) — Yellow warning zone
Used by: OTA tab Advanced card, Time tab Advanced card

```css
article.advanced-drawer {
  border-color: var(--warning, #f9a825);
  background: rgba(249, 168, 37, 0.04);
}
article.advanced-drawer > details > summary.advanced-summary {
  font-weight: 700; font-size: 1.1rem; color: var(--warning, #f9a825);
  cursor: pointer; list-style: none;
  display: flex; justify-content: space-between; align-items: center;
  padding: 0.25rem 0 0.75rem; user-select: none;
}
article.advanced-drawer > details > summary.advanced-summary::after {
  content: ''; display: inline-block;
  width: 5px; height: 5px;
  border-right: 2px solid currentColor;
  border-bottom: 2px solid currentColor;
  transform: rotate(-45deg); transition: transform 0.2s; flex-shrink: 0;
}
article.advanced-drawer > details[open] > summary.advanced-summary::after {
  transform: rotate(45deg);
}
article.advanced-drawer > details > summary.advanced-summary:focus-visible {
  outline: 2px solid var(--warning, #f9a825); outline-offset: 3px; border-radius: 2px;
}
article.advanced-drawer > details > summary.advanced-summary::-webkit-details-marker { display: none; }
@media (prefers-reduced-motion: reduce) {
  article.advanced-drawer > details > summary.advanced-summary::after { transition: none; }
}
```

## info-drawer (section 9d) — Neutral informational
Used by: WiFi tab Network Diagnostics

```css
article.info-drawer {
  border: 1px solid var(--border, #dee2e6);
  border-radius: 10px; overflow: hidden; margin-bottom: 0.75rem;
}
article.info-drawer > details > summary.info-summary {
  font-weight: 600; font-size: 0.85rem; color: var(--text-muted, #6c757d);
  cursor: pointer; list-style: none;
  display: flex; justify-content: space-between; align-items: center;
  padding: 0.6rem 0.75rem; user-select: none;
}
article.info-drawer > details > summary.info-summary::after {
  content: ''; display: inline-block;
  width: 5px; height: 5px;
  border-right: 1.5px solid currentColor;
  border-bottom: 1.5px solid currentColor;
  transform: rotate(-45deg); transition: transform 0.2s; flex-shrink: 0;
}
article.info-drawer > details[open] > summary.info-summary::after {
  transform: rotate(45deg);
}
article.info-drawer > details > summary.info-summary:focus-visible {
  outline: 2px solid var(--border, #dee2e6); outline-offset: 2px; border-radius: 2px;
}
article.info-drawer > details > summary.info-summary::-webkit-details-marker { display: none; }
article.info-drawer > details > .info-drawer-content { padding: 0 0.75rem 0.5rem; font-size: 0.8rem; }
@media (prefers-reduced-motion: reduce) {
  article.info-drawer > details > summary.info-summary::after { transition: none; }
}
```

## WiFi info-drawer HTML structure (wifi.js)
```html
<article class="info-drawer">
  <details id="wifi-diag-drawer">
    <summary class="info-summary">
      <span id="wifi-diag-dot" style="width:8px;height:8px;border-radius:50%;
        background:var(--border,#dee2e6);display:inline-block;
        margin-right:0.5rem;flex-shrink:0;"></span>
      Network Diagnostics
    </summary>
    <div class="info-drawer-content">
      <table id="wifi-diag-table"></table>
    </div>
  </details>
</article>
```
Note: `diagDot` still resolved by ID for colour updates. `diagTrigger`,
`diagContent`, `diagOpen` flag all deleted — native `<details>` replaces them.
