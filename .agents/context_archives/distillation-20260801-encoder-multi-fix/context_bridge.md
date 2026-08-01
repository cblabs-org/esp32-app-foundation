---
title: Encoder Module Multi-Encoder Architecture & Delta Pattern
distilled_at: 2026-08-01T15:10:00Z
original_plan_id: standalone
artifacts:
  - context_artifacts/adr_encoder_multi_support.md
  - context_artifacts/graveyard_encoder_tests.md
  - context_artifacts/state_encoder_module.md
---

## Executive Summary
The `EncoderModule` was refactored during PR #56 to natively support multiple encoders using the hardware's `SOC_PCNT_UNITS_PER_GROUP` as a maximum boundary, ensuring downstream consumers can safely reserve PCNT units. We also transitioned the PCNT hardware interface from a lossy `clear_count` model to a "delta-from-previous" pattern to prevent hardware pulse drops.

## Next Steps
- Verify PR #56 is merged cleanly into main.
- If moving onto a new input peripheral (like LVGL encoder bridging or a new I2C sensor), see `EncoderModule` implementation for the standard hardware boundary validation pattern.

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_encoder_multi_support.md` - Design decisions for `MAX_ENCODERS` vs dynamic allocation and `board_contract.h` limits
- `context_artifacts/graveyard_encoder_tests.md` - Why `clear_count` failed and how native tests were adapted with `stub_pcnt_add_count()`
- `context_artifacts/state_encoder_module.md` - The final `EncoderState` structure and `EncoderModule` multi-instance array API
