# Graveyard: Boot/WiFi Debugging Dead-ends

## Investigation Dead-end 1: esptool Hard Reset via `pio pkg exec`

**Attempt:** Used `pio pkg exec --package "platformio/tool-esptoolpy" -- esptool.py
run --before default_reset --after hard_reset` to force a hardware reset and capture
a clean boot sequence.

**Failure:**
```
ModuleNotFoundError: No module named 'intelhex'
```
The PlatformIO-sandboxed `tool-esptoolpy` package has a broken dependency
(`intelhex` not available in its isolated Python env). The `pio pkg exec` sandbox
does NOT inherit the host Python environment.

**Resolution:** Used the PlatformIO venv Python directly with `pyserial` to pulse
RTS/DTR manually:
```bash
~/.platformio/penv/bin/python -c "
import serial, time
s = serial.Serial('/dev/cu.usbmodem101', 115200)
s.dtr = False; s.rts = True; time.sleep(0.1); s.rts = False; s.close()
"
```

---

## Investigation Dead-end 2: Credential Storage Suspected First

**Initial hypothesis:** WiFi password was not being saved or loaded correctly —
user reported having set it twice without success.

**Why it was wrong:** The credential round-trip was working fine. Logs on the second
boot clearly showed:
```
WIFI_MGR: Config applied — SSID: cb-iot
CRYPTO: Decryption verified. Extracted plaintext configuration length: zu bytes
WIFI_MGR: Config applied — password retrieved from KeyManager
```

The actual failure was *after* retrieval — the returned password string contained
silent garbage bytes appended by the PKCS7 padding bug. Storage and retrieval of
the *corrupted* value worked correctly; the corruption was in the padding removal.

---

## Investigation Dead-end 3: `%zu` Suspected as Symptom of Deeper Crypto Bug

**Initial hypothesis:** The `%zu` printing as literal "zu" might indicate a non-
standard toolchain config that was also corrupting the decryption output.

**Why it was wrong:** `%zu` is purely a Newlib nano logging formatter limitation.
The decryption itself was functionally executing correctly — it was the padding
removal that introduced the corruption, completely independently of the logging.

---

## Investigation Dead-end 4: Band/AP Suspected for AUTH_EXPIRE

**Initial hypothesis:** `cb-iot` might be 5GHz-only (ESP32-S3 is 2.4GHz only).

**Ruled out via:** UniFi MCP `get_wlan_configs` — confirmed `cb-iot` is a 2.4GHz
WPA2 (`wpapsk`) network. Device MAC `80:65:99:a1:8e:90` also never appeared in
client history, but this was because the password was always wrong, not because of
band mismatch.

---

## Stale Monitor Claim Found on Boot

On the first `list_devices` call, `/dev/cu.debug-console` had a stale monitor claim
from PID 31993. This was cleared with `stop_monitor` before starting the real monitor
on `/dev/cu.usbmodem101`. Future agents: always check for stale claims before starting
a new monitor session.
