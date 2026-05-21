# Graveyard: Key Manager

## Storing Keys in Plaintext JSON
Initial considerations included saving keys directly to standard configuration JSON files for simplicity. However, storing API keys in plaintext violates secure IoT guidelines, leaving them vulnerable to physical flash readout attacks.
- **Lesson Learned**: Always enforce hardware-derived AES-256 encryption for any credentials written to physical storage.
