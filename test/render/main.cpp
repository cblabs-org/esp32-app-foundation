/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/render/main.cpp
 * Description: Host render harness. Renders a foundation display board through
 *              the real U8G2 library (real C sources, real fonts) into a
 *              256x64 SSD1322 full buffer with fixed inputs, then writes the raw
 *              buffer, a PBM image and an inputs record per scenario. The first
 *              render target is the legacy DiagnosticsBoard, compiled unmodified
 *              from modules/U8G2Display; further targets register in kTargets
 *              and are selected with --board. The second target, u8g2gl, is the
 *              migrated board: modules/U8G2GLDisplay's DiagnosticsScreenBoard
 *              running the generated U8G2GL screen, compiled from the committed
 *              generated/ UI and the pinned runtime. Its output can be compared
 *              byte for byte with the legacy baseline (--compare).
 *
 *              The platform the board reads (WiFi, millis, heap, hostname) is the
 *              set of shims in test/render/shim, driven through HostEnvironment.
 *
 * Exported Functions:
 * - main(): Parse --board / --out / --ascii / --probe-separator / --compare /
 *           --probe-state / --probe-clip, render every scenario of the chosen
 *           target, write its files, and optionally compare them with another
 *           output root.
 *
 * Usage:
 *   render_harness --board legacy --out <dir>
 *   render_harness --board legacy --ascii connected
 *   render_harness --board u8g2gl --out <dir> --compare <baseline-root> --probe-state --probe-clip
 */

// --- Standard and U8G2 headers first, so the access hack below touches only the board ---
#include <U8g2lib.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "HostEnvironment.hpp"
#include "WifiManager.hpp"

// The harness reads back the exact strings the board formatted (_wifiLine and
// each LabelWidget's _textBuffer) rather than re-deriving them with a second copy
// of the snprintf formats. Those members are private, so this translation unit
// alone widens access while it includes the board headers. In practice (GCC and
// Clang) access specifiers do not change object layout, though the standard does
// not promise it before C++23, so DiagnosticsBoard.cpp (compiled normally) and
// this file agree on the layout.
#define private public
#include "boards/DiagnosticsBoard.hpp"
#undef private

#include "fonts/Fonts.hpp"

#ifdef HARNESS_HAS_U8G2GL
#include "U8G2GLDisplay.hpp"
#include "boards/DiagnosticsScreenBoard.hpp"
#include "ui.hpp"  // Generated: ui_screen_diagnostics, ui_subject_diag_*
#endif

namespace fs = std::filesystem;

// =============================================================================
// Constants — what is being rendered, and from what
// =============================================================================

static constexpr int DISPLAY_WIDTH  = 256;   // Panel width in pixels (SSD1322 NHD 256x64)
static constexpr int DISPLAY_HEIGHT = 64;    // Panel height in pixels
static constexpr int ROW2_TOP = 16;          // First pixel row of row 2, the WiFi line (legacy y)
static constexpr int ROW3_TOP = 30;          // First pixel row of row 3, the uptime (legacy y); row 2 ends above it
static constexpr int ROW2_CLIP_X0 = 4;       // Row 2's first drawable column (legacy x)
static constexpr int ROW2_CLIP_X1 = 251;     // Row 2's far clip corner, x + w - 1 = 4 + 248 - 1 (exclusive in U8G2)
static constexpr const char* LEGACY_SOURCE_COMMIT = "e5342373";  // Commit whose DiagnosticsBoard is rendered
static constexpr const char* DISPLAY_SETUP_CALL =
    "u8g2_Setup_ssd1322_nhd_256x64_f(u8g2, U8G2_R0, u8x8_byte_empty, u8x8_dummy_cb)";

// Provided by CMake so the record states what was actually built.
#ifndef U8G2_PINNED_VERSION
#define U8G2_PINNED_VERSION "unknown"
#endif
#ifndef U8G2_SOURCE_DESCRIPTION
#define U8G2_SOURCE_DESCRIPTION "unknown"
#endif
#ifndef HARNESS_DEFAULT_BUILD_VERSION
#define HARNESS_DEFAULT_BUILD_VERSION "unknown"
#endif
#ifndef HARNESS_U8G2_FLAGS
#define HARNESS_U8G2_FLAGS "unknown"
#endif

// =============================================================================
// Display — the firmware's U8G2 object, minus the SPI pins
// =============================================================================

/**
 * @brief Which of the legacy module's begin-time U8G2 calls to reproduce.
 *
 * The default is the legacy state, exactly. The other values exist for the
 * u8g2gl target's probes, which ask what a screen needs that a plain U8G2GL
 * user, who has made none of these calls, would not have.
 */
struct DisplayState {
    bool fontPosTop = true;        ///< setFontPosTop(), U8G2Display.cpp:146
    bool fontRefHeightAll = true;  ///< setFontRefHeightAll(), U8G2Display.cpp:145
    bool legacyCalls = true;       ///< false: only begin(), as a plain U8G2GL user has
};

/**
 * @brief The firmware's U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI, with null I/O.
 *
 * Same setup call as that class (U8g2lib.h: u8g2_Setup_ssd1322_nhd_256x64_f), so
 * the buffer geometry and font engine are identical; only the SPI byte and GPIO
 * callbacks differ, and they are null because nothing here talks to a panel.
 */
class HostDisplay : public U8G2 {
public:
    /**
     * @brief Set up the SSD1322 256x64 full buffer and reproduce the legacy begin state.
     *
     * The state calls mirror U8G2Display.cpp lines 143-148 in order, after
     * _u8g2.begin() at line 137. setContrast and setFlipMode do not touch the
     * buffer; they are repeated so the display object is in the same state.
     *
     * @param state Which calls to make; the default reproduces the legacy module.
     */
    explicit HostDisplay(const DisplayState& state = DisplayState()) : U8G2() {
        u8g2_Setup_ssd1322_nhd_256x64_f(&u8g2, U8G2_R0, u8x8_byte_empty, u8x8_dummy_cb);

        begin();                 // U8G2Display.cpp:137  _u8g2.begin()
        if (!state.legacyCalls) return;
        setDrawColor(1);         // U8G2Display.cpp:143
        setFontMode(1);          // U8G2Display.cpp:144
        if (state.fontRefHeightAll) setFontRefHeightAll();   // U8G2Display.cpp:145
        if (state.fontPosTop) setFontPosTop();               // U8G2Display.cpp:146
        setContrast(0);          // U8G2Display.cpp:147
        setFlipMode(0);          // U8G2Display.cpp:148  (_flipScreen defaults to false)
    }
};

// =============================================================================
// Scenarios
// =============================================================================

/** @brief One fixed set of platform inputs for the DiagnosticsBoard. */
struct Scenario {
    const char* name;        ///< File stem and scenario id
    bool connected;          ///< WiFi.isConnected()
    bool apMode;             ///< WifiManager::getAPMode()
    const char* hostname;    ///< WifiManager::getHostname()
    const char* ssid;        ///< WifiManager::getSSID(), or nullptr when the branch never reads it
    const char* ip;          ///< WiFi.localIP().toString(), or nullptr when the branch never reads it
    unsigned long uptimeMs;  ///< Value of millis() and of the tick() argument
    size_t freeHeapBytes;    ///< heap_caps_get_free_size(MALLOC_CAP_8BIT), bytes (KB x 1024)
    const char* buildVersion;  ///< BUILD_VERSION for this scenario, or nullptr for HARNESS_DEFAULT_BUILD_VERSION
};

