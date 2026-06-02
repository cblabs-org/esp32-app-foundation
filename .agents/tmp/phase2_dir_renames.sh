#!/bin/bash
# Phase 2: Rename all module/lib directories from camelCase to PascalCase
# Uses 'mv' on filesystem (macOS case-insensitive needs intermediate step)
set -e
cd "$(git rev-parse --show-toplevel)"

echo "=== Phase 2: Directory Renames ==="

# Map: old_dir → new_dir
declare -a DIR_RENAMES=(
    "modules/application:modules/Application"
    "modules/configManager:modules/ConfigManager"
    "modules/dataManager:modules/DataManager"
    "modules/eventBus:modules/EventBus"
    "modules/keyManager:modules/KeyManager"
    "modules/mcpServer:modules/MCPServer"
    "modules/otaUpdateManager:modules/OTAUpdateManager"
    "modules/timeManager:modules/TimeManager"
    "modules/u8g2Display:modules/U8G2Display"
    "modules/webServer:modules/WebServer"
    "modules/wifiManager:modules/WifiManager"
    "lib/deviceCrypto:lib/DeviceCrypto"
    "lib/githubClient:lib/GithubClient"
    "lib/logger:lib/Logger"
    "lib/md5Utils:lib/MD5Utils"
    "lib/otaDownloadEngine:lib/OTADownloadEngine"
)

for pair in "${DIR_RENAMES[@]}"; do
    old="${pair%%:*}"
    new="${pair##*:}"
    if [ -d "$old" ]; then
        # macOS case-insensitive: two-step via temp name
        tmp="${old}_tmp_rename"
        mv "$old" "$tmp"
        mv "$tmp" "$new"
        echo "  ✓ $old → $new"
    else
        echo "  ⚠ $old not found (already renamed?)"
    fi
done

echo ""
echo "=== Verification ==="
echo "modules/:"
ls -d modules/*/
echo ""
echo "lib/:"
ls -d lib/*/
echo ""
echo "=== Done ==="
