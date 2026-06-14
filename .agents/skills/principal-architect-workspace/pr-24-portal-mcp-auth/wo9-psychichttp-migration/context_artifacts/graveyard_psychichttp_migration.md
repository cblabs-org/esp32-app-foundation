# Graveyard: PsychicHttp Migration (WO9)

## Failed Approaches and Dead Ends

1. **Returning void in PsychicHttp handlers**:
   * **Issue**: We initially left some handler methods (like in `WifiManager` and `TimeManager`) returning `void` while trying to map them to `PsychicHttpServer::on` which expects `esp_err_t`.
   * **Result**: `error: no matching function for call to 'PsychicHttpServer::on(..., <lambda>)'` and `error: return-statement with a value, in function returning 'void'`.
   * **Resolution**: All handlers *must* explicitly return an `esp_err_t` (e.g., `ESP_OK` or `ESP_FAIL`). They cannot be `void`.

2. **Doxygen block syntax masking**:
   * **Issue**: In `OTAUpdateManager.cpp`, a missing `*/` on the `handleSignedUpload` method caused the compiler to interpret the entire method signature as a comment. This resulted in cryptic `expected unqualified-id before 'void'` errors further down the file.
   * **Resolution**: Strict auditing of Doxygen block closure when updating method parameter types.

3. **Incomplete AuthManager argument passing**:
   * **Issue**: During the migration of `AuthManager`, we updated `checkMcpAuth` to take both `PsychicRequest*` and `PsychicResponse*`. However, `MCPServer` only passed `request`.
   * **Result**: Compilation failure for missing arguments.
   * **Resolution**: All calls to `checkMcpAuth` and `checkWebAuth` must pass both `request` and `response` pointers.

4. **Missing sdkconfig defaults**:
   * **Issue**: Missing IPv6 and HTTPS support caused ESP-IDF build errors during the PsychicHttp underlying stack compilation.
   * **Resolution**: Added `CONFIG_LWIP_IPV6=y` and `CONFIG_ESP_HTTPS_SERVER_ENABLE=y` to `sdkconfig.defaults`.