/**
 * @brief The scenarios; between them they cover every row-2 branch.
 *
 * The first four are synthetic. `device_dev91` replays the live values of the
 * ESP32-S3 Nano at the bench (hostname, SSID, IP, uptime, free heap and firmware
 * version as the device reported them), so a render can be compared pixel for
 * pixel with a frame captured from the device itself.
 */
static const std::vector<Scenario> kScenarios = {
    {"connected",    true,  false, "esp32-foundation", "MyNetwork", "192.168.1.42",
        (2UL * 3600 + 14 * 60 + 36) * 1000UL, 278 * 1024, nullptr},
    {"ap_mode",      false, true,  "esp32-foundation", nullptr, nullptr,
        42UL * 1000UL, 301 * 1024, nullptr},
    {"disconnected", false, false, "esp32-foundation", nullptr, nullptr,
        (3UL * 60 + 5) * 1000UL, 290 * 1024, nullptr},
    {"long_ssid",    true,  false, "esp32-foundation", "ThisIsAVeryLongNetworkNameForOverflow1",
        "10.100.200.250", (99UL * 3600 + 59 * 60 + 59) * 1000UL, 4 * 1024, nullptr},
    // Device values: "Up: 0h 1m 36s" is 96 000 ms; 8 515 584 B is the 8316 KB the panel showed.
    {"device_dev91", true,  false, "esp32-foundation", "cb-iot", "192.168.20.186",
        96000UL, 8316UL * 1024, "0.0.12-dev.91"},
};

// =============================================================================
// Render targets
// =============================================================================

/** @brief What a render target reports back about the frame it drew. */
struct RenderedRows {
    std::vector<std::string> rows;    ///< The exact text each row was given, in row order
    std::vector<std::string> fonts;   ///< The font name used per row, for the record
};

/** @brief A board the harness can render: how to draw a scenario into a display. */
struct RenderTarget {
    const char* id;                   ///< Value of --board
    const char* outputSubdir;         ///< Directory under the output root, e.g. diagnostics_board
    const char* description;          ///< One line for inputs.json
    std::function<RenderedRows(const Scenario&, U8G2&)> render;  ///< Draw one scenario
};

/**
 * @brief Load a scenario's inputs into the shimmed platform.
 * @param scenario The scenario to apply.
 */
static void applyScenario(const Scenario& scenario) {
    HostEnvironment& env = hostEnvironment();
    env.millis        = scenario.uptimeMs;
    env.wifiConnected = scenario.connected;
    env.apMode        = scenario.apMode;
    env.hostname      = scenario.hostname;
    env.ssid          = scenario.ssid ? scenario.ssid : "";
    env.localIP       = scenario.ip ? scenario.ip : "";
    env.freeHeapBytes = scenario.freeHeapBytes;
    env.buildVersion  = scenario.buildVersion ? scenario.buildVersion : HARNESS_DEFAULT_BUILD_VERSION;
}

/**
 * @brief Render the legacy DiagnosticsBoard for one scenario.
 *
 * Follows the legacy frame sequence (U8G2Display.cpp:1413): clearBuffer(), then
 * board.render(). There is no sendBuffer(); the harness reads the buffer instead.
 *
 * @param scenario Inputs to render.
 * @param display  Display to draw into (already in the legacy begin state).
 * @return The strings the board formatted for each row.
 */
