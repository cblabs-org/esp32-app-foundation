# Graveyard: Encoder Delta Tracking & PCNT Hardware Quirks

## Failed Attempt 1: PCNT `clear_count` Pulse Loss
**What we tried:** The initial module implementation relied on reading the hardware PCNT counter via `pcnt_unit_get_count()` and immediately clearing it via `pcnt_unit_clear_count()` to compute deltas.
**Why it failed:** Data races. Any physical encoder pulses that arrive in the nanoseconds between `get_count` and `clear_count` are permanently lost. This resulted in dropped detents during fast rotation.
**Solution:** The "delta-from-previous" pattern. We track `_prevCount`, and compute delta as `currentCount - _prevCount`. `clear_count` is NEVER called in the `tick()` loop.

## Failed Attempt 2: Test Fixture Incompatibility with Delta-from-Previous
**What we tried:** When we migrated to the delta-from-previous pattern, the native tests failed. The tests were injecting absolute hardware counts using `stub_pcnt_set_count(4)` for *both* tick 1 and tick 2.
**Why it failed:** With the old `clear_count` logic, setting absolute counts between ticks worked because the hardware counter was supposedly 0. With delta-from-previous, setting `4` on tick 1 reads a delta of `4 - 0 = 4`. Setting `4` again on tick 2 reads a delta of `4 - 4 = 0`. The tests incorrectly appeared to stall.
**Solution:** We created `stub_pcnt_add_count(delta)` for the test fixtures. Multi-tick tests must now accumulate their injected counts identically to how the real hardware accumulators behave, ensuring `currentCount` continuously grows or shrinks.

## Failed Attempt 3: Flat MCP Diagnostic Structure
**What we tried:** The MCP tool `encoder_status` returned a flat JSON object for a single encoder.
**Why it failed:** Once we added multi-encoder support, this broke. 
**Solution:** The MCP tool now returns a JSON object containing an `"encoders"` JSON Array. The tests and IDE integrations were updated to target `result["encoders"][0]`.
