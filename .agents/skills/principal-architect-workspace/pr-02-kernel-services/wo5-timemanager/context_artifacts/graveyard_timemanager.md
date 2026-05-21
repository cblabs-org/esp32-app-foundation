# Graveyard: Time Manager

## Busy-Wait Sync Loops
Initial sync code attempted to block execution using `while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED)` inside the main setup thread. This caused startup to hang for several seconds when internet connectivity was degraded.
- **Lesson Learned**: Always treat NTP sync status checks as asynchronous events and avoid blocking key system startup loops.
