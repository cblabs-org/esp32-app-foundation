#pragma once
#include "Arduino.h"

#define WIFI_STA 1
#define WIFI_AP 2

class WiFiClass {
public:
    void begin() {}
    void disconnect(bool wifioff = false, bool eraseap = false) { (void)wifioff; (void)eraseap; }
    void mode(int m) { (void)m; }
    int status() { return WL_DISCONNECTED; }
    IPAddress localIP() { return IPAddress(0, 0, 0, 0); }
    IPAddress softAPIP() { return IPAddress(0, 0, 0, 0); }
    String macAddress() { return "AA:BB:CC:DD:EE:FF"; }
    void setHostname(const char* hostname) { (void)hostname; }
    bool softAP(const char* ssid, const char* passphrase = nullptr, int channel = 1, int ssid_hidden = 0, int max_connection = 4) { return true; }
    int32_t RSSI() { return -50; }
    int16_t scanNetworks(bool async = false, bool show_hidden = false, bool passive = false, uint32_t max_ms_per_chan = 300, uint8_t channel = 0) { return 0; }
    int getMode() { return WIFI_STA; }
};
inline WiFiClass WiFi;
