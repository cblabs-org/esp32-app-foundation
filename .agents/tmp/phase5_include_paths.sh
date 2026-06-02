#!/bin/bash
# Phase 5: Update all #include paths to reflect new PascalCase directory names
# Also updates Module: header paths
set -e
cd "$(git rev-parse --show-toplevel)"

echo "=== Phase 5: #include Directory Path Updates ==="

# Directory mappings (old → new) for sed substitution
# These only affect directory COMPONENTS in include paths
declare -a DIR_MAPS=(
    "application:Application"
    "configManager:ConfigManager"
    "dataManager:DataManager"
    "eventBus:EventBus"
    "keyManager:KeyManager"
    "mcpServer:MCPServer"
    "otaUpdateManager:OTAUpdateManager"
    "timeManager:TimeManager"
    "u8g2Display:U8G2Display"
    "webServer:WebServer"
    "wifiManager:WifiManager"
    "deviceCrypto:DeviceCrypto"
    "githubClient:GithubClient"
    "logger:Logger"
    "md5Utils:MD5Utils"
    "otaDownloadEngine:OTADownloadEngine"
)

# Find all source files
FILES=$(find modules/ lib/ include/ src/ test/ -name "*.hpp" -o -name "*.cpp" -o -name "*.h" | grep -v portal_assets | grep -v '.pio')

for pair in "${DIR_MAPS[@]}"; do
    old="${pair%%:*}"
    new="${pair##*:}"
    
    # Update #include paths: match directory name followed by /
    # Using BSD sed word boundary for safety
    sed -i '' "s|/${old}/|/${new}/|g" $FILES 2>/dev/null || true
    
    # Update Module: header paths
    sed -i '' "s|modules/${old}/|modules/${new}/|g" $FILES 2>/dev/null || true
    sed -i '' "s|lib/${old}/|lib/${new}/|g" $FILES 2>/dev/null || true
done

echo "=== Verification: any remaining old directory refs in includes? ==="
grep -rn '#include' modules/ lib/ include/ src/ test/ --include="*.hpp" --include="*.cpp" --include="*.h" | grep -E '/application/|/configManager/|/dataManager/|/eventBus/|/keyManager/|/mcpServer/|/otaUpdateManager/|/timeManager/|/u8g2Display/|/webServer/|/wifiManager/|/deviceCrypto/|/githubClient/|/md5Utils/|/otaDownloadEngine/' | grep -v portal_assets | grep -v '/Application/' | grep -v '/ConfigManager/' | grep -v '/DataManager/' | grep -v '/EventBus/' | grep -v '/KeyManager/' | grep -v '/MCPServer/' | grep -v '/OTAUpdateManager/' | grep -v '/TimeManager/' | grep -v '/U8G2Display/' | grep -v '/WebServer/' | grep -v '/WifiManager/' | grep -v '/DeviceCrypto/' | grep -v '/GithubClient/' | grep -v '/Logger/' | grep -v '/MD5Utils/' | grep -v '/OTADownloadEngine/' || echo "✅ All clean"

echo ""
echo "=== Done ==="
