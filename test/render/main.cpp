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
 *              and are selected with --board.
 *
 *              The platform the board reads (WiFi, millis, heap, hostname) is the
 *              set of shims in test/render/shim, driven through HostEnvironment.
 *
 * Exported Functions:
 * - main(): Parse --board / --out / --ascii / --probe-separator, render every
 *           scenario of the chosen target and write its baseline files.
 *
 * Usage:
 *   render_harness --board legacy --out <dir>
 *   render_harness --board legacy --ascii connected
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
// alone widens access while it includes the board headers. Access specifiers do
// not change object layout, so DiagnosticsBoard.cpp (compiled normally) and this
// file agree on the layout.
#define private public
#include "boards/DiagnosticsBoard.hpp"
#undef private

#include "fonts/Fonts.hpp"

namespace fs = std::filesystem;

// =============================================================================
// Constants — what is being rendered, and from what
// =============================================================================

static constexpr int DISPLAY_WIDTH  = 256;   // Panel width in pixels (SSD1322 NHD 256x64)
static constexpr int DISPLAY_HEIGHT = 64;    // Panel height in pixels
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
#ifndef HARNESS_U8G2_FLAGS
#define HARNESS_U8G2_FLAGS "unknown"
#endif

// =============================================================================
// Display — the firmware's U8G2 object, minus the SPI pins
// =============================================================================

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
     * _u8g2.begin() at line 139. setContrast and setFlipMode do not touch the
     * buffer; they are repeated so the display object is in the same state.
     */
    HostDisplay() : U8G2() {
        u8g2_Setup_ssd1322_nhd_256x64_f(&u8g2, U8G2_R0, u8x8_byte_empty, u8x8_dummy_cb);

        begin();                 // U8G2Display.cpp:139  _u8g2.begin()
        setDrawColor(1);         // U8G2Display.cpp:143
        setFontMode(1);          // U8G2Display.cpp:144
        setFontRefHeightAll();   // U8G2Display.cpp:145
        setFontPosTop();         // U8G2Display.cpp:146
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
};