static RenderedRows renderLegacyDiagnostics(const Scenario& scenario, U8G2& display) {
    // --- Step 1: platform inputs ---
    applyScenario(scenario);

    // --- Step 2: construct and configure exactly as src/main.cpp does ---
    DiagnosticsBoard board(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    WifiManager fakeWifi;
    board.configure(&fakeWifi);

    // --- Step 3: tick past the 1-second throttle (millis() and the argument agree) ---
    board.tick(static_cast<uint32_t>(scenario.uptimeMs));

    // --- Step 4: draw ---
    display.clearBuffer();
    board.render(display);

    // --- Step 5: read back what each row was actually given ---
    RenderedRows result;
    result.rows  = {board._row1._textBuffer, board._row2._textBuffer,
                    board._row3._textBuffer, board._row4._textBuffer};
    result.fonts = {"u8g2_font_helvB12_tr", "u8g2_font_helvR08_tr",
                    "u8g2_font_helvR08_tr", "u8g2_font_helvR08_tr"};
    return result;
}

#ifdef HARNESS_HAS_U8G2GL

/// Count of cross-checks that failed during the run; any non-zero fails the harness.
static int g_selfCheckFailures = 0;

/**
 * @brief Prove formatDiagnostics() is pure by driving it with fixed inputs.
 *
 * The board reads the platform through readDiagnosticsInputs() and the shims.
 * This builds a DiagnosticsInputs by hand from the scenario's literals, formats
 * it, and requires the same three rows the board published. It fails the run if
 * the formatter depends on anything but its input.
 *
 * @param scenario The scenario that was rendered.
 * @param wifi     Row 2 as the board published it.
 * @param uptime   Row 3 as the board published it.
 * @param heap     Row 4 as the board published it.
 */
static void crossCheckFormatDiagnostics(const Scenario& scenario, const std::string& wifi,
                                        const std::string& uptime, const std::string& heap) {
    DiagnosticsInputs inputs;
    inputs.wifiConnected = scenario.connected;
    inputs.apMode        = scenario.apMode;
    snprintf(inputs.hostname, sizeof(inputs.hostname), "%s", scenario.hostname);
    if (scenario.ssid) snprintf(inputs.ssid, sizeof(inputs.ssid), "%s", scenario.ssid);
    if (scenario.ip)   snprintf(inputs.ip, sizeof(inputs.ip), "%s", scenario.ip);
    inputs.uptimeMs      = scenario.uptimeMs;
    inputs.freeHeapBytes = scenario.freeHeapBytes;

    DiagnosticsLines lines;
    formatDiagnostics(inputs, lines);
    if (wifi != lines.wifi || uptime != lines.uptime || heap != lines.heap) {
        fprintf(stderr, "SELF-CHECK FAILED (%s): formatDiagnostics(fixed inputs) differs from the board's rows\n",
                scenario.name);
        ++g_selfCheckFailures;
    }
}

/**
 * @brief Render the migrated DiagnosticsScreenBoard for one scenario.
 *
 * Follows the legacy frame sequence the carousel runs: onActivate(), tick(),
 * clearBuffer(), render(); there is no sendBuffer(). The board is built and
 * configured as src/main.cpp does, after U8G2GLDisplay::begin(). The generated
 * UI is initialised once (begin() is idempotent) and every scenario overwrites
 * all four subjects, so no scenario inherits another's text.
 *
 * @param scenario Inputs to render.
 * @param display  Display to draw into (already in the legacy begin state).
 * @return The strings the screen's subjects held, row by row.
 */
static RenderedRows renderU8G2GLDiagnostics(const Scenario& scenario, U8G2& display) {
    // --- Step 1: platform inputs ---
    applyScenario(scenario);

    // --- Step 2: start the runtime once, then construct and configure the board ---
    U8G2GLDisplay::begin();
    DiagnosticsScreenBoard board;
    WifiManager fakeWifi;
    board.configure(&fakeWifi);

    // --- Step 3: the carousel's switch-in, then a tick past the 1-second throttle ---
    board.onActivate();
    board.tick(static_cast<uint32_t>(scenario.uptimeMs));

    // --- Step 4: draw ---
    display.clearBuffer();
    board.render(display);
    board.onDeactivate();

    // --- Step 5: read back the text each label was bound to ---
    RenderedRows result;
    result.rows  = {ui_subject_diag_hostname.get(), ui_subject_diag_wifi.get(),
                    ui_subject_diag_uptime.get(),   ui_subject_diag_heap.get()};
    result.fonts = {"u8g2_font_helvB12_tr", "u8g2_font_helvR08_tr",
                    "u8g2_font_helvR08_tr", "u8g2_font_helvR08_tr"};

    // --- Step 6: the formatter, driven by fixed inputs, must agree ---
    crossCheckFormatDiagnostics(scenario, result.rows[1], result.rows[2], result.rows[3]);
    return result;
}

#endif  // HARNESS_HAS_U8G2GL

/** @brief Every render target; add a line here to render another board. */
static const std::vector<RenderTarget> kTargets = {
    {"legacy", "diagnostics_board",
     "Legacy DiagnosticsBoard (modules/U8G2Display/boards/DiagnosticsBoard.cpp), compiled unmodified",
     renderLegacyDiagnostics},
#ifdef HARNESS_HAS_U8G2GL
    {"u8g2gl", "diagnostics_board",
     "Migrated board: modules/U8G2GLDisplay DiagnosticsScreenBoard running the generated U8G2GL "
     "screen (uidesign/ssd1322-256x64/u8g2gl)",
     renderU8G2GLDiagnostics},
#endif
};

// =============================================================================
// Buffer analysis and file output
// =============================================================================

/** @brief The display buffer as bytes, copied out of U8G2. */
static std::vector<uint8_t> copyBuffer(U8G2& display) {
    const size_t size = 8u * display.getBufferTileHeight() * display.getBufferTileWidth();
    const uint8_t* ptr = display.getBufferPtr();
    return std::vector<uint8_t>(ptr, ptr + size);
}

/**
 * @brief Is the pixel at (x, y) lit in a U8G2 tile-format buffer?
 *
 * U8G2 stores the buffer as horizontal bands of 8 pixel rows. Within a band,
 * byte x holds column x, and bit 0 is the top row of the band.
 */
static bool pixelLit(const std::vector<uint8_t>& buffer, int x, int y) {
    const size_t tileWidthPx = DISPLAY_WIDTH;  // One byte per column per band
    const size_t index = static_cast<size_t>(y / 8) * tileWidthPx + static_cast<size_t>(x);
    return (buffer[index] >> (y % 8)) & 1;
}

/** @brief Number of lit pixels in the whole buffer. */
static int countLit(const std::vector<uint8_t>& buffer) {
    int count = 0;
    for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
        for (int x = 0; x < DISPLAY_WIDTH; ++x) {
            count += pixelLit(buffer, x, y) ? 1 : 0;
        }
    }
    return count;
}

/** @brief Bounding box of lit pixels inside rows [y0, y1); x0 > x1 means none. */
struct BoundingBox {
    int x0 = DISPLAY_WIDTH;   ///< Leftmost lit column
    int x1 = -1;              ///< Rightmost lit column
    int litPixels = 0;        ///< Lit pixel count inside the band
};

/** @brief Measure the lit pixels inside a horizontal band of pixel rows. */
static BoundingBox measureBand(const std::vector<uint8_t>& buffer, int y0, int y1) {
    BoundingBox box;
    for (int y = y0; y < y1; ++y) {
        for (int x = 0; x < DISPLAY_WIDTH; ++x) {
            if (pixelLit(buffer, x, y)) {
                box.x0 = std::min(box.x0, x);
                box.x1 = std::max(box.x1, x);
                ++box.litPixels;
            }
        }
    }
    return box;
}

/** @brief Write the raw U8G2 buffer, exactly as getBufferPtr() holds it. */
static bool writeBin(const fs::path& path, const std::vector<uint8_t>& buffer) {
    FILE* file = fopen(path.string().c_str(), "wb");
    if (!file) return false;
    const size_t written = fwrite(buffer.data(), 1, buffer.size(), file);
    fclose(file);
    return written == buffer.size();
}

/**
 * @brief Write a binary PBM (P4): 256x64, row-major, MSB first, 1 = lit pixel.
 *
 * Converts from U8G2's tile format: within each 8-row band, byte x is column x
 * and bit 0 is the band's top row; PBM instead packs 8 columns per byte.
 */
static bool writePbm(const fs::path& path, const std::vector<uint8_t>& buffer) {
    FILE* file = fopen(path.string().c_str(), "wb");
    if (!file) return false;
    fprintf(file, "P4\n%d %d\n", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
        for (int xByte = 0; xByte < DISPLAY_WIDTH / 8; ++xByte) {
            uint8_t packed = 0;
            for (int bit = 0; bit < 8; ++bit) {
                if (pixelLit(buffer, xByte * 8 + bit, y)) {
                    packed |= static_cast<uint8_t>(0x80 >> bit);  // MSB is the leftmost pixel
                }
            }
            fputc(packed, file);
        }
    }
    fclose(file);
    return true;
}

/** @brief Print the buffer as text, '#' lit and '.' dark, rows 0-63, columns 0-255. */
static void printAscii(const std::vector<uint8_t>& buffer) {
    for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
        std::string line;
        for (int x = 0; x < DISPLAY_WIDTH; ++x) {
            line += pixelLit(buffer, x, y) ? '#' : '.';
        }
        printf("%s\n", line.c_str());
    }
}

// =============================================================================
// JSON output (hand-written: the harness has no JSON dependency)
// =============================================================================

/**
 * @brief Quote a string for JSON.
 *
 * Bytes at or above 0x80 are written as \u00XX of the raw byte value. The legacy
 * board emits a lone 0xB7 byte (not valid UTF-8), so the raw bytes cannot be
 * embedded; the escape keeps the file valid JSON and the byte recoverable.
 */
static std::string jsonString(const char* text) {
    std::string out = "\"";
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text); *p; ++p) {
        if (*p == '"' || *p == '\\') {
            out += '\\';
            out += static_cast<char>(*p);
        } else if (*p < 0x20 || *p >= 0x80) {
            char escape[8];
            snprintf(escape, sizeof(escape), "\\u%04x", *p);
            out += escape;
        } else {
            out += static_cast<char>(*p);
        }
    }
    out += '"';
    return out;
}

