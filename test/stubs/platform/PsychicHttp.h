#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include "Arduino.h"
#include "esp_system.h"

// Define ESP-IDF error types for native build
typedef int esp_err_t;
#ifndef ESP_OK
#define ESP_OK 0
#endif

class PsychicResponse {
public:
    esp_err_t send(int code, const char* contentType, const char* content) { return ESP_OK; }
    esp_err_t send() { return ESP_OK; }
    void setContent(const uint8_t* content, size_t len) {}
    void setContentType(const char* type) {}
    void addHeader(const char* name, const char* value) {}
    esp_err_t redirect(const char* url) { return ESP_OK; }
};

class PsychicWebParameter {
public:
    String value() { return ""; }
};

class PsychicRequest {
public:
    String host() { return String(""); }
    String url() { return String(""); }
    String uri() { return String(""); }
    String header(const char* name) { return String(""); }
    bool hasHeader(const char* name) { return false; }
    std::string body() { return std::string(); }
    size_t contentLength() { return 0; }
    const char* bodyCStr() { return ""; }
    bool hasParam(const char* name) { return false; }
    PsychicWebParameter* getParam(const char* name) { static PsychicWebParameter p; return &p; }
};

typedef std::function<esp_err_t(PsychicRequest *request, PsychicResponse *response)> PsychicHttpRequestCallback;
typedef std::function<esp_err_t()> PsychicMiddlewareNext;
typedef std::function<esp_err_t(PsychicRequest*, PsychicResponse*, PsychicMiddlewareNext)> PsychicMiddlewareCallback;

#define HTTP_GET     0b00000001
#define HTTP_POST    0b00000010
#define HTTP_DELETE  0b00000100
#define HTTP_PUT     0b00001000
#define HTTP_PATCH   0b00010000
#define HTTP_HEAD    0b00100000
#define HTTP_OPTIONS 0b01000000
#define HTTP_ANY     0b01111111

class PsychicEndpoint {};
class PsychicHandler {};

class PsychicHttpServer {
public:
    PsychicHttpServer(uint16_t port = 80) {}
    virtual ~PsychicHttpServer() {}
    esp_err_t begin() { return ESP_OK; }
    void end() {}

    struct Route {
        std::string uri;
        int method;
        PsychicHttpRequestCallback handler;
    };
    std::vector<Route> routes;

    PsychicEndpoint* on(const char* uri, int method, PsychicHttpRequestCallback handler) {
        routes.push_back({uri, method, handler});
        return nullptr;
    }
    PsychicEndpoint* on(const char* uri, int method, PsychicHandler* handler) {
        return nullptr;
    }
    void onNotFound(PsychicHttpRequestCallback handler) {} // Stub for captive portal
    void addMiddleware(PsychicMiddlewareCallback fn) {}    // Stub for global auth middleware
    static void setMaxRequestBodySize(uint32_t size) {
        lastMaxBodySize = size;
    }

    static uint32_t lastMaxBodySize;
};

inline uint32_t PsychicHttpServer::lastMaxBodySize = 0;

class PsychicHttpsServer : public PsychicHttpServer {
public:
    PsychicHttpsServer() : PsychicHttpServer(443) {}
    void setCertificate(const char* cert, const char* private_key) {}
    void setCertificate(const uint8_t* cert, size_t cert_size, const uint8_t* private_key, size_t private_key_size) {}
};
