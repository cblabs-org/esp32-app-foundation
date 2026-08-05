---
name: pr-66-wo6
description: Handoff instructions for Work Order 6 — SSOT elimination and buffer fix
complexity: standard
---

# WO6 — SSOT Elimination + Buffer Fix (F3, F4)

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Eliminate the duplicated gate-resolution logic in `ConfiguringStatusBoard::render()` (the SSOT violation) and increase the `GenericGateHelperBoard` help text buffer.

## Parent Project

`pr-66-configuring-orchestration` — PR [#68](https://github.com/matthewmcneill/esp32-app-foundation/pull/68)

## Strict Boundary Box

- **DO** modify: `modules/U8G2Display/boards/ConfiguringStatusBoard.hpp`, `modules/U8G2Display/boards/ConfiguringStatusBoard.cpp`, `modules/U8G2Display/boards/GenericGateHelperBoard.hpp`, `modules/U8G2Display/U8G2DisplayModule.cpp`, `test/test_unit/test_u8g2Display/test_boards.cpp`
- **DO NOT** modify Application.cpp, Application.hpp, IConfigGate.hpp, or any kernel files
- **DO NOT** create branches — commit directly on the current branch

## Tooling Constraints

Use `platformio` MCP tools for any build verification. Do not use raw `pio` CLI commands.

## Line-Item Instructions

- [ ] **F3a: Add `setActionableGateName()` to `ConfiguringStatusBoard.hpp`**

  Add a new public method declaration:

  ```cpp
  /**
   * @brief Set the authoritative actionable gate name from the Application.
   *
   * Called by U8G2DisplayModule::_onAppConfiguring() alongside
   * setActiveGateStatus(). Eliminates the need for render() to
   * re-derive the actionable gate by re-walking the dependency graph.
   *
   * @param gateName  Gate name from AppConfiguringPayload::gateName (non-owning).
   */
  void setActionableGateName(const char* gateName);
  ```

  Add a public test accessor:

  ```cpp
  /** Return the stored actionable gate name (for testing). */
  const char* getActionableGateName() const { return _actionableGateName; }
  ```

  Add a new private member variable:

  ```cpp
  const char* _actionableGateName = nullptr;  ///< Authoritative actionable gate name (from Application via event)
  ```

- [ ] **F3b: Implement setter in `ConfiguringStatusBoard.cpp`**

  Add the implementation:

  ```cpp
  void ConfiguringStatusBoard::setActionableGateName(const char* gateName) {
      _actionableGateName = gateName;
  }
  ```

- [ ] **F3c: Simplify `render()` in `ConfiguringStatusBoard.cpp`**

  In the `render()` method, find the block that computes `actionableGateName` by walking the dependency graph. This is the section with comments about "compute the actionable gate name from the gate list itself" and the nested for-loops checking dependencies with `getDependencies()` and `strcmp()`.

  Replace that entire block with:

  ```cpp
  // Actionable gate name is authoritative — set by _onAppConfiguring
  // from Application::getActionableGate(). No dependency walk needed (F3: SSOT).
  const char* actionableGateName = _actionableGateName;
  ```

  The block to delete starts with:
  ```cpp
  // --- Count satisfied gates and determine actionable gate ---
  // We compute the actionable gate name from the gate list itself for
  ```
  And ends just before `// --- Increment animation frame for InProgress effect ---`

  Keep the `satisfiedCount` variable and its counting loop — only remove the `actionableGateName` computation and the nested dependency-walk loops. The counting of `satisfiedCount` should remain in a simple loop:

  ```cpp
  int satisfiedCount = 0;
  for (int i = 0; i < gateCount; i++) {
      if ((*_gates)[i]->isSatisfied()) {
          satisfiedCount++;
      }
  }
  ```

- [ ] **F3d: Feed `setActionableGateName()` in `U8G2DisplayModule.cpp`**

  In `_onAppConfiguring()`, find the line:
  ```cpp
  self->boards.configuringStatus.setActiveGateStatus(p->actionStatus);
  ```

  Add immediately after it:
  ```cpp
  self->boards.configuringStatus.setActionableGateName(p->gateName);
  ```

- [ ] **F4: Increase GenericGateHelperBoard buffer in `GenericGateHelperBoard.hpp`**

  Find the private member declarations:
  ```cpp
  char _gateName[32]  = "";   ///< Gate name for title display (max 31 chars + null)
  char _helpText[64]  = "";   ///< Help text body (max 63 chars + null)
  ```

  Replace with:
  ```cpp
  char _gateName[32]  = "";   ///< Gate name for title (max 31 chars + null). Framework gate names are short by convention.
  char _helpText[128] = "";   ///< Help text body (max 127 chars + null — room for LVGL-width gates)
  ```

- [ ] **F3e: Add tests for `setActionableGateName()` in `test_boards.cpp`**

  In the `ConfiguringStatusBoardTest` group, add these tests:

  ```cpp
  /** setActionableGateName stores the name for render() comparison. */
  TEST_F(ConfiguringStatusBoardTest, SetActionableGateName_StoresName) {
      board.setActionableGateName("WiFi Station");
      EXPECT_STREQ(board.getActionableGateName(), "WiFi Station");
  }

  /** setActionableGateName with nullptr is safe. */
  TEST_F(ConfiguringStatusBoardTest, SetActionableGateName_Nullptr_IsSafe) {
      board.setActionableGateName("Test");
      board.setActionableGateName(nullptr);
      EXPECT_EQ(board.getActionableGateName(), nullptr);
  }
  ```

## Verification

After making all changes, compile with the PlatformIO MCP `build_project` tool for `esp32dev` environment to verify no build errors.

## Commit

Once verified, commit with message:
```
fix(display): eliminate SSOT violation in ConfiguringStatusBoard, increase gate helper buffer
```