/** @brief jsonString for a std::string. */
static std::string jsonString(const std::string& text) { return jsonString(text.c_str()); }

/** @brief JSON for an optional C string: null when absent. */
static std::string jsonOptional(const char* text) { return text ? jsonString(text) : "null"; }

/** @brief Per-scenario facts the harness measured, for inputs.json. */
struct ScenarioResult {
    const Scenario* scenario;       ///< The inputs
    RenderedRows rows;              ///< The strings drawn
    std::vector<uint8_t> buffer;    ///< The rendered buffer
    int row2NaturalWidthPx;         ///< getUTF8Width of the row-2 string under the row-2 font
};

/** @brief Write inputs.json for a target's scenarios. */
static bool writeInputsJson(const fs::path& path, const RenderTarget& target,
                            const std::vector<ScenarioResult>& results) {
    FILE* file = fopen(path.string().c_str(), "wb");
    if (!file) return false;

    // --- Step 1: target and toolchain facts ---
    // Which board was rendered, from which sources, and how U8G2 and the U8G2GL runtime were built.
    fprintf(file, "{\n");
    fprintf(file, "  \"target\": %s,\n", jsonString(target.id).c_str());
    fprintf(file, "  \"description\": %s,\n", jsonString(target.description).c_str());
    fprintf(file, "  \"legacy_source_commit\": %s,\n", jsonString(LEGACY_SOURCE_COMMIT).c_str());
    fprintf(file, "  \"u8g2\": {\"version\": %s, \"source\": %s},\n",
            jsonString(U8G2_PINNED_VERSION).c_str(), jsonString(U8G2_SOURCE_DESCRIPTION).c_str());
#ifdef HARNESS_HAS_U8G2GL
    if (std::string(target.id) == "u8g2gl") {
        fprintf(file, "  \"u8g2gl\": {\"pinned_sha\": %s, \"source\": %s, "
                      "\"generated_ui\": \"uidesign/ssd1322-256x64/u8g2gl/generated (committed, not regenerated)\", "
                      "\"module\": \"modules/U8G2GLDisplay\", "
                      "\"runtime_capacities\": \"harness build: the generated capacities are forced into the "
                      "runtime sources, as the firmware build does via scripts/u8g2gl_generate.py\"},\n",
                jsonString(U8G2GL_PINNED_SHA).c_str(), jsonString(U8G2GL_SOURCE_DESCRIPTION).c_str());
    }
#endif
    // --- Step 2: fixed descriptions ---
    // The display state calls, the buffer and PBM formats, the string encoding and the re-run command.
    // None of these varies per scenario, so they are written once, before the scenarios.
    fprintf(file, "  \"u8g2_compile_flags\": %s,\n", jsonString(HARNESS_U8G2_FLAGS).c_str());
    fprintf(file, "  \"build_version_define\": \"harnessBuildVersion(): per scenario, default %s\",\n",
            HARNESS_DEFAULT_BUILD_VERSION);
    fprintf(file, "  \"display_setup_call\": %s,\n", jsonString(DISPLAY_SETUP_CALL).c_str());
    fprintf(file, "  \"display_state_calls\": [\"begin()\", \"setDrawColor(1)\", \"setFontMode(1)\", "
                  "\"setFontRefHeightAll()\", \"setFontPosTop()\", \"setContrast(0)\", \"setFlipMode(0)\"],\n");
    fprintf(file, "  \"display_state_source\": \"modules/U8G2Display/U8G2Display.cpp lines 137-148\",\n");
    const bool migrated = std::string(target.id) == "u8g2gl";
    fprintf(file, "  \"frame_sequence\": %s,\n",
            jsonString(migrated
                ? "U8G2GLDisplay::begin() once; board.configure(); board.onActivate(); board.tick(); "
                  "clearBuffer(); board.render(u8g2); board.onDeactivate(); no sendBuffer() "
                  "(modules/U8G2Display/U8G2Display.cpp:1413)"
                : "clearBuffer(); board.render(u8g2); no sendBuffer() "
                  "(modules/U8G2Display/U8G2Display.cpp:1413)").c_str());
    fprintf(file, "  \"buffer_format\": \"Raw getBufferPtr() bytes, 2048 = 8 * 8 tile rows * 32 tile "
                  "columns. U8G2 tile format: 8 horizontal bands of 8 pixel rows; byte index = "
                  "band * 256 + x; bit 0 is the top pixel row of the band; 1 is lit.\",\n");
    fprintf(file, "  \"pbm_format\": \"Binary PBM (P4), 256x64, row-major, MSB first, 1 = lit pixel.\",\n");
    fprintf(file, "  \"strings_encoding\": \"Non-ASCII bytes are written as \\\\u00XX escapes of the raw "
                  "byte value; the legacy separator is the single byte 0xB7 and appears as \\\\u00b7.\",\n");
    fprintf(file, "  \"rerun\": %s,\n",
            jsonString(migrated
                ? "cmake -S test/render -B tmp/render-build && cmake --build tmp/render-build --target after"
                : "cmake -S test/render -B tmp/render-build && cmake --build tmp/render-build --target baseline").c_str());
    fprintf(file, "  \"scenarios\": {\n");

    // --- Step 3: one block per scenario ---
    // Its inputs, the rows as drawn, the lit-pixel count and the row-2 geometry measured from the buffer.
    for (size_t i = 0; i < results.size(); ++i) {
        const ScenarioResult& r = results[i];
        const Scenario& s = *r.scenario;

        const BoundingBox row2Band = measureBand(r.buffer, ROW2_TOP, ROW3_TOP);

        fprintf(file, "    %s: {\n", jsonString(s.name).c_str());
        fprintf(file, "      \"inputs\": {\"connected\": %s, \"ap_mode\": %s, \"hostname\": %s, "
                      "\"ssid\": %s, \"ip\": %s, \"millis\": %lu, \"free_heap_bytes\": %zu, \"build_version\": %s},\n",
                s.connected ? "true" : "false", s.apMode ? "true" : "false",
                jsonString(s.hostname).c_str(), jsonOptional(s.ssid).c_str(),
                jsonOptional(s.ip).c_str(), s.uptimeMs, s.freeHeapBytes,
                jsonString(s.buildVersion ? s.buildVersion : HARNESS_DEFAULT_BUILD_VERSION).c_str());
        fprintf(file, "      \"rows\": [\n");
        for (size_t row = 0; row < r.rows.rows.size(); ++row) {
            fprintf(file, "        {\"row\": %zu, \"font\": %s, \"text\": %s}%s\n", row + 1,
                    jsonString(r.rows.fonts[row]).c_str(), jsonString(r.rows.rows[row]).c_str(),
                    row + 1 < r.rows.rows.size() ? "," : "");
        }
        fprintf(file, "      ],\n");
        fprintf(file, "      \"lit_pixels\": %d,\n", countLit(r.buffer));
        // The legacy label passes x + w - 1 = 251 as the far corner of its clip, which U8G2
        // treats as exclusive; the migrated label's far corner is x + w = 251 for the same
        // columns. Either way columns 4..250 are drawable, so the range recorded is the
        // legacy argument for legacy and the same corner for the migrated board.
        // band_rows is inclusive: row 2 occupies ROW2_TOP up to the row above ROW3_TOP.
        fprintf(file, "      \"row2\": {\"natural_width_px\": %d, \"clip_x_range\": [%d, %d], "
                      "\"lit_x_range\": [%d, %d], \"band_rows\": [%d, %d]}\n",
                r.row2NaturalWidthPx, ROW2_CLIP_X0, ROW2_CLIP_X1, row2Band.x0, row2Band.x1,
                ROW2_TOP, ROW3_TOP - 1);
        fprintf(file, "    }%s\n", i + 1 < results.size() ? "," : "");
    }

    fprintf(file, "  }\n}\n");
    fclose(file);
    return true;
}

