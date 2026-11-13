/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/BLEManager/BLESerialization.cpp
 * Description: Pure serialization and validation functions for BLE REST endpoints
 *              and MCP tools.
 */

#include "BLESerialization.hpp"
#include "BLEManager.hpp"
#include "IBLEClientProvider.hpp"
#include <strings.h>
#include <cstdio>

#if defined(ESP_PLATFORM) && defined(WITH_BLE)
#include <NimBLEDevice.h>
#endif

namespace BLESerialization {

static void bytesToHex(const uint8_t* data, size_t len, char* out, size_t outSize) {
    if (!data || !out || outSize < len * 2 + 1) {
        if (out && outSize > 0) out[0] = '\0';
        return;
    }
    static const char hexChars[] = "0123456789abcdef";
    for (size_t i = 0; i < len; ++i) {
        out[i * 2]     = hexChars[(data[i] >> 4) & 0x0F];
        out[i * 2 + 1] = hexChars[data[i] & 0x0F];
    }
    out[len * 2] = '\0';
}

const char* stateToString(BLEManagerState state) {
    switch (state) {
        case BLEManagerState::Disabled: return "Disabled";
        case BLEManagerState::Idle:     return "Idle";
        case BLEManagerState::Starting: return "Starting";
        case BLEManagerState::Active:   return "Active";
        case BLEManagerState::Paused:   return "Paused";
        case BLEManagerState::Error:    return "Error";
        default:                        return "Unknown";
    }
}

const char* addressTypeToString(BLEPeerAddress::Type type) {
    switch (type) {
        case BLEPeerAddress::Type::Public:              return "Public";
        case BLEPeerAddress::Type::RandomStatic:        return "RandomStatic";
        case BLEPeerAddress::Type::RandomResolvable:    return "RandomResolvable";
        case BLEPeerAddress::Type::RandomNonResolvable: return "RandomNonResolvable";
        default:                                        return "Unknown";
    }
}

bool parseAddressType(const char* str, BLEPeerAddress::Type& out) {
    if (!str) return false;
    if (strcasecmp(str, "Public") == 0) {
        out = BLEPeerAddress::Type::Public;
        return true;
    }
    if (strcasecmp(str, "RandomStatic") == 0) {
        out = BLEPeerAddress::Type::RandomStatic;
        return true;
    }
    if (strcasecmp(str, "RandomResolvable") == 0) {
        out = BLEPeerAddress::Type::RandomResolvable;
        return true;
    }
    if (strcasecmp(str, "RandomNonResolvable") == 0) {
        out = BLEPeerAddress::Type::RandomNonResolvable;
        return true;
    }
    if (strcasecmp(str, "Unknown") == 0) {
        out = BLEPeerAddress::Type::Unknown;
        return true;
    }
    return false;
}

const char* reconnectPhaseToString(BLEReconnectPolicy::Phase phase) {
    switch (phase) {
        case BLEReconnectPolicy::Phase::Idle:      return "Idle";
        case BLEReconnectPolicy::Phase::Fast:      return "Fast";
        case BLEReconnectPolicy::Phase::Slow:      return "Slow";
        case BLEReconnectPolicy::Phase::Exhausted: return "Exhausted";
        default:                                   return "Unknown";
    }
}

void serializeStatus(JsonObject root, const BLEManager& mgr, bool redactAddresses) {
    root["state"] = stateToString(mgr.getState());
    root["isScanning"] = mgr.isScanning();
    root["isAdvertising"] = mgr.isAdvertising();
    root["advertisedName"] = mgr.getConfig().advertisedName;
#if defined(ESP_PLATFORM) && defined(WITH_BLE)
    root["bleAddress"] = NimBLEDevice::getAddress().toString();
#else
    root["bleAddress"] = "";
#endif

    JsonObject adv = root["advertising"].to<JsonObject>();
    const auto& pack = mgr.getLastPackResult();
    JsonObject budget = adv["budget"].to<JsonObject>();
    budget["advUsed"] = pack.budget.advUsed;
    budget["advMax"] = pack.budget.advMax;
    budget["scanRespUsed"] = pack.budget.scanRespUsed;
    budget["scanRespMax"] = pack.budget.scanRespMax;

    char hexBuf[70];
    bytesToHex(pack.primaryAdv.data, pack.primaryAdv.length, hexBuf, sizeof(hexBuf));
    adv["primaryAdvHex"] = hexBuf;
    bytesToHex(pack.scanResponse.data, pack.scanResponse.length, hexBuf, sizeof(hexBuf));
    adv["scanResponseHex"] = hexBuf;
    adv["hasFault"] = pack.hasFault();

    JsonArray faults = adv["faults"].to<JsonArray>();
    for (const auto& fault : pack.faults) {
        JsonObject fObj = faults.add<JsonObject>();
        fObj["description"] = fault.description;
        fObj["requiredBytes"] = fault.requiredBytes;
        fObj["availableBytes"] = fault.availableBytes;
    }

    JsonArray conns = root["connections"].to<JsonArray>();
    for (const auto& conn : mgr.getActiveConnections()) {
        JsonObject cObj = conns.add<JsonObject>();
        cObj["handle"] = conn.handle;

        char addrBuf[24] = {};
        char redBuf[24] = {};
        if (redactAddresses) {
            conn.address.toRedactedString(addrBuf, sizeof(addrBuf));
        } else {
            conn.address.toString(addrBuf, sizeof(addrBuf));
        }
        conn.address.toRedactedString(redBuf, sizeof(redBuf));
        cObj["address"] = addrBuf;
        cObj["addressRedacted"] = redBuf;
        cObj["mtu"] = conn.mtu;
        cObj["encrypted"] = conn.encrypted;

        JsonObject params = cObj["params"].to<JsonObject>();
        params["minIntervalUnits"] = conn.params.minIntervalUnits;
        params["maxIntervalUnits"] = conn.params.maxIntervalUnits;
        params["latency"] = conn.params.latency;
        params["supervisionTimeoutUnits"] = conn.params.supervisionTimeoutUnits;
        params["minIntervalMs"] = conn.params.minIntervalUnits * 1.25f;
        params["maxIntervalMs"] = conn.params.maxIntervalUnits * 1.25f;
        params["intervalMs"] = (conn.params.minIntervalUnits + conn.params.maxIntervalUnits) * 0.5f * 1.25f;
        params["supervisionTimeoutMs"] = conn.params.supervisionTimeoutUnits * 10;
    }

    JsonArray devices = root["devices"].to<JsonArray>();
    for (auto* p : mgr.getClientProviders()) {
        if (!p) continue;
        const char* pid = p->getProviderId();
        JsonObject dObj = devices.add<JsonObject>();
        dObj["providerId"] = pid;
        dObj["label"] = p->getDeviceTypeLabel();
        dObj["connected"] = mgr.isClientConnected(pid);

        BLEStoredDevice stored;
        if (mgr.getStoredDevice(pid, stored)) {
            JsonObject sObj = dObj["stored"].to<JsonObject>();
            sObj["name"] = stored.name;
            char sAddrBuf[24] = {};
            char sRedBuf[24] = {};
            if (redactAddresses) {
                stored.address.toRedactedString(sAddrBuf, sizeof(sAddrBuf));
            } else {
                stored.address.toString(sAddrBuf, sizeof(sAddrBuf));
            }
            stored.address.toRedactedString(sRedBuf, sizeof(sRedBuf));
            sObj["address"] = sAddrBuf;
            sObj["addressRedacted"] = sRedBuf;
            sObj["addressType"] = addressTypeToString(stored.address.type);
            sObj["bonded"] = stored.bonded;
        } else {
            dObj["stored"] = nullptr;
        }

        BLEReconnectPolicy policy;
        if (mgr.getReconnectPolicy(pid, policy)) {
            JsonObject rObj = dObj["reconnect"].to<JsonObject>();
            rObj["phase"] = reconnectPhaseToString(policy.phase());
            rObj["suspended"] = policy.isSuspended();
        }
    }

    JsonObject counters = root["counters"].to<JsonObject>();
    counters["droppedEvents"] = mgr.getDroppedEventsCount();
    counters["scanInhibitCount"] = mgr.getScanInhibitCount();

    JsonObject disc = root["discovery"].to<JsonObject>();
    disc["active"] = mgr.isDiscoveryActive();
    disc["providerId"] = mgr.getDiscoveryProviderId() ? mgr.getDiscoveryProviderId() : "";
    disc["durationMs"] = mgr.getDiscoveryDurationMs();
}

void serializeScanResults(JsonArray root, const BLEScanResult* results, size_t count, bool redactAddresses) {
    if (!results) return;
    for (size_t i = 0; i < count; ++i) {
        const auto& r = results[i];
        JsonObject obj = root.add<JsonObject>();
        char rAddrBuf[24] = {};
        char rRedBuf[24] = {};
        if (redactAddresses) {
            r.address.toRedactedString(rAddrBuf, sizeof(rAddrBuf));
        } else {
            r.address.toString(rAddrBuf, sizeof(rAddrBuf));
        }
        r.address.toRedactedString(rRedBuf, sizeof(rRedBuf));
        obj["address"] = rAddrBuf;
        obj["addressRedacted"] = rRedBuf;
        obj["addressType"] = addressTypeToString(r.address.type);
        obj["name"] = r.name[0] != '\0' ? r.name : "";
        obj["rssi"] = r.rssi;
        obj["appearance"] = r.appearance;
        obj["connectable"] = r.connectable;
        if (r.uuid16Count > 0) {
            JsonArray uuids = obj["uuids16"].to<JsonArray>();
            for (uint8_t u = 0; u < r.uuid16Count; ++u) {
                uuids.add(r.uuids16[u]);
            }
        }
    }
}

} // namespace BLESerialization
