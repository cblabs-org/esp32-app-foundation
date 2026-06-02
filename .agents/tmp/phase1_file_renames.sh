#!/bin/bash
# Phase 1: Two-step git mv for all file renames (macOS case-insensitive FS)
# Each rename goes: old → old.tmp → new
set -e

cd "$(git rev-parse --show-toplevel)"

rename_file() {
    local old="$1"
    local new="$2"
    if [ -f "$old" ]; then
        git mv "$old" "${old}.tmp"
        git mv "${old}.tmp" "$new"
        echo "  ✓ $old → $new"
    else
        echo "  ⚠ SKIP (not found): $old"
    fi
}

echo "=== Phase 1: File Renames ==="
echo ""

echo "--- include/interfaces/ (i→I prefix) ---"
rename_file "include/interfaces/iConfigurable.hpp" "include/interfaces/IConfigurable.hpp"
rename_file "include/interfaces/iDataSource.hpp" "include/interfaces/IDataSource.hpp"

echo ""
echo "--- lib/deviceCrypto/ ---"
rename_file "lib/deviceCrypto/deviceCrypto.hpp" "lib/deviceCrypto/DeviceCrypto.hpp"
rename_file "lib/deviceCrypto/deviceCrypto.cpp" "lib/deviceCrypto/DeviceCrypto.cpp"

echo ""
echo "--- lib/githubClient/ ---"
rename_file "lib/githubClient/githubClient.hpp" "lib/githubClient/GithubClient.hpp"
rename_file "lib/githubClient/githubClient.cpp" "lib/githubClient/GithubClient.cpp"

echo ""
echo "--- lib/logger/ ---"
rename_file "lib/logger/logger.hpp" "lib/logger/Logger.hpp"
rename_file "lib/logger/logger.cpp" "lib/logger/Logger.cpp"

echo ""
echo "--- lib/md5Utils/ ---"
rename_file "lib/md5Utils/md5Utils.hpp" "lib/md5Utils/Md5Utils.hpp"
rename_file "lib/md5Utils/md5Utils.cpp" "lib/md5Utils/Md5Utils.cpp"

echo ""
echo "--- modules/configManager/ ---"
rename_file "modules/configManager/configManager.hpp" "modules/configManager/ConfigManager.hpp"
rename_file "modules/configManager/configManager.cpp" "modules/configManager/ConfigManager.cpp"

echo ""
echo "--- modules/dataManager/ ---"
rename_file "modules/dataManager/dataManager.hpp" "modules/dataManager/DataManager.hpp"
rename_file "modules/dataManager/dataManager.cpp" "modules/dataManager/DataManager.cpp"

echo ""
echo "--- modules/keyManager/ ---"
rename_file "modules/keyManager/keyManager.hpp" "modules/keyManager/KeyManager.hpp"
rename_file "modules/keyManager/keyManager.cpp" "modules/keyManager/KeyManager.cpp"

echo ""
echo "--- modules/mcpServer/ ---"
rename_file "modules/mcpServer/mcpServer.hpp" "modules/mcpServer/McpServer.hpp"
rename_file "modules/mcpServer/mcpServer.cpp" "modules/mcpServer/McpServer.cpp"
rename_file "modules/mcpServer/mcpToolRegistry.hpp" "modules/mcpServer/McpToolRegistry.hpp"
rename_file "modules/mcpServer/mcpToolRegistry.cpp" "modules/mcpServer/McpToolRegistry.cpp"
rename_file "modules/mcpServer/mcpBuiltinTools.hpp" "modules/mcpServer/McpBuiltinTools.hpp"
rename_file "modules/mcpServer/mcpBuiltinTools.cpp" "modules/mcpServer/McpBuiltinTools.cpp"

echo ""
echo "--- modules/otaUpdateManager/ ---"
rename_file "modules/otaUpdateManager/otaUpdateManager.hpp" "modules/otaUpdateManager/OtaUpdateManager.hpp"
rename_file "modules/otaUpdateManager/otaUpdateManager.cpp" "modules/otaUpdateManager/OtaUpdateManager.cpp"

echo ""
echo "--- modules/timeManager/ ---"
rename_file "modules/timeManager/timeManager.hpp" "modules/timeManager/TimeManager.hpp"
rename_file "modules/timeManager/timeManager.cpp" "modules/timeManager/TimeManager.cpp"

echo ""
echo "--- modules/webServer/ ---"
rename_file "modules/webServer/webServer.hpp" "modules/webServer/WebServerManager.hpp"
rename_file "modules/webServer/webServer.cpp" "modules/webServer/WebServerManager.cpp"

echo ""
echo "--- modules/wifiManager/ ---"
rename_file "modules/wifiManager/wifiManager.hpp" "modules/wifiManager/WifiManager.hpp"
rename_file "modules/wifiManager/wifiManager.cpp" "modules/wifiManager/WifiManager.cpp"

echo ""
echo "--- modules/u8g2Display/ (root) ---"
rename_file "modules/u8g2Display/u8g2DisplayModule.hpp" "modules/u8g2Display/U8g2DisplayModule.hpp"
rename_file "modules/u8g2Display/u8g2DisplayModule.cpp" "modules/u8g2Display/U8g2DisplayModule.cpp"

echo ""
echo "--- modules/u8g2Display/fonts/ ---"
rename_file "modules/u8g2Display/fonts/fonts.hpp" "modules/u8g2Display/fonts/Fonts.hpp"