// =============================================================================
// Separator probe
// =============================================================================

/**
 * @brief Measure what the legacy 0xB7 separator actually does under the row-2 font.
 *
 * Settles whether the byte draws anything, by measurement: glyph presence,
 * width and lit pixels of the byte alone, and whether the full row-2 string is
 * pixel-identical to the same string with the byte removed. Also measures how
 * far the long-SSID row-2 string reaches when the widget's clip is absent.
 *
 * @param path Where to write separator_probe.json.
 * @return true on success.
 */
static bool writeSeparatorProbe(const fs::path& path) {
    HostDisplay display;
    display.setFont(AppFonts::defaultSmall());  // u8g2_font_helvR08_tr, the row-2 font
    u8g2_t* raw = display.getU8g2();

    // --- Step 1: the byte alone, and its Unicode form ---
    const int widthByte      = display.getUTF8Width("\xB7");
    const int widthStrByte   = display.getStrWidth("\xB7");
    const int widthUtf8Form  = display.getUTF8Width("\xC2\xB7");   // U+00B7 as UTF-8
    const int widthSpace     = display.getUTF8Width(" ");
    const bool glyphB7Present = u8g2_IsGlyph(raw, 0xB7) != 0;

    auto litAfterDraw = [&](const char* text) {
        display.clearBuffer();
        display.drawUTF8(10, 10, text);
        return countLit(copyBuffer(display));
    };
    const int litByteAlone = litAfterDraw("\xB7");
    const int litUtf8Form  = litAfterDraw("\xC2\xB7");

    // --- Step 2: the connected row-2 string with and without the byte ---
    // The legacy format is "%s \xB7 %s". If the byte draws nothing, it is
    // pixel-identical to "%s  %s" (two spaces, the byte removed).
    auto drawRow = [&](const char* text) {
        display.clearBuffer();
        display.drawUTF8(ROW2_CLIP_X0, ROW2_TOP, text);
        return copyBuffer(display);
    };
    const std::vector<uint8_t> withByte  = drawRow("MyNetwork \xB7 192.168.1.42");
    const std::vector<uint8_t> twoSpaces = drawRow("MyNetwork  192.168.1.42");
    const std::vector<uint8_t> oneSpace  = drawRow("MyNetwork 192.168.1.42");

    // --- Step 3: the long-SSID row without the widget's clip window ---
    // The legacy row-2 widget clips to x = ROW2_CLIP_X0..ROW2_CLIP_X1 (4..251). Drawing the same string with
    // no clip shows how much the clip suppresses.
    const std::string longRow = std::string("ThisIsAVeryLongNetworkNameForOverflow1") + " \xB7 10.100.200.250";
    const std::vector<uint8_t> unclipped = drawRow(longRow.c_str());
    int unclippedMaxX = -1;
    int unclippedLitBeyond251 = 0;
    for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
        for (int x = 0; x < DISPLAY_WIDTH; ++x) {
            if (!pixelLit(unclipped, x, y)) continue;
            unclippedMaxX = std::max(unclippedMaxX, x);
            if (x > ROW2_CLIP_X1) ++unclippedLitBeyond251;
        }
    }

    FILE* file = fopen(path.string().c_str(), "wb");
    if (!file) return false;
    fprintf(file, "{\n");
    fprintf(file, "  \"font\": \"u8g2_font_helvR08_tr\",\n");
    fprintf(file, "  \"flags\": %s,\n", jsonString(HARNESS_U8G2_FLAGS).c_str());
    fprintf(file, "  \"glyph_0xB7_in_font\": %s,\n", glyphB7Present ? "true" : "false");
    fprintf(file, "  \"getUTF8Width_byte_0xB7_px\": %d,\n", widthByte);
    fprintf(file, "  \"getStrWidth_byte_0xB7_px\": %d,\n", widthStrByte);
    fprintf(file, "  \"getUTF8Width_utf8_C2B7_px\": %d,\n", widthUtf8Form);
    fprintf(file, "  \"getUTF8Width_space_px\": %d,\n", widthSpace);
    fprintf(file, "  \"drawUTF8_byte_0xB7_alone_lit_pixels\": %d,\n", litByteAlone);
    fprintf(file, "  \"drawUTF8_utf8_C2B7_alone_lit_pixels\": %d,\n", litUtf8Form);
    fprintf(file, "  \"row2_with_byte_equals_two_spaces\": %s,\n", withByte == twoSpaces ? "true" : "false");
    fprintf(file, "  \"row2_with_byte_equals_one_space\": %s,\n", withByte == oneSpace ? "true" : "false");
    fprintf(file, "  \"long_ssid_row2_unclipped_max_lit_x\": %d,\n", unclippedMaxX);
    fprintf(file, "  \"long_ssid_row2_unclipped_lit_pixels_beyond_x251\": %d\n", unclippedLitBeyond251);
    fprintf(file, "}\n");
    fclose(file);
    return true;
}

// =============================================================================
// Comparison with another output root
// =============================================================================

/** @brief How two display buffers differ. */
struct BufferDiff {
    int differingPixels = 0;  ///< Pixels lit in exactly one of the two buffers
    int onlyInA = 0;          ///< Lit in A (the reference) and dark in B
    int onlyInB = 0;          ///< Dark in A and lit in B
    int rowMin = DISPLAY_HEIGHT;  ///< First row holding a difference
    int rowMax = -1;              ///< Last row holding a difference
    int colMin = DISPLAY_WIDTH;   ///< First column holding a difference
    int colMax = -1;              ///< Last column holding a difference
};

/** @brief Compare two buffers pixel by pixel. A is the reference. */
static BufferDiff diffBuffers(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    BufferDiff diff;
    for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
        for (int x = 0; x < DISPLAY_WIDTH; ++x) {
            const bool litA = pixelLit(a, x, y);
            const bool litB = pixelLit(b, x, y);
            if (litA == litB) continue;
            ++diff.differingPixels;
            if (litA) ++diff.onlyInA; else ++diff.onlyInB;
            diff.rowMin = std::min(diff.rowMin, y);
            diff.rowMax = std::max(diff.rowMax, y);
            diff.colMin = std::min(diff.colMin, x);
            diff.colMax = std::max(diff.colMax, x);
        }
    }
    return diff;
}

