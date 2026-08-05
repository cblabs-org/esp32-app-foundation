# Third-Party Notices

This project (`esp32-app-foundation`) incorporates third-party open-source software dependencies. This file documents the external third-party components referenced in `platformio.ini`, their repository sources, and their corresponding license types.

All third-party dependencies listed below utilize permissive open-source licenses (such as MIT, BSD-2-Clause, BSD-3-Clause, and Apache-2.0) that are fully compatible with this project's **Apache License, Version 2.0**.

---

## External Dependencies

| Library / Dependency | Repository / Source | License Type | Description / Usage |
| :--- | :--- | :--- | :--- |
| **ArduinoJson** | [github.com/bblanchon/ArduinoJson](https://github.com/bblanchon/ArduinoJson) | MIT | C++ JSON library for IoT & embedded systems |
| **PsychicHttp** | [github.com/hoeken/PsychicHttp](https://github.com/hoeken/PsychicHttp) | MIT | Asynchronous HTTP server library for ESP32 |
| **U8g2** | [github.com/olikraus/u8g2](https://github.com/olikraus/u8g2) | BSD-2-Clause | Monochromatic display library for embedded devices |
| **JsonStreamingParser** | [github.com/squix78/json-streaming-parser](https://github.com/squix78/json-streaming-parser) | MIT | Memory-efficient streaming JSON parser for large payloads |
| **LVGL (Light and Versatile Graphics Library)** | [github.com/lvgl/lvgl](https://github.com/lvgl/lvgl) | MIT | Embedded graphics library for HMI display interfaces |
| **ESP32Encoder** | [github.com/Madhephaestus/ESP32Encoder](https://github.com/Madhephaestus/ESP32Encoder) | MIT | Hardware rotary encoder pulse counting library |
| **LovyanGFX** | [github.com/lovyan03/LovyanGFX](https://github.com/lovyan03/LovyanGFX) | FreeBSD (BSD-2-Clause) | High-performance display driver graphics library |
| **GoogleTest** | [github.com/google/googletest](https://github.com/google/googletest) | BSD-3-Clause | Host C++ unit testing framework (`env:native`) |

---

## Internal Modules & Framework Dependencies

| Module / Framework | Source Location | License Type | Description / Usage |
| :--- | :--- | :--- | :--- |
| **Arduino-ESP32 / ESP-IDF** | [github.com/espressif/arduino-esp32](https://github.com/espressif/arduino-esp32) | LGPL-2.1 / Apache-2.0 | Base microcontroller framework and SDK |

---

## License Compatibility Statement

All third-party libraries bundled or referenced by `esp32-app-foundation` in `platformio.ini` use OSI-approved, permissive open-source licenses (MIT, BSD-2-Clause, BSD-3-Clause, Apache-2.0) or weak-copyleft licenses for the underlying SDK (LGPL-2.1). These licenses allow royalty-free use, modification, and distribution in open-source and commercial applications, confirming complete compatibility with the project's **Apache License, Version 2.0**.
