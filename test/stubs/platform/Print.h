/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Stub: test/stubs/platform/Print.h
 * Description: Redirect-to-Arduino.h stub for the U8g2lib.h include chain.
 *              U8g2lib.h includes both <Arduino.h> and <Print.h>; our Arduino.h
 *              stub already defines the Print base class, so this file simply
 *              re-includes it to avoid a "Print.h not found" error.
 */
#pragma once
#include "Arduino.h"
