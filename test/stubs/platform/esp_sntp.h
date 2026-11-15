#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <array>
#include <string>

#ifndef CONFIG_LWIP_DHCP_GET_NTP_SRV
#define CONFIG_LWIP_DHCP_GET_NTP_SRV 1
#endif

#ifndef CONFIG_LWIP_SNTP_MAX_SERVERS
#define CONFIG_LWIP_SNTP_MAX_SERVERS 3
#endif

#ifndef SNTP_MAX_SERVERS
#define SNTP_MAX_SERVERS CONFIG_LWIP_SNTP_MAX_SERVERS
#endif

#ifndef IPADDR_STRLEN_MAX
#define IPADDR_STRLEN_MAX 46
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SNTP_SYNC_STATUS_RESET,
    SNTP_SYNC_STATUS_COMPLETED,
    SNTP_SYNC_STATUS_IN_PROGRESS,
} sntp_sync_status_t;

enum lwip_ip_addr_type {
    IPADDR_TYPE_V4  = 0U,
    IPADDR_TYPE_V6  = 6U,
    IPADDR_TYPE_ANY = 46U
};

typedef struct ip4_addr {
    uint32_t addr;
} ip4_addr_t;

typedef struct ip_addr {
    union {
        ip4_addr_t ip4;
    } u_addr;
    uint8_t type;
} ip_addr_t;

struct pbuf {
    struct pbuf* next;
    void* payload;
    uint16_t tot_len;
    uint16_t len;
};

struct dhcp {
    uint8_t state;
};

enum {
    DHCP_STATE_OFF         = 0,
    DHCP_STATE_REQUESTING  = 1,
    DHCP_STATE_INIT        = 2,
    DHCP_STATE_REBOOTING   = 3,
    DHCP_STATE_REBINDING   = 4,
    DHCP_STATE_RENEWING    = 5,
    DHCP_STATE_SELECTING   = 6,
    DHCP_STATE_INFORMING   = 7,
    DHCP_STATE_CHECKING    = 8,
    DHCP_STATE_PERMANENT   = 9,
    DHCP_STATE_BOUND       = 10,
    DHCP_STATE_RELEASING   = 11,
    DHCP_STATE_BACKING_OFF = 12
};

void lwip_dhcp_on_extra_option(struct dhcp* dhcp, uint8_t state, uint8_t option,
                               uint8_t len, struct pbuf* p, uint16_t offset);

#ifdef __cplusplus
}
#endif

struct SntpStubSlot {
    ip_addr_t addr{};
    std::string nameStorage;
    bool hasName = false;
};

struct SntpStubState {
    sntp_sync_status_t syncStatus = SNTP_SYNC_STATUS_COMPLETED;
    bool enabled = false;
    bool dhcpMode = false;
    bool netifInitialized = false;
    bool sntpCalledBeforeNetifInit = false;
    size_t netifInitCount = 0;
    std::array<SntpStubSlot, SNTP_MAX_SERVERS> slots{};
};

inline SntpStubState& stub_sntp_state() {
    static SntpStubState state;
    return state;
}

inline void stub_sntp_reset() {
    auto& st = stub_sntp_state();
    st.syncStatus = SNTP_SYNC_STATUS_COMPLETED;
    st.enabled = false;
    st.dhcpMode = false;
    st.netifInitialized = false;
    st.sntpCalledBeforeNetifInit = false;
    st.netifInitCount = 0;
    for (size_t i = 0; i < SNTP_MAX_SERVERS; ++i) {
        st.slots[i].addr = ip_addr_t{};
        st.slots[i].nameStorage.clear();
        st.slots[i].hasName = false;
    }
}

inline void stub_sntp_set_sync_status(sntp_sync_status_t status) {
    stub_sntp_state().syncStatus = status;
}

inline bool stub_sntp_is_dhcp_mode() {
    return stub_sntp_state().dhcpMode;
}

inline ip4_addr_t stub_sntp_make_ip4(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    ip4_addr_t out{};
    out.addr = static_cast<uint32_t>(a) |
               (static_cast<uint32_t>(b) << 8) |
               (static_cast<uint32_t>(c) << 16) |
               (static_cast<uint32_t>(d) << 24);
    return out;
}

inline bool ip_addr_isany(const ip_addr_t* addr) {
    return (addr == nullptr) || (addr->u_addr.ip4.addr == 0);
}

inline bool ip_addr_isany_val(ip_addr_t addr) {
    return addr.u_addr.ip4.addr == 0;
}

inline char* ipaddr_ntoa_r(const ip_addr_t* addr, char* buf, int buflen) {
    if (addr == nullptr || buf == nullptr || buflen <= 0) {
        return nullptr;
    }
    uint32_t v = addr->u_addr.ip4.addr;
    snprintf(buf, static_cast<size_t>(buflen), "%u.%u.%u.%u",
             static_cast<unsigned>(v & 0xFFu),
             static_cast<unsigned>((v >> 8) & 0xFFu),
             static_cast<unsigned>((v >> 16) & 0xFFu),
             static_cast<unsigned>((v >> 24) & 0xFFu));
    return buf;
}

