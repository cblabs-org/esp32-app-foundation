#!/usr/bin/env bash
# ESP32 App Foundation (c) 2026 CB Labs
#
# Script: scripts/generate_keys.sh
# Description: Generates an RSA-2048 key pair for OTA firmware signing.
#
#   The private key is AES-256 encrypted with a passphrase — you will be
#   prompted to set one during generation. The passphrase is never stored
#   anywhere; you will need it each time you sign a release.
#
#   The public key is embedded in the firmware for signature verification
#   and is safe to commit to the repository.
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

# --- Generate AES-256-encrypted RSA-2048 private key -------------------------
echo "Generating RSA-2048 private key (AES-256 encrypted)..."
echo "You will be prompted to set a passphrase. Store it securely — you will"
echo "need it every time you sign a release."
echo ""
openssl genrsa -aes256 -out "${PRIVATE_KEY}" 2048

# Lock down file permissions — readable only by owner
chmod 600 "${PRIVATE_KEY}"

# --- Extract public key -------------------------------------------------------
echo ""
echo "Extracting public key..."
openssl rsa -in "${PRIVATE_KEY}" -pubout -out "${PUBLIC_KEY}"

# --- Print instructions -------------------------------------------------------
cat <<EOF

Keys generated:
  private_key.pem — AES-256 encrypted, KEEP SECRET, never commit to git
  public_key.pem  — embed in firmware via include/publicKey.hpp

To embed the public key in firmware:
  xxd -i public_key.pem > include/publicKey.hpp

To sign a release (passphrase will be prompted):
  python3 scripts/deploy_release.py --tag vX.X.X --local --private-key private_key.pem
EOF
