# ADR: TLS Certificate SAN Injection & PEM Upload Offloading

## Context
Chrome strictly requires a Subject Alternative Name (SAN) extension on certificates to bypass the ERR_CERT_INVALID block. We needed a way to generate this on the ESP32 and allow custom uploads.

## Decisions

1. **SAN Injection using mbedtls**:
   Rather than relying on newer mbedtls helper functions which may or may not be available across ESP-IDF versions, we explicitly built the `MBEDTLS_OID_SUBJECT_ALT_NAME` extension using `mbedtls_asn1_write_*` functions and `mbedtls_x509write_crt_set_extension`. The ASN.1 sequence successfully encodes both the mDNS hostname (`esp32-device.local`) and the currently active IP Address.

2. **Frontend PEM Stripping**:
   To minimize memory overhead on the ESP32 (heap exhaustion), we avoided passing raw PEM strings to the backend. Instead, `auth.js` runs a regex `replace()` to strip `-----BEGIN...-----` headers, footers, and whitespace. The raw Base64 is then sent to the ESP32, where it is directly decoded into DER format using `mbedtls_base64_decode` and written to LittleFS. This saves hundreds of bytes of RAM during parsing.
