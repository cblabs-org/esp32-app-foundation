# PR-24 Web Portal + MCP Server Authentication + TLS

## Strategic Reasoning & Decisions

### 1. PsychicHttp Migration
We replaced `ESPAsyncWebServer` with `PsychicHttp` because ESPAsyncWebServer lacks robust, native TLS support for the ESP32-S3. PsychicHttp provides native HTTPS using mbedTLS.

### 2. Dual-Server TLS Model
- If a password is set, we boot `PsychicHttpServer` on port 443 (HTTPS) and a secondary redirect server on port 80.
- If no password is set, or the device is in AP Mode (captive portal), we boot a standard HTTP server on port 80 with no auth. This preserves backward compatibility and ensures captive portals don't break due to certificate errors.

### 3. Authentication Mechanisms
- **Portal UI:** Uses session cookies (`esp32_session`) generated via SHA-256. 
- **MCP Server:** Uses a static Bearer Token (derived from the password) via the `Authorization: Bearer <token>` header.

### 4. Self-Signed Certificate Generation
Certificates (EC P-256) are generated on-device and stored in LittleFS (`/cert.der`, `/key.der`). We explicitly added the Subject Alternative Name (SAN) extension (v3) to include the device's IP address and `esp32-device.local` to prevent `ERR_CERT_INVALID` in modern Chrome.

### 5. API Route Whitelisting
To prevent captive portal and bootstrap lockouts, we explicitly whitelisted `/api/auth/status`, `/api/auth/login`, `/api/auth/password`, and exact-matched `/api/system` in the `WebServerManager` global middleware.
