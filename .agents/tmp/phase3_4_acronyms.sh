#!/bin/bash
# Phase 3+4: Complete acronym CAPS rename (Option B)
# Handles: Ota→OTA, Mcp→MCP, Md5→MD5, U8g2→U8G2
# Includes: files, types, includes, log tags, test classes
set -e
cd "$(git rev-parse --show-toplevel)"

echo "=== Phase 3: File Renames ==="

# File renames (macOS case-insensitive: two-step)
declare -a FILE_RENAMES=(
    # MCP
    "modules/MCPServer/McpServer.cpp:modules/MCPServer/MCPServer.cpp"
    "modules/MCPServer/McpServer.hpp:modules/MCPServer/MCPServer.hpp"
    "modules/MCPServer/McpToolRegistry.cpp:modules/MCPServer/MCPToolRegistry.cpp"
    "modules/MCPServer/McpToolRegistry.hpp:modules/MCPServer/MCPToolRegistry.hpp"
    "modules/MCPServer/McpBuiltinTools.cpp:modules/MCPServer/MCPBuiltinTools.cpp"
    "modules/MCPServer/McpBuiltinTools.hpp:modules/MCPServer/MCPBuiltinTools.hpp"
    # OTA module
    "modules/OTAUpdateManager/OtaUpdateManager.cpp:modules/OTAUpdateManager/OTAUpdateManager.cpp"
    "modules/OTAUpdateManager/OtaUpdateManager.hpp:modules/OTAUpdateManager/OTAUpdateManager.hpp"
    "modules/U8G2Display/boards/OtaProgressBoard.cpp:modules/U8G2Display/boards/OTAProgressBoard.cpp"
    "modules/U8G2Display/boards/OtaProgressBoard.hpp:modules/U8G2Display/boards/OTAProgressBoard.hpp"
    # OTA lib
    "lib/OTADownloadEngine/OtaDownloadEngine.cpp:lib/OTADownloadEngine/OTADownloadEngine.cpp"
    "lib/OTADownloadEngine/OtaDownloadEngine.hpp:lib/OTADownloadEngine/OTADownloadEngine.hpp"
    "lib/OTADownloadEngine/OtaError.cpp:lib/OTADownloadEngine/OTAError.cpp"
    "lib/OTADownloadEngine/OtaError.hpp:lib/OTADownloadEngine/OTAError.hpp"
    # MD5
    "lib/MD5Utils/Md5Utils.cpp:lib/MD5Utils/MD5Utils.cpp"
    "lib/MD5Utils/Md5Utils.hpp:lib/MD5Utils/MD5Utils.hpp"
    # U8G2
    "modules/U8G2Display/U8g2DisplayModule.cpp:modules/U8G2Display/U8G2DisplayModule.cpp"
    "modules/U8G2Display/U8g2DisplayModule.hpp:modules/U8G2Display/U8G2DisplayModule.hpp"
    # Interfaces
    "include/interfaces/IOtaVerifier.hpp:include/interfaces/IOTAVerifier.hpp"
    "include/interfaces/IMcpContributor.hpp:include/interfaces/IMCPContributor.hpp"
)

for pair in "${FILE_RENAMES[@]}"; do
    old="${pair%%:*}"
    new="${pair##*:}"
    if [ -f "$old" ]; then
        tmp="${old}.tmp_rename"
        mv "$old" "$tmp"
        mv "$tmp" "$new"
        echo "  ✓ $(basename $old) → $(basename $new)"
    else
        echo "  ⚠ $old not found"
    fi
done

echo ""
echo "=== Phase 4: Type + Include + Tag Renames ==="

FILES=$(find modules/ lib/ include/ src/ test/ \( -name "*.hpp" -o -name "*.cpp" -o -name "*.h" \) | grep -v portal_assets | grep -v '.pio')

# --- MCP types ---
echo "  MCP types..."
sed -i '' 's/[[:<:]]McpServer[[:>:]]/MCPServer/g' $FILES 2>/dev/null
sed -i '' 's/[[:<:]]McpToolRegistry[[:>:]]/MCPToolRegistry/g' $FILES 2>/dev/null
sed -i '' 's/[[:<:]]McpToolEntry[[:>:]]/MCPToolEntry/g' $FILES 2>/dev/null
sed -i '' 's/[[:<:]]McpBuiltinTools[[:>:]]/MCPBuiltinTools/g' $FILES 2>/dev/null
sed -i '' 's/[[:<:]]IMcpContributor[[:>:]]/IMCPContributor/g' $FILES 2>/dev/null