/** @brief Read a raw buffer file; empty if it is missing or the wrong size. */
static std::vector<uint8_t> readBin(const fs::path& path) {
    std::vector<uint8_t> buffer(2048);
    FILE* file = fopen(path.string().c_str(), "rb");
    if (!file) return {};
    const size_t got = fread(buffer.data(), 1, buffer.size(), file);
    const bool extra = fgetc(file) != EOF;
    fclose(file);
    return (got == buffer.size() && !extra) ? buffer : std::vector<uint8_t>{};
}

/**
 * @brief Print the pixels where two buffers differ, for the worst case.
 *
 * '#' lit in both, '+' lit only in the rendered buffer, '-' lit only in the
 * reference, '.' dark in both. Only rows that contain something are printed,
 * with their row number.
 */
static void printAsciiDiff(const std::vector<uint8_t>& reference, const std::vector<uint8_t>& rendered) {
    printf("  ASCII diff ('#' both, '+' rendered only, '-' reference only):\n");
    for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
        std::string line;
        bool any = false;
        for (int x = 0; x < DISPLAY_WIDTH; ++x) {
            const bool litA = pixelLit(reference, x, y);
            const bool litB = pixelLit(rendered, x, y);
            line += litA && litB ? '#' : (litB ? '+' : (litA ? '-' : '.'));
            any = any || litA || litB;
        }
        if (any) printf("  %2d %s\n", y, line.c_str());
    }
}

/**
 * @brief Compare every scenario's rendered buffer with a reference output root.
 *
 * Reads <referenceRoot>/<subdir>/<scenario>.bin, as written by an earlier run.
 *
 * @param referenceRoot Root holding the reference output (e.g. baseline/).
 * @param target        The target whose subdirectory is compared.
 * @param results       What was just rendered.
 * @return true only if every scenario is byte-identical to its reference.
 */
static bool compareWithReference(const fs::path& referenceRoot, const RenderTarget& target,
                                 const std::vector<ScenarioResult>& results) {
    bool allIdentical = true;
    printf("compare with %s\n", (referenceRoot / target.outputSubdir).string().c_str());
    for (const ScenarioResult& r : results) {
        const std::vector<uint8_t> reference =
            readBin(referenceRoot / target.outputSubdir / (std::string(r.scenario->name) + ".bin"));
        if (reference.empty()) {
            printf("  %-13s reference missing or not 2048 bytes\n", r.scenario->name);
            allIdentical = false;
            continue;
        }
        const BufferDiff diff = diffBuffers(reference, r.buffer);
        const bool identical = reference == r.buffer;
        printf("  %-13s identical=%s differing_pixels=%d", r.scenario->name,
               identical ? "yes" : "NO", diff.differingPixels);
        if (diff.differingPixels > 0) {
            printf(" (rendered-only %d, reference-only %d) rows %d..%d columns %d..%d",
                   diff.onlyInB, diff.onlyInA, diff.rowMin, diff.rowMax, diff.colMin, diff.colMax);
        }
        printf("\n");
        if (!identical) {
            allIdentical = false;
            if (diff.differingPixels > 0) printAsciiDiff(reference, r.buffer);
        }
    }
    return allIdentical;
}

// =============================================================================
// Clip probe: what the label clip removes
// =============================================================================

/**
 * @brief Measure the pixels the labels' clip removes, row by row.
 *
 * Draws each scenario's four strings with no clip at all, where the legacy board
 * draws them (the same x, y and font, centred in 256 for row 1), and reports
 * which lit pixels of that unclipped frame are dark in the rendered frame, by pixel
 * row. This is the legacy descender defect, measured: legacy drawText() passes
 * x + w - 1 and y + h - 1 as the far corner of setClipWindow(), U8G2 treats that
 * corner as exclusive (u8g2.h:352/354), so the last column and row of each label
 * are lost; with h = getMaxCharHeight() that is the descenders' last row.
 *
 * @param path    Where to write clip_probe.json.
 * @param results The rendered scenarios of the chosen target.
 * @return true on success.
 */
static bool writeClipProbe(const fs::path& path, const std::vector<ScenarioResult>& results) {
    /** @brief Legacy placement of one row. */
    struct RowPlacement {
        int x;          ///< Left edge of the label
        int y;          ///< Top pixel row of the label
        bool centred;   ///< Centred in the display width (row 1 only)
        bool large;     ///< The large (bold) font, else the small one
    };
    const RowPlacement placements[4] = {
        {0, 0,  true,  true},  {ROW2_CLIP_X0, ROW2_TOP, false, false},
        {4, ROW3_TOP, false, false}, {4, 44, false, false},
    };

    FILE* file = fopen(path.string().c_str(), "wb");
    if (!file) return false;
    fprintf(file, "{\n  \"method\": \"each row drawn unclipped at its legacy position; a pixel lit there and dark in the "
                  "rendered frame was removed by the clip. The _inclusive_ figures repeat the measurement through a "
                  "window whose far corner includes the label's last column and row (what the legacy code evidently "
                  "intended); whatever that still removes lies outside the label's own height.\",\n  \"scenarios\": {\n");
    for (size_t i = 0; i < results.size(); ++i) {
        const ScenarioResult& r = results[i];

        // --- Step 1: the unclipped frame ---
        HostDisplay display;
        display.clearBuffer();
        for (int row = 0; row < 4; ++row) {
            const RowPlacement& place = placements[row];
            display.setFont(place.large ? AppFonts::defaultLarge() : AppFonts::defaultSmall());
            int x = place.x;
            if (place.centred) x += (DISPLAY_WIDTH - display.getUTF8Width(r.rows.rows[row].c_str())) / 2;
            display.drawUTF8(x, place.y, r.rows.rows[row].c_str());
        }
        const std::vector<uint8_t> unclipped = copyBuffer(display);

        // --- Step 1b: the clip the legacy code evidently intended ---
        // The same rows through a window whose far corner is INCLUSIVE of the label's last
        // column and row, i.e. setClipWindow(x, y, x + w, y + h) with h = getMaxCharHeight().
        // What this still removes is not the exclusive-corner defect: it is glyph pixels
        // outside the label's own height.
        HostDisplay intended;
        intended.clearBuffer();
        for (int row = 0; row < 4; ++row) {
            const RowPlacement& place = placements[row];
            intended.setFont(place.large ? AppFonts::defaultLarge() : AppFonts::defaultSmall());
            const int width = place.centred ? DISPLAY_WIDTH : DISPLAY_WIDTH - 8;  // Legacy label widths
            int x = place.x;
            if (place.centred) x += (DISPLAY_WIDTH - intended.getUTF8Width(r.rows.rows[row].c_str())) / 2;
            intended.setClipWindow(place.x, place.y, place.x + width, place.y + intended.getMaxCharHeight());
            intended.drawUTF8(x, place.y, r.rows.rows[row].c_str());
            intended.setMaxClipWindow();
        }
        const std::vector<uint8_t> intendedFrame = copyBuffer(intended);

        // --- Step 2: what the clip removed, and anything it added (nothing should be) ---
        const BufferDiff diff = diffBuffers(unclipped, r.buffer);
        auto lostByRow = [&](const std::vector<uint8_t>& kept) {
            std::string out;
            for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
                int lostInRow = 0;
                for (int x = 0; x < DISPLAY_WIDTH; ++x) {
                    if (pixelLit(unclipped, x, y) && !pixelLit(kept, x, y)) ++lostInRow;
                }
                if (lostInRow > 0) {
                    out += (out.empty() ? "" : ", ") + std::string("\"") + std::to_string(y) + "\": " +
                           std::to_string(lostInRow);
                }
            }
            return out;
        };
        const BufferDiff intendedDiff = diffBuffers(unclipped, intendedFrame);
        fprintf(file, "    %s: {\"unclipped_lit\": %d, \"rendered_lit\": %d, \"removed_by_clip\": %d, "
                      "\"added_by_render\": %d, \"removed_by_pixel_row\": {%s},\n"
                      "      \"removed_if_far_corner_were_inclusive\": %d, "
                      "\"removed_if_far_corner_were_inclusive_by_pixel_row\": {%s}}%s\n",
                jsonString(r.scenario->name).c_str(), countLit(unclipped), countLit(r.buffer),
                diff.onlyInA, diff.onlyInB, lostByRow(r.buffer).c_str(),
                intendedDiff.onlyInA, lostByRow(intendedFrame).c_str(), i + 1 < results.size() ? "," : "");
    }
    fprintf(file, "  }\n}\n");
    fclose(file);
    return true;
}

