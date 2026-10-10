/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/render/legacy_boards.cpp
 * Description: Host pixel renderer for the legacy boards that reach drawText()
 *              with the default height (h = -1): BootBoard, ConfiguringStatusBoard,
 *              OTAProgressBoard, ErrorBoard, GenericGateHelperBoard and
 *              WiFiWizardBoard. It is a second executable beside render_harness
 *              because render_harness is built around the DiagnosticsBoard's five
 *              scenarios and its byte-for-byte identity gate; this one renders
 *              each board in a few fixed states and writes the frames, so a
 *              change to drawText() can be shown as a before/after pixel diff.
 *
 *              The boards, widgets and DrawingPrimitives.cpp are compiled
 *              unmodified from modules/U8G2Display, against the same real U8G2,
 *              flags, shims and legacy display state as render_harness.
 *
 * Exported Functions/Classes:
 * - HostDisplay: The firmware's SSD1322 256x64 U8G2 object with null I/O, in the
 *                legacy begin state (U8G2Display.cpp:137-148).
 * - kFrames: The board states that are rendered.
 * - main(): Parse --out / --ascii / --diff, render every frame, write or compare.
 *
 * Usage:
 *   render_legacy_boards --out <dir>             write <frame>.bin and <frame>.pbm for every frame
 *   render_legacy_boards --ascii <frame>         print one frame to stdout ('#' lit, '.' dark)
 *   render_legacy_boards --diff <dirA> <dirB>    ASCII diff of every frame that differs
 *                                                ('#' both, '-' only in A, '+' only in B)
 */

#include <U8g2lib.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "HostEnvironment.hpp"

#include "PreconditionEngine/PreconditionEngine.hpp"
#include "boards/BootBoard.hpp"
#include "boards/ConfiguringStatusBoard.hpp"
#include "boards/ErrorBoard.hpp"
#include "boards/GenericGateHelperBoard.hpp"
#include "boards/OTAProgressBoard.hpp"
#include "boards/WiFiWizardBoard.hpp"
#include "fonts/Fonts.hpp"

namespace fs = std::filesystem;

// =============================================================================
// Constants
// =============================================================================

static constexpr int DISPLAY_WIDTH  = 256;   // Panel width in pixels (SSD1322 NHD 256x64)
static constexpr int DISPLAY_HEIGHT = 64;    // Panel height in pixels
static constexpr size_t BUFFER_BYTES = 2048; // 8 bands * 256 columns

// =============================================================================
// Host stand-in for PreconditionEngine's two read methods
// =============================================================================

// ConfiguringStatusBoard.cpp calls these on a bound engine. The harness renders it
// from snapshots (setPreconditions), so the engine pointer is always null and they
// never run; they exist only so the board links without PreconditionEngine.cpp,
// which needs the ESP-IDF logging headers.
size_t PreconditionEngine::getPreconditionCount() const { return 0; }
const PreconditionState& PreconditionEngine::getStateAt(size_t) const {
    static PreconditionState unused;
    return unused;
}

// =============================================================================
// Display
// =============================================================================

/**
 * @brief The firmware's U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI, with null I/O,
 *        in the state the legacy module establishes at begin.
 *
 * Same setup call and state calls as render_harness's HostDisplay (main.cpp),
 * mirroring U8G2Display.cpp lines 137-148.
 */
class HostDisplay : public U8G2 {
public:
    /** @brief Set up the 256x64 full buffer and reproduce the legacy begin state. */
    HostDisplay() : U8G2() {
        u8g2_Setup_ssd1322_nhd_256x64_f(&u8g2, U8G2_R0, u8x8_byte_empty, u8x8_dummy_cb);
        begin();                 // U8G2Display.cpp:137
        setDrawColor(1);         // :143
        setFontMode(1);          // :144
        setFontRefHeightAll();   // :145
        setFontPosTop();         // :146
        setContrast(0);          // :147
        setFlipMode(0);          // :148
    }
};

// =============================================================================
// Frames
// =============================================================================

/** @brief One board in one fixed state: how to draw it into a display. */
struct Frame {
    const char* name;                         ///< File stem
    std::function<void(U8G2&)> draw;          ///< Build the board, put it in the state, render one frame
};

/** @brief Set the value millis() answers with, as the boards read the clock through it. */
static void setMillis(unsigned long ms) { hostEnvironment().millis = ms; }

/** @brief A precondition snapshot for ConfiguringStatusBoard. */
static PreconditionSnapshot snapshot(const char* name, PreconditionRole role, Cause cause, Severity severity) {
    PreconditionSnapshot s;
    snprintf(s.name, sizeof(s.name), "%s", name);
    s.role     = role;
    s.cause    = cause;
    s.severity = severity;
    s.met      = (cause == Cause::Met);
    s.everMet  = s.met;
    return s;
}

