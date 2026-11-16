# Graveyard: OTA and Flash Failures

## OTA Upload — curl exit 56

**Attempted:** `curl -F "firmware=@firmware.bin" http://<device-ip>/api/system/ota`

**Result:** `curl: (56) Recv failure: Connection reset by peer` — transferred ~128KB of 1.4MB before the connection dropped.

**Root cause:** Unknown — possibly the ESP32's HTTP server runs out of buffer or the OTA partition write stalls under concurrent MQTT/weather activity. The device did not crash (web portal remained responsive).

**Workaround:** Use serial flash via PlatformIO MCP `upload_firmware` instead of OTA for development builds.

## Serial Port Busy — Errno 35

**Attempted:** `upload_firmware` via PlatformIO MCP

**Result:** `Could not exclusively lock port /dev/cu.usbserial-5AA70033491: [Errno 35] Resource temporarily unavailable`

**Root cause:** The PlatformIO MCP server's `agent_flash_monitor_verify` tool had left a serial monitor handle open from the previous flash cycle. The port was still locked when the next upload attempt ran.

**Fix:** Always call `stop_monitor` with the explicit port before retrying `upload_firmware`:
```
stop_monitor(port: "/dev/cu.usbserial-5AA70033491")
```

## MCP Flash Timeout (600s)

**Attempted:** `agent_flash_monitor_verify` with `timeoutSeconds: 30`

**Result:** Timed out at 600000ms (the MCP server's internal cap, not the 30s we requested).

**Root cause:** The tool combines flash + monitor + verification into one atomic operation. If the serial port is slow to enumerate after a device reset, the entire pipeline stalls.

**Lesson:** For iterative development, prefer the two-step approach: `upload_firmware` → manual `curl /api/system` verification. Reserve `agent_flash_monitor_verify` for CI-style full-pipeline validation.
