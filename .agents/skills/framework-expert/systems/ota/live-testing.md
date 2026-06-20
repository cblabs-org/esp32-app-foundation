# OTA Live Testing — End-to-End Verification Pattern

This is the canonical sequence for live-testing the OTA update flow on a
physical device. It produces two firmware versions: one flashed directly via
USB (the "baseline"), and one deployed to the releases repo for the device to
discover and install over-the-air.

---

## Prerequisites

- Device connected via USB
- `private_key.pem` available for signing
- `releases/` repo clone is up-to-date (`git -C releases pull`)
- PlatformIO MCP server running

---

## Sequence

### Phase 1 — Flash the baseline firmware

The baseline is the version the device **boots with**. It contains whatever
code changes you want to test (new manifest URL, new OTA logic, etc.).

```
1. Tag the version         git tag v0.0.X-dev.N
2. Build the firmware      MCP: build_project (esp32dev)
3. Flash the device        MCP: upload_firmware (esp32dev)
```

The device is now running `v0.0.X-dev.N` with your latest code. Confirm it
boots and connects to WiFi before proceeding.

### Phase 2 — Deploy the OTA target

The OTA target is a **newer** version that the device should discover and
install. It must be higher than the baseline per SemVer.

```
4. Bump the version        git tag v0.0.X-dev.(N+1)
5. Build the firmware      MCP: build_project (esp32dev)
6. Sign and deploy:
   a. Sign firmware        openssl dgst -sha256 -sign private_key.pem \
                             -out release/firmware-esp32dev.bin.sig \
                             .pio/build/esp32dev/firmware.bin
   b. Create release       gh release create v0.0.X-dev.(N+1) \
                             --repo owner/releases-repo \
                             --prerelease \
                             firmware.bin firmware.bin.sig firmware.bin.sha256
   c. Generate manifest    python3 scripts/generate_manifest.py --merge-into ...
   d. Sign manifest        openssl dgst -sha256 -sign private_key.pem \
                             -out releases/manifest.json.sig \
                             releases/manifest.json
   e. Commit manifest      cd releases && git add manifest.json manifest.json.sig \
                             && git commit -m "chore: update manifest" && git push
```

Or use the deploy script once WO2 lands:
```
   python3 scripts/deploy_release.py --local --private-key private_key.pem
```

### Phase 3 — Monitor and verify OTA

```
7. Start serial monitor    MCP: start_monitor (esp32dev)
8. Trigger OTA check       Device MCP: ota_check_now
                           — or wait for the automatic daily/boot check
9. Verify:
   a. Serial logs          MCP: query_logs — watch for the 3-gate sequence:
                             Gate 1: Manifest signature verified ✓
                             Gate 2: Firmware signature verified ✓
                             Gate 3: Binary header + version match ✓
   b. Device MCP           system_info → firmware_version should show v0.0.X-dev.(N+1)
   c. Re-trigger           ota_check_now → should report "no update available"
```

---

## What the 3-gate log sequence looks like

```
[OTAUpdate] Fetching manifest from https://raw.githubusercontent.com/.../manifest.json
[OTAUpdate] Manifest signature verified ✓
[OTAUpdate] Update available: v0.0.X-dev.(N+1) (current: v0.0.X-dev.N)
[OTAUpdate] Downloading firmware...
[OTAUpdate] Firmware signature verified ✓
[OTAUpdate] Binary header validated (magic: 0xE9, version match) ✓
[OTAUpdate] Update applied — rebooting
```

After reboot, `system_info` should report the new version.

---

## Common Failure Modes

| Symptom | Likely Cause |
|---|---|
| Manifest fetch 404 | Manifest not committed to releases repo, or URL wrong in `main.cpp` |
| Manifest signature fails | Manifest was modified after signing, or wrong public key compiled in |
| Firmware download 404 | Binary not uploaded to release, or `firmwareUrl` in manifest is wrong |
| Firmware signature fails | Binary was re-built after signing (different hash), or wrong key |
| "No update available" | Device already on latest version, or channel mismatch (device on `dev`, manifest only has `stable`) |
| Version mismatch at Gate 3 | Binary was built from wrong tag — `esp_app_desc_t.version` doesn't match manifest |
| TLS handshake failure | CA bundle doesn't cover the download host — check `OTACerts.h` |