/**
 * @brief The frames. Every string is fixed and chosen to contain descenders
 *        (g, j, p, q, y), because the descender row is what the default height
 *        decides.
 */
static const std::vector<Frame> kFrames = {  // The boot title is the root firmware's branding (src/main.cpp:82)
    {"error_nonfatal", [](U8G2& d) {
        ErrorBoard board(DISPLAY_WIDTH, DISPLAY_HEIGHT);
        board.showError("Sensor query failed: gyro offline", false);
        board.render(d);
    }},
    {"error_fatal", [](U8G2& d) {
        ErrorBoard board(DISPLAY_WIDTH, DISPLAY_HEIGHT);
        board.showError("Storage mount failed (flash)", true);
        board.render(d);
    }},
    {"wifi_wizard", [](U8G2& d) {
        WiFiWizardBoard board(DISPLAY_WIDTH, DISPLAY_HEIGHT);
        board.configure("Foundation-Setup", "192.168.4.1");
        board.render(d);
    }},
    {"gate_helper", [](U8G2& d) {
        GenericGateHelperBoard board(DISPLAY_WIDTH, DISPLAY_HEIGHT);
        board.configure("Weather API key", "Enter your key in the portal", "192.168.20.186");
        board.render(d);
    }},
    {"boot_splash", [](U8G2& d) {
        setMillis(1000);
        BootBoard board(DISPLAY_WIDTH, DISPLAY_HEIGHT, nullptr, "ESP32 App Foundation");
        board.activate("0.0.12-dev.91");
        board.render(d);                       // Still inside the 3 s splash: title, logo, version
    }},
    {"boot_loading", [](U8G2& d) {
        setMillis(1000);
        BootBoard board(DISPLAY_WIDTH, DISPLAY_HEIGHT, nullptr, "ESP32 App Foundation");
        board.activate("0.0.12-dev.91");
        setMillis(9000);                       // Past the splash: title, status, progress bar
        board.setStatus("Syncing the clock (pool.ntp.org)", 0.5f);
        board.render(d);
    }},
    {"ota_progress", [](U8G2& d) {
        OTAProgressBoard board(DISPLAY_WIDTH, DISPLAY_HEIGHT);
        board.reset();
        board.setProgress(0.42f, "Downloading update");
        board.render(d);
    }},
    {"configuring_items", [](U8G2& d) {
        ConfiguringStatusBoard board(DISPLAY_WIDTH, DISPLAY_HEIGHT);
        const PreconditionSnapshot items[] = {
            snapshot("WiFi Station", PreconditionRole::Startup, Cause::Met,        Severity::Transient),
            snapshot("API Key",      PreconditionRole::Startup, Cause::NotConfigured, Severity::NeedsUser),
            snapshot("Time Sync",    PreconditionRole::Startup, Cause::DependencyUnmet, Severity::Blocked),
        };
        board.setPreconditions(items, 3);
        board.setConnectionInfo("cb-iot", "http://192.168.20.186");
        board.render(d);
    }},
    {"configuring_waiting", [](U8G2& d) {
        ConfiguringStatusBoard board(DISPLAY_WIDTH, DISPLAY_HEIGHT);
        board.render(d);                       // No preconditions yet: "Waiting..."
    }},
};

// =============================================================================
// Output
// =============================================================================

/** @brief Whether pixel (x, y) is lit in a U8G2 tile buffer (band = y / 8, bit = y % 8). */
static bool lit(const std::vector<uint8_t>& buf, int x, int y) {
    return (buf[(y / 8) * DISPLAY_WIDTH + x] >> (y % 8)) & 1;
}

/** @brief Render one frame into a fresh display and copy the buffer out. */
static std::vector<uint8_t> renderFrame(const Frame& frame) {
    HostDisplay display;
    display.clearBuffer();
    frame.draw(display);
    const uint8_t* p = display.getBufferPtr();
    return std::vector<uint8_t>(p, p + BUFFER_BYTES);
}

/** @brief Write the raw U8G2 buffer. */
static bool writeBin(const fs::path& path, const std::vector<uint8_t>& buf) {
    FILE* f = fopen(path.string().c_str(), "wb");
    if (!f) return false;
    const bool ok = fwrite(buf.data(), 1, buf.size(), f) == buf.size();
    fclose(f);
    return ok;
}

