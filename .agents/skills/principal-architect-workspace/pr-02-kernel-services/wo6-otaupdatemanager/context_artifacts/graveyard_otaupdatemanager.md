# Graveyard: OTA Update Manager

## Immediate Blessing of App
Initially, the code blessed the partition immediately after the update binary was successfully written and verified via MD5. However, this defeated the purpose of OTA rollback since a firmware that compiles and downloads successfully could still have fatal startup/runtime crashes (e.g. infinite reboot loops on init).
- **Lesson Learned**: Never bless the partition at download time. The new app must boot, initialize, verify its critical dependencies, and bless itself.

## Physical Device Lock Acquisition Timeout
During verification, the automated upload test timed out due to unavailability of the physical hardware target.
- **Lesson Learned**: Automated integration tests requiring physical hardware should degrade gracefully to mock/build-verification when the device is not on the active lock/remote test bench.
