# Architecture Decision Record: Time Manager

## Non-Blocking NTP Synchronization
`TimeManager` handles NTP synchronization asynchronously. It uses ESP-IDF's SNTP client but avoids blocking the boot thread during synchronization. Instead, it periodically checks the SNTP status and logs updates when sync is achieved.

## POSIX Timezone Support
Timezones are configured using standard POSIX timezone environment variables (e.g. `TZ` set via `setenv()` and `tzset()`), enabling the system library (`time.h`, `localtime()`) to handle daylight saving time (DST) offsets automatically and correctly.