inline uint16_t pbuf_copy_partial(const struct pbuf* buf, void* dataptr, uint16_t len, uint16_t offset) {
    if (buf == nullptr || dataptr == nullptr || len == 0) {
        return 0;
    }
    uint16_t copied = 0;
    uint16_t left = len;
    uint8_t* dst = static_cast<uint8_t*>(dataptr);
    const struct pbuf* p = buf;
    while (p != nullptr && left > 0) {
        if (offset >= p->len) {
            offset = static_cast<uint16_t>(offset - p->len);
        } else {
            if (p->payload == nullptr) {
                break;
            }
            uint16_t avail = static_cast<uint16_t>(p->len - offset);
            uint16_t chunk = (left < avail) ? left : avail;
            memcpy(dst + copied, static_cast<const uint8_t*>(p->payload) + offset, chunk);
            copied = static_cast<uint16_t>(copied + chunk);
            left = static_cast<uint16_t>(left - chunk);
            offset = 0;
        }
        p = p->next;
    }
    return copied;
}

inline sntp_sync_status_t sntp_get_sync_status(void) {
    return stub_sntp_state().syncStatus;
}

inline bool esp_sntp_enabled(void) {
    return stub_sntp_state().enabled;
}

inline void esp_sntp_stop(void) {
    auto& st = stub_sntp_state();
    if (!st.netifInitialized) {
        st.sntpCalledBeforeNetifInit = true;
    }
    st.enabled = false;
}

inline void esp_sntp_setserver(uint8_t idx, const ip_addr_t* addr) {
    auto& st = stub_sntp_state();
    if (!st.netifInitialized) {
        st.sntpCalledBeforeNetifInit = true;
    }
    if (idx < SNTP_MAX_SERVERS) {
        if (addr != nullptr) {
            st.slots[idx].addr = *addr;
        } else {
            st.slots[idx].addr = ip_addr_t{};
        }
        st.slots[idx].hasName = false;
        st.slots[idx].nameStorage.clear();
    }
}

inline void esp_sntp_setservername(uint8_t idx, const char* server) {
    auto& st = stub_sntp_state();
    if (!st.netifInitialized) {
        st.sntpCalledBeforeNetifInit = true;
    }
    if (idx < SNTP_MAX_SERVERS) {
        if (server != nullptr) {
            st.slots[idx].nameStorage = server;
            st.slots[idx].hasName = true;
        } else {
            st.slots[idx].nameStorage.clear();
            st.slots[idx].hasName = false;
        }
    }
}

inline const char* esp_sntp_getservername(uint8_t idx) {
    auto& st = stub_sntp_state();
    if (idx < SNTP_MAX_SERVERS && st.slots[idx].hasName) {
        return st.slots[idx].nameStorage.c_str();
    }
    return nullptr;
}

inline const ip_addr_t* esp_sntp_getserver(uint8_t idx) {
    auto& st = stub_sntp_state();
    if (idx < SNTP_MAX_SERVERS) {
        return &st.slots[idx].addr;
    }
    static const ip_addr_t anyAddr{};
    return &anyAddr;
}

inline void esp_sntp_servermode_dhcp(bool enable) {
    auto& st = stub_sntp_state();
    if (!st.netifInitialized) {
        st.sntpCalledBeforeNetifInit = true;
    }
    st.dhcpMode = enable;
}

inline void stub_sntp_simulate_dhcp_servers(uint8_t num, const ip4_addr_t* servers) {
    auto& st = stub_sntp_state();
    if (st.dhcpMode && num > 0 && servers != nullptr) {
        uint8_t i = 0;
        for (i = 0; i < num && i < SNTP_MAX_SERVERS; ++i) {
            ip_addr_t addr{};
            addr.u_addr.ip4 = servers[i];
            addr.type = IPADDR_TYPE_V4;
            esp_sntp_setserver(i, &addr);
        }
        for (; i < SNTP_MAX_SERVERS; ++i) {
            esp_sntp_setserver(i, nullptr);
        }
    }
}

inline void configTzTime(const char* tz, const char* server1, const char* server2, const char* server3) {
    auto& st = stub_sntp_state();
    st.netifInitialized = true;
    ++st.netifInitCount;
    if (esp_sntp_enabled()) {
        esp_sntp_stop();
    }
    esp_sntp_setservername(0, server1);
    esp_sntp_setservername(1, server2);
    esp_sntp_setservername(2, server3);
    st.enabled = true;
    if (tz != nullptr) {
        setenv("TZ", tz, 1);
        tzset();
    }
}
