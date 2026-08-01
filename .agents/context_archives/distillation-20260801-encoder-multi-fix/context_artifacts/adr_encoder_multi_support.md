# ADR: Encoder Module Multi-Encoder Architecture

## Context
The ESP32-S3 (and other ESP-IDF SoCs) have a hard hardware limit on Pulse Counter (PCNT) units. For S3, this is 4 (`SOC_PCNT_UNITS_PER_GROUP`).
During PR #56 review, we needed to support multiple encoders without exhausting hardware resources or breaking API ergonomics.

## Decision
1. **Fixed Ceiling, Dynamic Claiming**: The `EncoderModule` internal array is statically sized to `SOC_PCNT_UNITS_PER_GROUP` (e.g., 4 on S3). However, it only claims the exact number of PCNT units passed into `addEncoder()`.
2. **Consumer Freedom**: If a board configures 2 encoders, the remaining 2 hardware PCNT units are completely untouched and free for the downstream consumer to claim directly via ESP-IDF (e.g., for flow sensors or motors).
3. **Compile-time Contract**: Added a `static_assert(BOARD_ENCODER_COUNT <= SOC_PCNT_UNITS_PER_GROUP)` to `board_contract.h`. This guarantees the board definition never asks for more encoders than the chip physically supports.

## Rationale
This approach avoids dynamic memory allocation (std::vector) for core framework state, maintaining deterministic memory usage (~36 bytes per slot, max 144 bytes). It safely protects downstream resources by ensuring `EncoderModule` never greedily consumes all PCNT units unless explicitly instructed to by `BOARD_ENCODER_COUNT`.