/** @brief Write the frame as a binary PBM (P4): 256x64, MSB first, 1 = lit. */
static bool writePbm(const fs::path& path, const std::vector<uint8_t>& buf) {
    FILE* f = fopen(path.string().c_str(), "wb");
    if (!f) return false;
    fprintf(f, "P4\n%d %d\n", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
        for (int byteX = 0; byteX < DISPLAY_WIDTH / 8; ++byteX) {
            uint8_t out = 0;
            for (int bit = 0; bit < 8; ++bit) {
                if (lit(buf, byteX * 8 + bit, y)) out |= static_cast<uint8_t>(0x80 >> bit);
            }
            fputc(out, f);
        }
    }
    fclose(f);
    return true;
}

/** @brief Read a <frame>.bin written by --out. */
static bool readBin(const fs::path& path, std::vector<uint8_t>& buf) {
    FILE* f = fopen(path.string().c_str(), "rb");
    if (!f) return false;
    buf.assign(BUFFER_BYTES, 0);
    const bool ok = fread(buf.data(), 1, BUFFER_BYTES, f) == BUFFER_BYTES;
    fclose(f);
    return ok;
}

/** @brief Print a frame, '#' lit and '.' dark. */
static void printAscii(const std::vector<uint8_t>& buf) {
    for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
        for (int x = 0; x < DISPLAY_WIDTH; ++x) putchar(lit(buf, x, y) ? '#' : '.');
        putchar('\n');
    }
}

/**
 * @brief Print the rows of two frames that differ, as one ASCII picture.
 * @return The number of differing pixels.
 */
static int printDiff(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    int differing = 0;
    for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
        bool rowDiffers = false;
        int minX = DISPLAY_WIDTH, maxX = -1;
        for (int x = 0; x < DISPLAY_WIDTH; ++x) {
            if (lit(a, x, y) != lit(b, x, y)) {
                rowDiffers = true;
                ++differing;
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
            }
        }
        if (!rowDiffers) continue;
        printf("row %2d, columns %3d..%3d: ", y, minX, maxX);
        for (int x = minX; x <= maxX; ++x) {
            const bool la = lit(a, x, y), lb = lit(b, x, y);
            putchar(la && lb ? '#' : la ? '-' : lb ? '+' : '.');
        }
        putchar('\n');
    }
    return differing;
}

/** @brief Print usage and return a failure code. */
static int usage() {
    fprintf(stderr, "usage: render_legacy_boards --out <dir> | --ascii <frame> | --diff <dirA> <dirB>\nframes:");
    for (const Frame& f : kFrames) fprintf(stderr, " %s", f.name);
    fprintf(stderr, "\n");
    return 2;
}

int main(int argc, char** argv) {
    // --- Step 1: arguments ---
    fs::path outDir, diffA, diffB;
    std::string asciiFrame;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--out" && i + 1 < argc)         outDir = argv[++i];
        else if (arg == "--ascii" && i + 1 < argc)  asciiFrame = argv[++i];
        else if (arg == "--diff" && i + 2 < argc) { diffA = argv[i + 1]; diffB = argv[i + 2]; i += 2; }
        else return usage();
    }
    if (outDir.empty() && asciiFrame.empty() && diffA.empty()) return usage();

    // --- Step 2: diff two earlier outputs ---
    if (!diffA.empty()) {
        int framesDiffering = 0;
        for (const Frame& f : kFrames) {
            std::vector<uint8_t> a, b;
            if (!readBin(diffA / (std::string(f.name) + ".bin"), a) ||
                !readBin(diffB / (std::string(f.name) + ".bin"), b)) {
                fprintf(stderr, "missing %s.bin in %s or %s\n", f.name, diffA.c_str(), diffB.c_str());
                return 1;
            }
            printf("== %s\n", f.name);
            const int n = printDiff(a, b);
            if (n == 0) printf("identical\n");
            else { printf("%d pixels differ\n", n); ++framesDiffering; }
        }
        printf("\n%d of %zu frames differ\n", framesDiffering, kFrames.size());
        return 0;
    }

    // --- Step 3: render ---
    if (!outDir.empty()) fs::create_directories(outDir);
    for (const Frame& f : kFrames) {
        const std::vector<uint8_t> buf = renderFrame(f);
        if (asciiFrame == f.name) printAscii(buf);
        if (outDir.empty()) continue;
        int litCount = 0;
        for (int y = 0; y < DISPLAY_HEIGHT; ++y)
            for (int x = 0; x < DISPLAY_WIDTH; ++x) litCount += lit(buf, x, y) ? 1 : 0;
        if (!writeBin(outDir / (std::string(f.name) + ".bin"), buf) ||
            !writePbm(outDir / (std::string(f.name) + ".pbm"), buf)) {
            fprintf(stderr, "failed to write %s\n", f.name);
            return 1;
        }
        printf("%-20s lit=%d\n", f.name, litCount);
    }
    return 0;
}