echo ""
echo "--- modules/u8g2Display/boards/ ---"
rename_file "modules/u8g2Display/boards/bootBoard.hpp" "modules/u8g2Display/boards/BootBoard.hpp"
rename_file "modules/u8g2Display/boards/bootBoard.cpp" "modules/u8g2Display/boards/BootBoard.cpp"
rename_file "modules/u8g2Display/boards/configuringStatusBoard.hpp" "modules/u8g2Display/boards/ConfiguringStatusBoard.hpp"
rename_file "modules/u8g2Display/boards/configuringStatusBoard.cpp" "modules/u8g2Display/boards/ConfiguringStatusBoard.cpp"
rename_file "modules/u8g2Display/boards/diagnosticsBoard.hpp" "modules/u8g2Display/boards/DiagnosticsBoard.hpp"
rename_file "modules/u8g2Display/boards/diagnosticsBoard.cpp" "modules/u8g2Display/boards/DiagnosticsBoard.cpp"
rename_file "modules/u8g2Display/boards/errorBoard.hpp" "modules/u8g2Display/boards/ErrorBoard.hpp"
rename_file "modules/u8g2Display/boards/errorBoard.cpp" "modules/u8g2Display/boards/ErrorBoard.cpp"
rename_file "modules/u8g2Display/boards/messageBoardBase.hpp" "modules/u8g2Display/boards/MessageBoardBase.hpp"
rename_file "modules/u8g2Display/boards/messageBoardBase.cpp" "modules/u8g2Display/boards/MessageBoardBase.cpp"
rename_file "modules/u8g2Display/boards/otaProgressBoard.hpp" "modules/u8g2Display/boards/OtaProgressBoard.hpp"
rename_file "modules/u8g2Display/boards/otaProgressBoard.cpp" "modules/u8g2Display/boards/OtaProgressBoard.cpp"
rename_file "modules/u8g2Display/boards/screensaverBoard.hpp" "modules/u8g2Display/boards/ScreensaverBoard.hpp"
rename_file "modules/u8g2Display/boards/screensaverBoard.cpp" "modules/u8g2Display/boards/ScreensaverBoard.cpp"
rename_file "modules/u8g2Display/boards/wifiWizardBoard.hpp" "modules/u8g2Display/boards/WiFiWizardBoard.hpp"
rename_file "modules/u8g2Display/boards/wifiWizardBoard.cpp" "modules/u8g2Display/boards/WiFiWizardBoard.cpp"

echo ""
echo "--- modules/u8g2Display/boards/interfaces/ ---"
rename_file "modules/u8g2Display/boards/interfaces/iBoardLayout.hpp" "modules/u8g2Display/boards/interfaces/IBoardLayout.hpp"

echo ""
echo "--- modules/u8g2Display/widgets/ ---"
rename_file "modules/u8g2Display/widgets/iGfxWidget.hpp" "modules/u8g2Display/widgets/IGfxWidget.hpp"
rename_file "modules/u8g2Display/widgets/clockWidget.hpp" "modules/u8g2Display/widgets/ClockWidget.hpp"
rename_file "modules/u8g2Display/widgets/clockWidget.cpp" "modules/u8g2Display/widgets/ClockWidget.cpp"
rename_file "modules/u8g2Display/widgets/drawingPrimitives.hpp" "modules/u8g2Display/widgets/DrawingPrimitives.hpp"
rename_file "modules/u8g2Display/widgets/drawingPrimitives.cpp" "modules/u8g2Display/widgets/DrawingPrimitives.cpp"
rename_file "modules/u8g2Display/widgets/imageWidget.hpp" "modules/u8g2Display/widgets/ImageWidget.hpp"
rename_file "modules/u8g2Display/widgets/imageWidget.cpp" "modules/u8g2Display/widgets/ImageWidget.cpp"
rename_file "modules/u8g2Display/widgets/labelWidget.hpp" "modules/u8g2Display/widgets/LabelWidget.hpp"
rename_file "modules/u8g2Display/widgets/labelWidget.cpp" "modules/u8g2Display/widgets/LabelWidget.cpp"
rename_file "modules/u8g2Display/widgets/progressBarWidget.hpp" "modules/u8g2Display/widgets/ProgressBarWidget.hpp"
rename_file "modules/u8g2Display/widgets/progressBarWidget.cpp" "modules/u8g2Display/widgets/ProgressBarWidget.cpp"
rename_file "modules/u8g2Display/widgets/scrollingTextWidget.hpp" "modules/u8g2Display/widgets/ScrollingTextWidget.hpp"
rename_file "modules/u8g2Display/widgets/scrollingTextWidget.cpp" "modules/u8g2Display/widgets/ScrollingTextWidget.cpp"
rename_file "modules/u8g2Display/widgets/scrollingMessagePoolWidget.hpp" "modules/u8g2Display/widgets/ScrollingMessagePoolWidget.hpp"
rename_file "modules/u8g2Display/widgets/scrollingMessagePoolWidget.cpp" "modules/u8g2Display/widgets/ScrollingMessagePoolWidget.cpp"
rename_file "modules/u8g2Display/widgets/systemMessageWidget.hpp" "modules/u8g2Display/widgets/SystemMessageWidget.hpp"
rename_file "modules/u8g2Display/widgets/systemMessageWidget.cpp" "modules/u8g2Display/widgets/SystemMessageWidget.cpp"
rename_file "modules/u8g2Display/widgets/wifiStatusWidget.hpp" "modules/u8g2Display/widgets/WifiStatusWidget.hpp"
rename_file "modules/u8g2Display/widgets/wifiStatusWidget.cpp" "modules/u8g2Display/widgets/WifiStatusWidget.cpp"

echo ""
echo "=== Phase 1 Complete ==="
echo ""
git status --short | wc -l | xargs -I{} echo "Total files changed: {}"
