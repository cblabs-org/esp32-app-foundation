/**
 * JsonStreamingParser.h — Native test stub
 *
 * Provides a no-op implementation of the JsonStreamingParser and JsonListener
 * interfaces for compilation in native (host) test environments.
 * Uses Arduino String type to match the squix78/JsonStreamingParser API.
 */

#pragma once
#include "Arduino.h"

class JsonListener {
public:
    virtual ~JsonListener() = default;
    virtual void whitespace(char c) {}
    virtual void startDocument() {}
    virtual void key(String key) {}
    virtual void value(String value) {}
    virtual void endArray() {}
    virtual void endObject() {}
    virtual void endDocument() {}
    virtual void startArray() {}
    virtual void startObject() {}
};

class JsonStreamingParser {
public:
    void setListener(JsonListener* listener) {}
    void parse(char c) {}
    void reset() {}
};
