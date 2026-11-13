---
name: pr-12-wo8-ap-ssid-connection-info
description: Fix WiFi wizard AP name + add connection info footer to configuring board
---

# WO8 — AP SSID Fix + Connection Info Footer

You are an expert senior software engineer and execution-focused worker agent.

## CRITICAL: WORKSPACE
ALL operations MUST use `/Users/mcneillm/Documents/Projects/esp32-app-foundation` as the working directory.

## Branch

```bash
git checkout pr-12-state-machine
```

## Changes Required

### FIX 1: WiFi Wizard Always Uses Hostname

**File:** `modules/u8g2Display/u8g2DisplayModule.cpp`
**Location:** `_onStateChanged()` method, the WiFi gate branch (~L898-910)

**Problem:** When the state first transitions to CONFIGURING, WiFi hasn't timed out yet. `getAPMode()` returns false, so the code uses the stored SSID ("<home-ssid>") instead of the hostname. The AP mode only activates ~15s later.

**Fix:** The WiFi wizard is shown when the WiFi gate is unsatisfied. This means the device IS or WILL BE in AP mode. The AP always broadcasts as the hostname. So always use hostname:

```cpp
if (strcmp(gate->getGateName(), "WiFi Station") == 0) {
    // WiFi gate unsatisfied — device is/will-be in AP mode.
    // AP always broadcasts as hostname.
    _wifiWizard.configure(_app->getConfig().hostname);
    showBoard(&_wifiWizard);
    ESP_LOGI(TAG, "State → CONFIGURING: showing WiFi wizard");
    return;
}
```

Remove the entire `getAPMode()` / `getSSID()` conditional block. Replace with the single line above.

### FIX 2: Connection Info Footer on ConfiguringStatusBoard

**Purpose:** When non-WiFi gates are unsatisfied (WiFi IS connected), the user needs to know how to reach the web portal to configure the device. Show a small footer at the bottom of the configuring status board.

#### 2a. Add `setConnectionInfo()` to `configuringStatusBoard.hpp`

Add a public method and private members:

```cpp
public:
    /**
     * @brief Set network connection info for the footer display.
     * @param network  The WiFi network name (SSID) the device is connected to.
     * @param url      The device URL (e.g., "http://192.168.1.42").
     */
    void setConnectionInfo(const char* network, const char* url);

private:
    char _networkName[33] = "";   ///< Connected WiFi network name
    char _deviceUrl[48] = "";     ///< Device URL for portal access
```

#### 2b. Implement `setConnectionInfo()` in `configuringStatusBoard.cpp`

```cpp
void ConfiguringStatusBoard::setConnectionInfo(const char* network, const char* url) {
    if (network) {
        strncpy(_networkName, network, sizeof(_networkName) - 1);
        _networkName[sizeof(_networkName) - 1] = '\0';
    }
    if (url) {
        strncpy(_deviceUrl, url, sizeof(_deviceUrl) - 1);
        _deviceUrl[sizeof(_deviceUrl) - 1] = '\0';
    }
}
```

#### 2c. Render the footer in `render()` 

After the progress bar rendering (inside the `#ifndef NATIVE_BUILD` block), add a footer line at the bottom of the display. The display is 256×64. The progress bar is at Y=38 with height 16, so it ends at Y=54. Use the remaining space (Y=56-63) for a small footer.

```cpp
// --- 4. Draw connection info footer ---
if (_networkName[0] != '\0' && _deviceUrl[0] != '\0') {
    char footerBuf[80];
    snprintf(footerBuf, sizeof(footerBuf), "%s  %s", _networkName, _deviceUrl);
    u8g2.setFont(AppFonts::defaultSmall());
    // Centre the footer text at the very bottom
    int textWidth = u8g2.getStrWidth(footerBuf);
    int footerX = (_displayWidth - textWidth) / 2;
    if (footerX < 0) footerX = 0;
    u8g2.drawStr(footerX, _displayHeight - 2, footerBuf);
}
```

**Note:** `u8g2_font_helvR08_tr` (defaultSmall) has ~8px ascent. `drawStr` uses baseline Y. So `_displayHeight - 2` = 62 on a 64px display, placing the text baseline at Y=62 with the top of glyphs at ~Y=54. This sits right below the progress bar.

#### 2d. Pass connection info from `_onStateChanged()`

In `_onStateChanged()`, when showing the configuring status board for non-WiFi gates, pass the connection info:

```cpp
// Non-WiFi gate: show configuring status board with connection info
_configuringStatusBoard.setConnectionInfo(
    _app->getWifiManager().getSSID(),
    _app->getWifiManager().getMyUrl()
);
showBoard(&_configuringStatusBoard);
ESP_LOGI(TAG, "State → CONFIGURING: showing configuring status board");
return;
```

## Line-Item Checklist

- [ ] Read `modules/u8g2Display/u8g2DisplayModule.cpp` — find `_onStateChanged()` 
- [ ] Read `modules/u8g2Display/boards/configuringStatusBoard.cpp` — understand render layout
- [ ] Read `modules/u8g2Display/boards/configuringStatusBoard.hpp` — understand members
- [ ] FIX 1: Simplify WiFi wizard AP name to always use hostname
- [ ] FIX 2a: Add `setConnectionInfo()` declaration + private char arrays to hpp
- [ ] FIX 2b: Implement `setConnectionInfo()` in cpp
- [ ] FIX 2c: Add footer rendering in `render()` after progress bar
- [ ] FIX 2d: Call `setConnectionInfo()` from `_onStateChanged()` for non-WiFi gates
- [ ] Verify: `pio test -e native --filter "test_unit/test_u8g2Display"` — all tests pass
- [ ] Verify: `pio test -e native --filter "test_unit/test_application"` — all tests pass
- [ ] Verify: `pio run -e esp32s3nano` — hardware build succeeds

## Verification

After completing all items, run tests and hardware build. Then **STOP and send a message back** reporting:
1. Files modified
2. Test results
3. Any design decisions

Do NOT commit until I explicitly approve.
