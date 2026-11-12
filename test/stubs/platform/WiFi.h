#pragma once
#include "Arduino.h"

#define WIFI_STA 1
#define WIFI_AP 2
#define WIFI_AP_STA 3
#define WIFI_OFF 0
#define WIFI_PS_NONE 0
#define WIFI_AUTH_OPEN 0
#define WIFI_SCAN_FAILED -1
#define WIFI_SCAN_RUNNING -2

#define WIFI_FAST_SCAN 0
#define WIFI_ALL_CHANNEL_SCAN 1
#define WIFI_CONNECT_AP_BY_SIGNAL 0
#define WIFI_CONNECT_AP_BY_SECURITY 1

class WiFiClass {
public:
    void persistent(bool) {}
    void setSleep(int) {}
    void setScanMethod(int) {}
    void setSortMethod(int) {}
    void begin(const char* ssid = nullptr, const char* pass = nullptr, int32_t channel = 0, const uint8_t* bssid = nullptr, bool tryConnect = true) {
        (void)ssid; (void)pass; (void)channel; (void)bssid; (void)tryConnect;
    }
    void disconnect(bool wifioff = false, bool eraseap = false) { (void)wifioff; (void)eraseap; }
    void mode(int m) { (void)m; }
    int status() { return WL_CONNECTED; }
    bool isConnected() { return status() == WL_CONNECTED; }
    IPAddress localIP() { return IPAddress(192, 168, 1, 100); }
    IPAddress softAPIP() { return IPAddress(0, 0, 0, 0); }
    std::string macAddress() { return "AA:BB:CC:DD:EE:FF"; }
    std::string softAPmacAddress() { return "AA:BB:CC:DD:EE:FF"; }
    std::string SSID(int i = 0) { (void)i; return "TEST_SSID"; }
    std::string softAPSSID() { return "TEST_AP_SSID"; }
    std::string last_softAP_ssid;
    std::string last_hostname;
    void resetTestStubs() {
        last_softAP_ssid.clear();
        last_hostname.clear();
    }
    void setHostname(const char* hostname) { last_hostname = hostname ? hostname : ""; }
    bool softAP(const char* ssid, const char* passphrase = nullptr, int channel = 1, int ssid_hidden = 0, int max_connection = 4) {
        last_softAP_ssid = ssid ? ssid : "";
        return true;
    }
    bool softAPConfig(IPAddress local_ip, IPAddress gateway, IPAddress subnet) { return true; }
    bool softAPdisconnect(bool wifioff = false) { return true; }
    int32_t RSSI(int i = 0) { (void)i; return -50; }
    int16_t scanNetworks(bool async = false, bool show_hidden = false, bool passive = false, uint32_t max_ms_per_chan = 300, uint8_t channel = 0) { return 0; }
    void scanDelete() {}
    int16_t scanComplete() { return 0; }
    int encryptionType(int i) { (void)i; return WIFI_AUTH_OPEN; }
    int channel() { return 1; }
    int channel(int i) { (void)i; return 1; }
    int getMode() { return WIFI_STA; }
    IPAddress gatewayIP() { return IPAddress(192, 168, 1, 1); }
    IPAddress subnetMask() { return IPAddress(255, 255, 255, 0); }
    std::string BSSIDstr() { return "AA:BB:CC:DD:EE:FF"; }
};
inline WiFiClass WiFi;
