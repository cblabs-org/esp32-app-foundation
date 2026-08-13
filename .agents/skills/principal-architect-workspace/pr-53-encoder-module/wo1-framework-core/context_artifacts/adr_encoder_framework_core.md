# ADR: EncoderModule Framework Core Architecture

## Status
Approved / Implemented (WO1)

## Context
Rotary encoder inputs are critical for hardware human-machine interfaces. Higher level application logic needs responsive quadrature decoding, physical detent normalisation, velocity-based acceleration for fast turning, thread-safe consumption, and diagnostic visibility via MCP.

## Key Design Decisions

1. **ESP-IDF PCNT Peripheral Integration**:
   - Uses ESP-IDF's raw pulse counter driver (`driver/pulse_cnt.h`) instead of poller loops or software components.
   - PCNT hardware counts quadrature signals autonomously on dedicated hardware channels, eliminating high-frequency GPIO interrupt overhead.

2. **Dual Atomic Accumulators & `consumeDelta`**:
   - `_rawDelta` and `_accelDelta` are stored as `std::atomic<int32_t>`.
   - `consumeDelta(EncoderDelta mode)` uses `exchange(0)` to read and reset the chosen accumulator atomically in a single instruction without locking or race conditions.

3. **Velocity-Based Quadratic Acceleration**:
   - Computed in `tick()` using inter-pulse interval (`elapsed = currentMillis - _lastMovementMs`).
   - Standard quadratic curve applied when `elapsed < deadZoneMs`: `multiplier = 1 + t^2 * (maxMultiplier - 1)` where `t = (deadZoneMs - elapsed) / (deadZoneMs - shortCutoffMs)`.
   - Allows fine precise single-detent stepping when turned slowly, while allowing rapid value jumps when spun quickly.

4. **EventBus Decoupled Rotations**:
   - `EncoderEvent` `{ id, delta, velocityMs }` published on `encoder.rotate`.
   - Uses fixed member ring buffer `EncoderEvent _eventRing[4]` to guarantee zero runtime heap allocations during publication (GRAVE-05 compliance).

5. **PR-41 Native Build Stubbing**:
   - Avoids inline `#ifdef NATIVE_BUILD` conditional blocks in header and source code.
   - Provides native stub header `test/stubs/platform/driver/pulse_cnt.h` exposing identical ESP-IDF C API function signatures and a test hook (`g_stub_pcnt_count`).
