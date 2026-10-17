/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2GLDisplay/U8G2GLDisplay.hpp
 * Description: One-time start-up of the U8G2GL runtime for the firmware.
 *              U8G2GLDisplay does not own a display. The legacy
 *              U8G2DisplayModule keeps the panel, the draw loop, the carousel
 *              and the screensaver, until a later programme step replaces them
 *              (ruling R1 of U8G2GL PR-05, in
 *              .agents/skills/principal-architect-workspace/pr-u8g2gl-05/PROJECT.md);
 *              this module only makes the generated UI ready, so
 *              U8G2GLScreenBoard instances can run generated screens inside
 *              that carousel.
 *
 *              It does two things, once:
 *                1. registers millis() as the runtime's time source, and
 *                2. calls ui_init(), the generated function that initialises
 *                   every subject, style and screen of the project.
 *
 *              ui_init() must run exactly once. It binds every label to its
 *              subject, and a second call would register each binding again.
 *
 * Exported Classes:
 * - U8G2GLDisplay: Start-up of the U8G2GL runtime
 *   - begin():   Register the time source and run ui_init(), once (idempotent)
 *   - isReady(): Whether begin() has run
 */

#pragma once

/**
 * @brief Start-up of the U8G2GL runtime for this firmware.
 *
 * Static only: there is one runtime and one generated UI per firmware, so there
 * is nothing to instantiate.
 */
class U8G2GLDisplay {
public:
    /**
     * @brief Register the runtime's time source, then initialise the generated UI.
     *
     * Call once during setup, after the legacy display module has begun and
     * before any U8G2GLScreenBoard is configured: a board writes to subjects,
     * and a subject written before ui_init() has no observers yet, so the
     * screen would never learn of the change.
     *
     * Idempotent: a second call does nothing, because ui_init() is not safe to
     * run twice.
     */
    static void begin();

    /** @brief Returns true once begin() has run. */
    static bool isReady();

private:
    static bool _begun;  ///< Set by begin(); guards ui_init() against running twice
};
