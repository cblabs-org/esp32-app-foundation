#pragma once
#include "Arduino.h"
#include "WiFiClient.h"
#include <string>
#include <memory>

#define HTTPC_DISABLE_FOLLOW_REDIRECTS 0
#define HTTPC_STRICT_FOLLOW_REDIRECTS 1
#define HTTP_CODE_MOVED_PERMANENTLY 301
#define HTTP_CODE_FOUND 302
#define HTTP_CODE_TEMPORARY_REDIRECT 307
#define HTTP_CODE_PERMANENT_REDIRECT 308
#define HTTP_CODE_OK 200
#define HTTP_CODE_NOT_FOUND 404
#define HTTP_CODE_FORBIDDEN 403

// Stub for ESP32 TLS sslclient_context (used by WeatherModule via forward decl)
struct sslclient_context {};
inline void attach_ssl_certificate_bundle(sslclient_context* ssl_client, bool att) {}

class HTTPClient {
public:
    bool begin(WiFiClient& client, String url) { return true; }
    bool begin(const String& url) { return true; }
    void useHTTP10(bool use) {}
    void setTimeout(int timeout) {}
    void setFollowRedirects(int follow) {}
    void setUserAgent(const char* userAgent) {}
    void addHeader(const char* name, const char* value) {}
    bool connected() { return false; }
    int GET() { return 200; }
    int POST(const String& payload) { return 200; }
    int send(const String& method, const String& body) { return 200; }
    String getLocation() { return ""; }
    int getSize() { return 0; }
    WiFiClient* getStreamPtr() { static WiFiClient c; return &c; }
    void end() {}
    // Private member stubs accessed by WeatherModule internal calls
    bool _use_ca_bundle = false;
    std::shared_ptr<sslclient_context> sslclient;
};
