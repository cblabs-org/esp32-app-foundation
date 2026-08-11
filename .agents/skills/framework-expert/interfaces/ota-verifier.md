# IOTAVerifier

Pure abstract interface for OTA firmware cryptographic verification.

## Overview

To prevent malicious firmware updates, the framework requires incoming OTA binaries to be cryptographically signed. This interface abstracts the verification process, allowing downstream apps to inject different cryptographic suites (or disable it entirely during dev).

```cpp
class IOTAVerifier {
public:
    virtual ~IOTAVerifier() = default;
    virtual bool begin(const char* publicKeyPem) = 0;
    virtual void update(const uint8_t* data, size_t length) = 0;
    virtual bool verify(const uint8_t* signature, size_t signatureLength) = 0;
};
```

## Streaming Crypto (ADR-13)

Firmware binaries are large (~2-4MB), far exceeding the ESP32's available RAM. The verifier MUST use a streaming hash accumulation approach.

1. **`begin(publicKeyPem)`**: Injects the public key (often sourced from `FrameworkConfig` at compile time) and initializes the hashing context.
2. **`update(data, length)`**: Called repeatedly as chunks arrive over the network. Accumulates the SHA-256 hash incrementally. Memory usage must remain constant (`O(1)`).
3. **`verify(signature, signatureLength)`**: Finalizes the digest and performs RSA/ECDSA verification against the detached signature.

## Implementation Considerations

The default implementation uses MbedTLS for SHA-256 and RSA verification. Like `IDataSource`, this operates during the OTA task, meaning large cryptographic contexts should be placed on the FreeRTOS task stack rather than the heap, adhering to ADR-14 principles.
