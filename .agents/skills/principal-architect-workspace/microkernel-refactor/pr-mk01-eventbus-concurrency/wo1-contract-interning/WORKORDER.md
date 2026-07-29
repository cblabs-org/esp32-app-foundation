---
name: pr-mk01-eventbus-concurrency-wo1
description: Handoff instructions for Work order 1
complexity: standard
---

# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Define the concurrency contract for `EventBus`, and replace `std::string` topics with interned identity (e.g. `uint32_t` hash) to remove per-publish heap allocations, while keeping the public interface stable where possible. Implement in-place dispatch over the subscribers without copying the entire vector.

# Strict Boundary Box
- Touch **only** `modules/EventBus/*` (i.e. `EventBus.cpp` and `EventBus.hpp`).
- Provide minimal updates to publisher/subscriber call sites only if forced by topic intern changes (ideally keep `const char*` externally, intern internally).
- **Do not** begin module extraction, the boot seam, or composition work.
- **Do not** change `Application`'s module ownership.
- Surface any findings out of scope in your completion report; do not file issues or make out-of-scope changes.

# Tooling Constraints
Use the `platformio` MCP tools (`build_project`, `run_tests`) — **not** raw `pio` CLI. Native test env: `native`. Verify both `native` and at least one hardware env (`esp32dev`) compile.

# Checklist
- [x] Read `docs/microkernel-architecture-design.md` for context (specifically MK-11, MK-12).
- [x] Define the concurrency contract as a docstring comment in `EventBus.hpp`.
- [x] Modify `EventBus::subscribe`, `EventBus::unsubscribe`, and `EventBus::publish` to avoid copying the `_subscribers` vector (e.g. iterate in place, handle re-entrancy / self-unsubscription gracefully).
- [x] Replace `std::string topic` inside `Subscription` struct with an interned identity (e.g. `uint32_t topicHash` generated using a constexpr hash of the string, or at runtime). The caller should still be able to pass `const char*`.
- [x] Check if publisher/subscriber calls in `modules/EventBus` need adaptation.
- [x] Verify compilation on `esp32dev` env using `call_mcp_tool` with `platformio` `build_project`.
- [x] Verify native tests on `native` env using `call_mcp_tool` with `platformio` `run_tests`.
- [x] Ask the user for approval. Only after explicit approval, invoke `@distillery` to package the output targeting `.agents/skills/principal-architect-workspace/microkernel-refactor/pr-mk01-eventbus-concurrency/wo1-contract-interning/`.
- [x] Append a status update to this `WORKORDER.md` file.

# Status Log

| Date | Entry |
|---|---|
| 2026-07-29 | **COMPLETED & VALIDATED.** Concurrency contract docstring added; topic interning implemented via FNV-1a 32-bit hash; in-place dispatch with soft-deletion tombstones for re-entrancy safety. Hardware build (`esp32dev`) and 489 native unit tests passed. User approved execution. Distillery output generated at `.agents/skills/principal-architect-workspace/microkernel-refactor/pr-mk01-eventbus-concurrency/wo1-contract-interning/`. |

