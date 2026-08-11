---
name: esp32-app-foundation-design
description: Use this skill to generate well-branded interfaces and assets for the ESP32 App Foundation device configuration portal, either for production or throwaway prototypes/mocks/etc. Contains essential design guidelines, colors, type, fonts, assets, and UI kit components for prototyping.
user-invocable: true
---

Read the README.md file within this skill, and explore the other available files.

If creating visual artifacts (slides, mocks, throwaway prototypes, etc), copy assets out and create static HTML files for the user to view. If working on production code, you can copy assets and read the rules here to become an expert in designing with this brand.

If the user invokes this skill without any other guidance, ask them what they want to build or design, ask some questions, and act as an expert designer who outputs HTML artifacts _or_ production code, depending on the need.

## Fast facts
- This is the **web portal served from an ESP32 chip** — mobile-first, 480px column, size-constrained (HTML/CSS/JS embedded in firmware flash). Keep output small: system fonts, inline SVG, no frameworks.
- One accent (Material Blue `#2196F3`) + semantic green/red/amber + neutral greys. Light + `[data-theme="dark"]`.
- No logo — the name is set in type. Never invent one.
- Rebrand by overriding `--primary` and the title text; every fragment reads the semantic tokens in `styles.css`.
- Icons: Feather (2px stroke, 24px, currentColor) in `assets/icons/`.
- Destructive actions use armed / type-to-confirm patterns; feedback is short and factual.