# --- OTA types ---
echo "  OTA types..."
sed -i '' 's/[[:<:]]OtaUpdateManager[[:>:]]/OTAUpdateManager/g' $FILES 2>/dev/null
sed -i '' 's/[[:<:]]OtaState[[:>:]]/OTAState/g' $FILES 2>/dev/null
sed -i '' 's/[[:<:]]OtaProgressEvent[[:>:]]/OTAProgressEvent/g' $FILES 2>/dev/null
sed -i '' 's/[[:<:]]OtaProgressBoard[[:>:]]/OTAProgressBoard/g' $FILES 2>/dev/null
sed -i '' 's/[[:<:]]OtaProgressPayload[[:>:]]/OTAProgressPayload/g' $FILES 2>/dev/null
sed -i '' 's/[[:<:]]OtaDownloadEngine[[:>:]]/OTADownloadEngine/g' $FILES 2>/dev/null
sed -i '' 's/[[:<:]]OtaError[[:>:]]/OTAError/g' $FILES 2>/dev/null
sed -i '' 's/[[:<:]]IOtaVerifier[[:>:]]/IOTAVerifier/g' $FILES 2>/dev/null
sed -i '' 's/[[:<:]]OtaProgressCallback[[:>:]]/OTAProgressCallback/g' $FILES 2>/dev/null

# --- MD5 types ---
echo "  MD5 types..."
sed -i '' 's/[[:<:]]Md5Utils[[:>:]]/MD5Utils/g' $FILES 2>/dev/null

# --- U8G2 types ---
echo "  U8G2 types..."
sed -i '' 's/[[:<:]]U8g2DisplayModule[[:>:]]/U8G2DisplayModule/g' $FILES 2>/dev/null
sed -i '' 's/[[:<:]]U8g2StateSaver[[:>:]]/U8G2StateSaver/g' $FILES 2>/dev/null

# --- Include filenames ---
echo "  Include filenames..."
sed -i '' 's/McpServer\.hpp/MCPServer.hpp/g' $FILES 2>/dev/null
sed -i '' 's/McpServer\.cpp/MCPServer.cpp/g' $FILES 2>/dev/null
sed -i '' 's/McpToolRegistry\.hpp/MCPToolRegistry.hpp/g' $FILES 2>/dev/null
sed -i '' 's/McpToolRegistry\.cpp/MCPToolRegistry.cpp/g' $FILES 2>/dev/null
sed -i '' 's/McpBuiltinTools\.hpp/MCPBuiltinTools.hpp/g' $FILES 2>/dev/null
sed -i '' 's/McpBuiltinTools\.cpp/MCPBuiltinTools.cpp/g' $FILES 2>/dev/null
sed -i '' 's/OtaUpdateManager\.hpp/OTAUpdateManager.hpp/g' $FILES 2>/dev/null
sed -i '' 's/OtaUpdateManager\.cpp/OTAUpdateManager.cpp/g' $FILES 2>/dev/null
sed -i '' 's/OtaProgressBoard\.hpp/OTAProgressBoard.hpp/g' $FILES 2>/dev/null
sed -i '' 's/OtaProgressBoard\.cpp/OTAProgressBoard.cpp/g' $FILES 2>/dev/null
sed -i '' 's/OtaDownloadEngine\.hpp/OTADownloadEngine.hpp/g' $FILES 2>/dev/null
sed -i '' 's/OtaDownloadEngine\.cpp/OTADownloadEngine.cpp/g' $FILES 2>/dev/null
sed -i '' 's/OtaError\.hpp/OTAError.hpp/g' $FILES 2>/dev/null
sed -i '' 's/OtaError\.cpp/OTAError.cpp/g' $FILES 2>/dev/null
sed -i '' 's/Md5Utils\.hpp/MD5Utils.hpp/g' $FILES 2>/dev/null
sed -i '' 's/Md5Utils\.cpp/MD5Utils.cpp/g' $FILES 2>/dev/null
sed -i '' 's/U8g2DisplayModule\.hpp/U8G2DisplayModule.hpp/g' $FILES 2>/dev/null
sed -i '' 's/U8g2DisplayModule\.cpp/U8G2DisplayModule.cpp/g' $FILES 2>/dev/null
sed -i '' 's/IOtaVerifier\.hpp/IOTAVerifier.hpp/g' $FILES 2>/dev/null
sed -i '' 's/IMcpContributor\.hpp/IMCPContributor.hpp/g' $FILES 2>/dev/null

# --- Log TAG string literals ---
echo "  Log TAG strings..."
sed -i '' 's/"OtaVerifier"/"OTAVerifier"/g' $FILES 2>/dev/null
sed -i '' 's/"OtaEngine"/"OTAEngine"/g' $FILES 2>/dev/null
sed -i '' 's/"OtaUpdateManager"/"OTAUpdateManager"/g' $FILES 2>/dev/null
sed -i '' 's/"McpBuiltinTools"/"MCPBuiltinTools"/g' $FILES 2>/dev/null
sed -i '' 's/"McpServer"/"MCPServer"/g' $FILES 2>/dev/null
sed -i '' 's/"Md5Utils"/"MD5Utils"/g' $FILES 2>/dev/null
sed -i '' 's/"U8g2Display"/"U8G2Display"/g' $FILES 2>/dev/null

echo ""
echo "=== Done ==="
