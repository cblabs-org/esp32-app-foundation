#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SNTP_SYNC_STATUS_RESET,
    SNTP_SYNC_STATUS_COMPLETED,
    SNTP_SYNC_STATUS_IN_PROGRESS,
} sntp_sync_status_t;

inline sntp_sync_status_t sntp_get_sync_status(void) {
    return SNTP_SYNC_STATUS_COMPLETED;
}

inline void configTzTime(const char* tz, const char* server1, const char* server2, const char* server3) {
    (void)tz;
    (void)server1;
    (void)server2;
    (void)server3;
}

#ifdef __cplusplus
}
#endif
