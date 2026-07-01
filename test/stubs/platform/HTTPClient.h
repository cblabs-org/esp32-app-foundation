#pragma once
#include "Arduino.h"
#include "WiFiClient.h"
#include <string>

#define HTTPC_DISABLE_FOLLOW_REDIRECTS 0
#define HTTPC_STRICT_FOLLOW_REDIRECTS 1
#define HTTP_CODE_MOVED_PERMANENTLY 301
#define HTTP_CODE_FOUND 302
#define HTTP_CODE_TEMPORARY_REDIRECT 307
#define HTTP_CODE_PERMANENT_REDIRECT 308
#define HTTP_CODE_OK 200
#define HTTP_CODE_NOT_FOUND 404
#define HTTP_CODE_FORBIDDEN 403

class HTTPClient {
public:
    bool begin(WiFiClient& client, String url) { return true; }
    void useHTTP10(bool use) {}
    void setTimeout(int timeout) {}
    void setFollowRedirects(int follow) {}
    void setUserAgent(const char* userAgent) {}
    void addHeader(const char* name, const char* value) {}
    
    int GET() { return 200; }
    String getLocation() { return ""; }
    int getSize() { return 1024; }
    WiFiClient* getStreamPtr() { static WiFiClient c; return &c; }
    void end() {}
};