/** @brief The four scenarios; between them they cover every row-2 branch. */
static const std::vector<Scenario> kScenarios = {
    {"connected",    true,  false, "esp32-foundation", "MyNetwork", "192.168.1.42",
        (2UL * 3600 + 14 * 60 + 36) * 1000UL, 278 * 1024},
    {"ap_mode",      false, true,  "esp32-foundation", nullptr, nullptr,
        42UL * 1000UL, 301 * 1024},
    {"disconnected", false, false, "esp32-foundation", nullptr, nullptr,
        (3UL * 60 + 5) * 1000UL, 290 * 1024},
    {"long_ssid",    true,  false, "esp32-foundation", "ThisIsAVeryLongNetworkNameForOverflow1",
        "10.100.200.250", (99UL * 3600 + 59 * 60 + 59) * 1000UL, 4 * 1024},
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

/** @brief Every render target; add a line here to render another board. */
static const std::vector<RenderTarget> kTargets = {
    {"legacy", "diagnostics_board",
     "Legacy DiagnosticsBoard (modules/U8G2Display/boards/DiagnosticsBoard.cpp), compiled unmodified",
     renderLegacyDiagnostics},
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

    fprintf(file, "{\n");
    fprintf(file, "  \"target\": %s,\n", jsonString(target.id).c_str());
    fprintf(file, "  \"description\": %s,\n", jsonString(target.description).c_str());
    fprintf(file, "  \"legacy_source_commit\": %s,\n", jsonString(LEGACY_SOURCE_COMMIT).c_str());
    fprintf(file, "  \"u8g2\": {\"version\": %s, \"source\": %s},\n",
            jsonString(U8G2_PINNED_VERSION).c_str(), jsonString(U8G2_SOURCE_DESCRIPTION).c_str());
    fprintf(file, "  \"u8g2_compile_flags\": %s,\n", jsonString(HARNESS_U8G2_FLAGS).c_str());
    fprintf(file, "  \"build_version_define\": %s,\n", jsonString(BUILD_VERSION).c_str());
    fprintf(file, "  \"display_setup_call\": %s,\n", jsonString(DISPLAY_SETUP_CALL).c_str());
    fprintf(file, "  \"display_state_calls\": [\"begin()\", \"setDrawColor(1)\", \"setFontMode(1)\", "
                  "\"setFontRefHeightAll()\", \"setFontPosTop()\", \"setContrast(0)\", \"setFlipMode(0)\"],\n");
    fprintf(file, "  \"display_state_source\": \"modules/U8G2Display/U8G2Display.cpp lines 139-148\",\n");
    fprintf(file, "  \"frame_sequence\": \"clearBuffer(); board.render(u8g2); no sendBuffer() "
                  "(modules/U8G2Display/U8G2Display.cpp:1413)\",\n");
    fprintf(file, "  \"buffer_format\": \"Raw getBufferPtr() bytes, 2048 = 8 * 8 tile rows * 32 tile "
                  "columns. U8G2 tile format: 8 horizontal bands of 8 pixel rows; byte index = "
                  "band * 256 + x; bit 0 is the top pixel row of the band; 1 is lit.\",\n");
    fprintf(file, "  \"pbm_format\": \"Binary PBM (P4), 256x64, row-major, MSB first, 1 = lit pixel.\",\n");
    fprintf(file, "  \"strings_encoding\": \"Non-ASCII bytes are written as \\\\u00XX escapes of the raw "
                  "byte value; the legacy separator is the single byte 0xB7 and appears as \\\\u00b7.\",\n");
    fprintf(file, "  \"rerun\": \"cmake -S test/render -B tmp/render-build && "
                  "cmake --build tmp/render-build --target baseline\",\n");
    fprintf(file, "  \"scenarios\": {\n");

    for (size_t i = 0; i < results.size(); ++i) {
        const ScenarioResult& r = results[i];
        const Scenario& s = *r.scenario;

        const BoundingBox row2Band = measureBand(r.buffer, 16, 30);

        fprintf(file, "    %s: {\n", jsonString(s.name).c_str());
        fprintf(file, "      \"inputs\": {\"connected\": %s, \"ap_mode\": %s, \"hostname\": %s, "
                      "\"ssid\": %s, \"ip\": %s, \"millis\": %lu, \"free_heap_bytes\": %zu},\n",
                s.connected ? "true" : "false", s.apMode ? "true" : "false",
                jsonString(s.hostname).c_str(), jsonOptional(s.ssid).c_str(),
                jsonOptional(s.ip).c_str(), s.uptimeMs, s.freeHeapBytes);
        fprintf(file, "      \"rows\": [\n");
        for (size_t row = 0; row < r.rows.rows.size(); ++row) {
            fprintf(file, "        {\"row\": %zu, \"font\": %s, \"text\": %s}%s\n", row + 1,
                    jsonString(r.rows.fonts[row]).c_str(), jsonString(r.rows.rows[row]).c_str(),
                    row + 1 < r.rows.rows.size() ? "," : "");
        }
        fprintf(file, "      ],\n");
        fprintf(file, "      \"lit_pixels\": %d,\n", countLit(r.buffer));
        fprintf(file, "      \"row2\": {\"natural_width_px\": %d, \"clip_x_range\": [4, 251], "
                      "\"lit_x_range\": [%d, %d], \"band_rows\": [16, 29]}\n",
                r.row2NaturalWidthPx, row2Band.x0, row2Band.x1);
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
        display.drawUTF8(4, 16, text);
        return copyBuffer(display);
    };
    const std::vector<uint8_t> withByte  = drawRow("MyNetwork \xB7 192.168.1.42");
    const std::vector<uint8_t> twoSpaces = drawRow("MyNetwork  192.168.1.42");
    const std::vector<uint8_t> oneSpace  = drawRow("MyNetwork 192.168.1.42");

    // --- Step 3: the long-SSID row without the widget's clip window ---
    // The legacy row-2 widget clips to x = 4..251. Drawing the same string with
    // no clip shows how much the clip suppresses.
    const std::string longRow = std::string("ThisIsAVeryLongNetworkNameForOverflow1") + " \xB7 10.100.200.250";
    const std::vector<uint8_t> unclipped = drawRow(longRow.c_str());
    int unclippedMaxX = -1;
    int unclippedLitBeyond251 = 0;
    for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
        for (int x = 0; x < DISPLAY_WIDTH; ++x) {
            if (!pixelLit(unclipped, x, y)) continue;
            unclippedMaxX = std::max(unclippedMaxX, x);
            if (x > 251) ++unclippedLitBeyond251;
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
// Entry point
// =============================================================================

/** @brief Print usage and return a failure code. */
static int usage() {
    fprintf(stderr, "usage: render_harness --board <id> --out <dir> [--ascii <scenario>] [--probe-separator]\n");
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
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--board" && i + 1 < argc)       boardId = argv[++i];
        else if (arg == "--out" && i + 1 < argc)    outRoot = argv[++i];
        else if (arg == "--ascii" && i + 1 < argc)  asciiScenario = argv[++i];
        else if (arg == "--probe-separator")        probeSeparator = true;
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

    if (outRoot.empty()) return 0;

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
    return 0;
}