#ifdef HARNESS_HAS_U8G2GL

// =============================================================================
// State probe: what the screen inherits from the legacy display state
// =============================================================================

/** @brief One probe variant: how the rendered `connected` frame differs from the normal one. */
struct StateProbe {
    const char* name;         ///< Variant id
    const char* description;  ///< What differs from the normal render
    std::vector<uint8_t> buffer;  ///< The frame rendered under this variant
    BufferDiff diff;          ///< Versus the normal migrated `connected` frame
};

/** @brief The `connected` scenario, looked up by name. */
static const Scenario& connectedScenario() {
    for (const Scenario& scenario : kScenarios) {
        if (std::string(scenario.name) == "connected") return scenario;
    }
    return kScenarios.front();
}

/**
 * @brief Render `connected` through ui_handler(), as a plain U8G2GL user would.
 *
 * ui_handler() clears, ticks, renders and sends the buffer itself, and a
 * screen must be active. No board is involved, and no legacy state has been set.
 */
static std::vector<uint8_t> renderConnectedViaUiHandler(U8G2& display) {
    const Scenario& scenario = connectedScenario();
    applyScenario(scenario);
    U8G2GLDisplay::begin();
    DiagnosticsScreenBoard board;           // Feeds the subjects, as in the firmware
    WifiManager fakeWifi;
    board.configure(&fakeWifi);
    board.tick(static_cast<uint32_t>(scenario.uptimeMs));
    // A font must be current before the first frame: the runtime's state saver puts back
    // whatever font was current, and with none set that is nullptr, which U8G2 then
    // dereferences (u8g2_font.c:114, U8G2 2.36.18). Measured: the first run of this probe without
    // this line crashed there. Every label sets its own font, so this changes no pixel.
    display.setFont(AppFonts::defaultSmall());
    u8g2gl_set_active_screen(&ui_screen_diagnostics);
    ui_handler(display);
    u8g2gl_set_active_screen(nullptr);
    return copyBuffer(display);
}

/**
 * @brief Measure what the screen needs from the U8G2 state the legacy module sets.
 *
 * Renders the `connected` scenario under display states that lack one legacy
 * call each, and reports how each frame differs from the normal one. Also
 * reads the font mode after the runtime's render_fn and after the board's
 * render(), to show that the board restores what the runtime leaves changed.
 *
 * The suspected gap: the spec calls font-position-top "the framework default",
 * but nothing in the runtime's src/ calls setFontPosTop().
 *
 * @param path   Where to write state_probe.json.
 * @param normal The normal migrated `connected` frame.
 * @return true on success.
 */
static bool writeStateProbe(const fs::path& path, const std::vector<uint8_t>& normal) {
    const Scenario& scenario = connectedScenario();
    const RenderTarget* target = nullptr;
    for (const RenderTarget& t : kTargets) {
        if (std::string(t.id) == "u8g2gl") target = &t;
    }
    if (!target) return false;

    std::vector<StateProbe> probes;
    auto addProbe = [&](const char* name, const char* description, std::vector<uint8_t> buffer) {
        StateProbe probe{name, description, std::move(buffer), {}};
        probe.diff = diffBuffers(normal, probe.buffer);
        probes.push_back(std::move(probe));
    };

    // --- Variant 1: the legacy state without setFontPosTop() ---
    {
        DisplayState state;
        state.fontPosTop = false;
        HostDisplay display(state);
        target->render(scenario, display);
        addProbe("no_font_pos_top",
                 "legacy begin state minus setFontPosTop(); rendered by the board", copyBuffer(display));
    }
    // --- Variant 2: the legacy state without setFontRefHeightAll() ---
    {
        DisplayState state;
        state.fontRefHeightAll = false;
        HostDisplay display(state);
        target->render(scenario, display);
        addProbe("no_font_ref_height_all",
                 "legacy begin state minus setFontRefHeightAll(); rendered by the board", copyBuffer(display));
    }
    // --- Variant 3: a plain U8G2GL user: begin() only, drawn through ui_handler() ---
    {
        DisplayState state;
        state.legacyCalls = false;
        HostDisplay display(state);
        addProbe("plain_user_ui_handler",
                 "begin() only (no drawColor, fontMode, refHeight, posTop); drawn by ui_handler()",
                 renderConnectedViaUiHandler(display));
    }

    // --- Font mode after the runtime's render versus the board's render ---
    int modeAfterRuntime = -1;
    int modeAfterBoard = -1;
    {
        HostDisplay display;   // legacy state: font mode 1 (transparent)
        applyScenario(scenario);
        U8G2GLDisplay::begin();
        DiagnosticsScreenBoard board;
        WifiManager fakeWifi;
        board.configure(&fakeWifi);
        board.tick(static_cast<uint32_t>(scenario.uptimeMs));
        // The runtime alone needs a current font (see renderConnectedViaUiHandler); the board
        // supplies one itself, so only this bypass has to.
        display.setFont(AppFonts::defaultSmall());
        display.clearBuffer();
        ui_screen_diagnostics.render_fn(display);  // the runtime alone, bypassing the board
        modeAfterRuntime = display.getU8g2()->font_decode.is_transparent;

        display.setFontMode(1);
        display.clearBuffer();
        board.render(display);                     // through the board host
        modeAfterBoard = display.getU8g2()->font_decode.is_transparent;
    }

    // The frames themselves, so "what it changes" can be looked at, not only counted.
    for (const StateProbe& p : probes) {
        if (!writePbm(path.parent_path() / (std::string("probe_") + p.name + ".pbm"), p.buffer)) return false;
    }

    FILE* file = fopen(path.string().c_str(), "wb");
    if (!file) return false;
    fprintf(file, "{\n");
    fprintf(file, "  \"scenario\": \"connected\",\n");
    fprintf(file, "  \"reference\": \"the normal migrated connected frame (legacy display state)\",\n");
    fprintf(file, "  \"font_mode_before\": 1,\n");
    fprintf(file, "  \"font_mode_after_runtime_render_fn\": %d,\n", modeAfterRuntime);
    fprintf(file, "  \"font_mode_after_board_render\": %d,\n", modeAfterBoard);
    fprintf(file, "  \"font_mode_note\": \"1 = transparent (legacy), 0 = solid. The runtime's label sets the mode "
                  "per draw and its state saver does not restore it.\",\n");
    fprintf(file, "  \"variants\": [\n");
    for (size_t i = 0; i < probes.size(); ++i) {
        const StateProbe& p = probes[i];
        fprintf(file, "    {\"name\": %s, \"description\": %s, \"lit_pixels\": %d, "
                      "\"identical_to_normal\": %s, \"differing_pixels\": %d, "
                      "\"rendered_only\": %d, \"normal_only\": %d, "
                      "\"rows\": [%d, %d], \"columns\": [%d, %d]}%s\n",
                jsonString(p.name).c_str(), jsonString(p.description).c_str(), countLit(p.buffer),
                p.diff.differingPixels == 0 ? "true" : "false", p.diff.differingPixels,
                p.diff.onlyInB, p.diff.onlyInA,
                p.diff.differingPixels ? p.diff.rowMin : -1, p.diff.differingPixels ? p.diff.rowMax : -1,
                p.diff.differingPixels ? p.diff.colMin : -1, p.diff.differingPixels ? p.diff.colMax : -1,
                i + 1 < probes.size() ? "," : "");
    }
    fprintf(file, "  ]\n}\n");
    fclose(file);

    // Echo to stdout for the run log.
    printf("state probe (connected, versus the normal migrated frame):\n");
    for (const StateProbe& p : probes) {
        printf("  %-24s differing_pixels=%d (rendered-only %d, normal-only %d)", p.name,
               p.diff.differingPixels, p.diff.onlyInB, p.diff.onlyInA);
        if (p.diff.differingPixels) {
            printf(" rows %d..%d columns %d..%d", p.diff.rowMin, p.diff.rowMax, p.diff.colMin, p.diff.colMax);
        }
        printf("\n");
    }
    printf("  font mode after runtime render_fn=%d, after board render=%d (before=1)\n",
           modeAfterRuntime, modeAfterBoard);
    return true;
}

