# Graveyard: Boot/WiFi Debugging Dead-ends

## Dead-end 1: `pio pkg exec` esptool Hard Reset

**Attempt:** `pio pkg exec --package "platformio/tool-esptoolpy" -- esptool.py ...`

**Failure:** `ModuleNotFoundError: No module named 'intelhex'` — the PlatformIO
sandboxed Python env is missing `intelhex`. Does NOT inherit host Python.

**Resolution:** Used `~/.platformio/penv/bin/python` with `pyserial` to pulse
RTS/DTR directly for a hard reset.

---

## Dead-end 2: Credential Storage Suspected First

**Hypothesis:** Password not saving or loading correctly.

**Why wrong:** Round-trip was working — logs showed SSID and "Decryption verified"
on boot. The corruption was post-decryption in the padding removal, not in storage.

---

## Dead-end 3: `%zu` Suspected as Deeper Crypto Symptom

**Hypothesis:** `%zu` printing literally "zu" might indicate toolchain issues
also corrupting decryption.

**Why wrong:** Newlib nano limitation — purely cosmetic. Decryption logic itself
is correct; corruption was only in padding removal.

---

## Dead-end 4: 5GHz Band Mismatch

**Hypothesis:** `<home-ssid>` might be 5GHz-only (ESP32-S3 is 2.4GHz only).

**Ruled out via:** the access point's WLAN configuration — confirmed 2.4GHz WPA2.

---

## Operational Note: Stale Monitor Claims

On first `list_devices`, `/dev/cu.debug-console` had a stale monitor claim from
PID 31993. Always check for stale claims before starting a monitor session.
