#!/usr/bin/env bash
# ESP32 App Foundation (c) 2026 CB Labs
#
# Script: scripts/generate_keys.sh
# Description: Generates an RSA-2048 key pair for OTA firmware signing.
#              The private key MUST be kept secret and never committed to git.
#              The public key is embedded in the firmware for signature
#              verification.
#
# Usage:
#   chmod +x scripts/generate_keys.sh
#   ./scripts/generate_keys.sh
#
# Prerequisites: openssl

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

PRIVATE_KEY="${PROJECT_ROOT}/private_key.pem"
PUBLIC_KEY="${PROJECT_ROOT}/public_key.pem"

# --- Guard: refuse to overwrite an existing private key ----------------------
if [[ -f "${PRIVATE_KEY}" ]]; then
    echo "ERROR: ${PRIVATE_KEY} already exists."
    echo "       Remove it manually if you want to regenerate the key pair."
    exit 1
fi

# --- Generate RSA-2048 key pair ----------------------------------------------
echo "Generating RSA-2048 private key..."
openssl genrsa -out "${PRIVATE_KEY}" 2048

echo "Extracting public key..."
openssl rsa -in "${PRIVATE_KEY}" -pubout -out "${PUBLIC_KEY}"

# --- Print instructions ------------------------------------------------------
cat <<EOF

Keys generated:
  private_key.pem — KEEP SECRET, never commit to git
  public_key.pem  — Embed in firmware via publicKey.hpp

To convert the public key for embedding:
  xxd -i public_key.pem > include/publicKey.hpp
EOF