#endif  // HARNESS_HAS_U8G2GL

// =============================================================================
// Entry point
// =============================================================================

/** @brief Print usage and return a failure code. */
static int usage() {
    fprintf(stderr, "usage: render_harness --board <id> [--out <dir>] [--ascii <scenario>] [--probe-separator]\n"
                    "                       [--compare <reference-root>] [--probe-state] [--probe-clip]\n");
    fprintf(stderr, "boards:");
    for (const RenderTarget& t : kTargets) fprintf(stderr, " %s", t.id);
    fprintf(stderr, "\n");
    return 2;
}

int main(int argc, char** argv) {
    // --- Step 1: arguments ---
    std::string boardId;
    fs::path outRoot;
    std::string asciiScenario;
    bool probeSeparator = false;
    fs::path compareRoot;
    bool probeState = false;
    bool probeClip = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--board" && i + 1 < argc)       boardId = argv[++i];
        else if (arg == "--out" && i + 1 < argc)    outRoot = argv[++i];
        else if (arg == "--ascii" && i + 1 < argc)  asciiScenario = argv[++i];
        else if (arg == "--probe-separator")        probeSeparator = true;
        else if (arg == "--compare" && i + 1 < argc) compareRoot = argv[++i];
        else if (arg == "--probe-state")            probeState = true;
        else if (arg == "--probe-clip")             probeClip = true;
        else return usage();
    }
    if (boardId.empty()) return usage();

    const RenderTarget* target = nullptr;
    for (const RenderTarget& t : kTargets) {
        if (boardId == t.id) target = &t;
    }
    if (!target) return usage();

    // --- Step 2: render every scenario into its own fresh display ---
    std::vector<ScenarioResult> results;
    for (const Scenario& scenario : kScenarios) {
        HostDisplay display;
        ScenarioResult result;
        result.scenario = &scenario;
        result.rows = target->render(scenario, display);
        result.buffer = copyBuffer(display);

        // Natural width of row 2 under its own font, to tell clipping from fit.
        display.setFont(AppFonts::defaultSmall());
        result.row2NaturalWidthPx = display.getUTF8Width(result.rows.rows[1].c_str());

        results.push_back(std::move(result));
    }

    // --- Step 3: ASCII view, when asked ---
    if (!asciiScenario.empty()) {
        for (const ScenarioResult& r : results) {
            if (asciiScenario == r.scenario->name) printAscii(r.buffer);
        }
    }

    // --- Step 3b: comparison with a reference output root, when asked ---
    bool identicalToReference = true;
    if (!compareRoot.empty()) {
        identicalToReference = compareWithReference(compareRoot, *target, results);
    }

    if (outRoot.empty()) {
#ifdef HARNESS_HAS_U8G2GL
        if (g_selfCheckFailures != 0) return 4;  // The cross-check runs while rendering, so it counts here too
#endif
        return identicalToReference ? 0 : 3;
    }

    // --- Step 4: files ---
    const fs::path outDir = outRoot / target->outputSubdir;
    fs::create_directories(outDir);
    for (const ScenarioResult& r : results) {
        const std::string stem = r.scenario->name;
        if (!writeBin(outDir / (stem + ".bin"), r.buffer) ||
            !writePbm(outDir / (stem + ".pbm"), r.buffer)) {
            fprintf(stderr, "failed to write %s\n", stem.c_str());
            return 1;
        }
        printf("%-13s lit=%d\n", r.scenario->name, countLit(r.buffer));
    }
    if (!writeInputsJson(outDir / "inputs.json", *target, results)) return 1;
    if (probeSeparator && !writeSeparatorProbe(outDir / "separator_probe.json")) return 1;
    if (probeClip && !writeClipProbe(outDir / "clip_probe.json", results)) return 1;
#ifdef HARNESS_HAS_U8G2GL
    if (probeState) {
        // The normal frame the probe variants are measured against: the target's `connected` result.
        std::vector<uint8_t> normal;
        for (const ScenarioResult& r : results) {
            if (std::string(r.scenario->name) == "connected") normal = r.buffer;
        }
        if (std::string(target->id) != "u8g2gl" || !writeStateProbe(outDir / "state_probe.json", normal)) {
            fprintf(stderr, "--probe-state needs --board u8g2gl\n");
            return 1;
        }
    }
    if (g_selfCheckFailures != 0) return 4;
#else
    if (probeState) {
        fprintf(stderr, "--probe-state needs a build with RENDER_U8G2GL=ON\n");
        return 1;
    }
#endif
    return identicalToReference ? 0 : 3;
}
