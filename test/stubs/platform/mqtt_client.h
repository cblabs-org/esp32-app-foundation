#pragma once
#include <cstdint>

#ifndef ESP_OK
#define ESP_OK 0
#endif
typedef int esp_err_t;

typedef enum {
    MQTT_EVENT_ANY = -1,
    MQTT_EVENT_ERROR = 0,
    MQTT_EVENT_CONNECTED,
    MQTT_EVENT_DISCONNECTED,
    MQTT_EVENT_SUBSCRIBED,
    MQTT_EVENT_UNSUBSCRIBED,
    MQTT_EVENT_PUBLISHED,
    MQTT_EVENT_DATA,
    MQTT_EVENT_BEFORE_CONNECT,
    MQTT_EVENT_DELETED,
} esp_mqtt_event_id_t;

typedef struct {
    esp_mqtt_event_id_t event_id;
    void* client;
    void* user_context;
    char* data;
    int data_len;
    int total_data_len;
    int current_data_offset;
    char* topic;
    int topic_len;
    int msg_id;
    int retain;
    int qos;
    bool dup;
    int session_present;
    struct {
        int error_type;
        int connect_return_code;
    }* error_handle;
} esp_mqtt_event_t;

typedef esp_mqtt_event_t* esp_mqtt_event_handle_t;

typedef struct {
    struct {
        struct {
            const char* uri;
        } address;
        struct {
            const char* certificate;
        } verification;
    } broker;
    struct {
        const char* username;
        struct {
            const char* password;
        } authentication;
    } credentials;
    struct {
        int disable_auto_reconnect;
        int timeout_ms;
    } network;
    struct {
        struct {
            const char* topic;
            const char* msg;
            int qos;
            int retain;
        } last_will;
    } session;
} esp_mqtt_client_config_t;

typedef void* esp_mqtt_client_handle_t;

inline esp_mqtt_client_handle_t esp_mqtt_client_init(const esp_mqtt_client_config_t* config) { return (esp_mqtt_client_handle_t)1; }
inline esp_err_t esp_mqtt_client_start(esp_mqtt_client_handle_t client) { return ESP_OK; }
inline esp_err_t esp_mqtt_client_stop(esp_mqtt_client_handle_t client) { return ESP_OK; }
inline esp_err_t esp_mqtt_client_destroy(esp_mqtt_client_handle_t client) { return ESP_OK; }
inline int esp_mqtt_client_publish(esp_mqtt_client_handle_t client, const char* topic, const char* data, int len, int qos, int retain) { return 0; }
inline int esp_mqtt_client_subscribe(esp_mqtt_client_handle_t client, const char* topic, int qos) { return 0; }

// Dummy definition for esp_event_base_t if not defined elsewhere
#ifndef ESP_EVENT_BASE_T_DEFINED
#define ESP_EVENT_BASE_T_DEFINED
typedef const char* esp_event_base_t;
#endif

inline esp_err_t esp_mqtt_client_register_event(esp_mqtt_client_handle_t client, esp_mqtt_event_id_t event, void (*handler)(void*, esp_event_base_t, int32_t, void*), void* event_handler_arg) { return ESP_OK; }
