#!/bin/bash
# Fix git index to use PascalCase filenames on macOS case-insensitive FS
# Uses git update-index --force-remove + git add with explicit paths
set -e

cd "$(git rev-parse --show-toplevel)"
git config core.ignorecase false

# Map of old->new for all files that need case changes
declare -a RENAMES=(
    "include/interfaces/iConfigurable.hpp:include/interfaces/IConfigurable.hpp"
    "include/interfaces/iDataSource.hpp:include/interfaces/IDataSource.hpp"
    "lib/deviceCrypto/deviceCrypto.cpp:lib/deviceCrypto/DeviceCrypto.cpp"
    "lib/deviceCrypto/deviceCrypto.hpp:lib/deviceCrypto/DeviceCrypto.hpp"
    "lib/githubClient/githubClient.cpp:lib/githubClient/GithubClient.cpp"
    "lib/githubClient/githubClient.hpp:lib/githubClient/GithubClient.hpp"
    "lib/logger/logger.cpp:lib/logger/Logger.cpp"
    "lib/logger/logger.hpp:lib/logger/Logger.hpp"
    "lib/md5Utils/md5Utils.cpp:lib/md5Utils/Md5Utils.cpp"
    "lib/md5Utils/md5Utils.hpp:lib/md5Utils/Md5Utils.hpp"
    "modules/configManager/configManager.cpp:modules/configManager/ConfigManager.cpp"
    "modules/configManager/configManager.hpp:modules/configManager/ConfigManager.hpp"
    "modules/dataManager/dataManager.cpp:modules/dataManager/DataManager.cpp"
    "modules/dataManager/dataManager.hpp:modules/dataManager/DataManager.hpp"
    "modules/keyManager/keyManager.cpp:modules/keyManager/KeyManager.cpp"
    "modules/keyManager/keyManager.hpp:modules/keyManager/KeyManager.hpp"
    "modules/mcpServer/mcpServer.cpp:modules/mcpServer/McpServer.cpp"
    "modules/mcpServer/mcpServer.hpp:modules/mcpServer/McpServer.hpp"
    "modules/mcpServer/mcpToolRegistry.cpp:modules/mcpServer/McpToolRegistry.cpp"
    "modules/mcpServer/mcpToolRegistry.hpp:modules/mcpServer/McpToolRegistry.hpp"
    "modules/mcpServer/mcpBuiltinTools.cpp:modules/mcpServer/McpBuiltinTools.cpp"
    "modules/mcpServer/mcpBuiltinTools.hpp:modules/mcpServer/McpBuiltinTools.hpp"
    "modules/otaUpdateManager/otaUpdateManager.cpp:modules/otaUpdateManager/OtaUpdateManager.cpp"
    "modules/otaUpdateManager/otaUpdateManager.hpp:modules/otaUpdateManager/OtaUpdateManager.hpp"
    "modules/timeManager/timeManager.cpp:modules/timeManager/TimeManager.cpp"
    "modules/timeManager/timeManager.hpp:modules/timeManager/TimeManager.hpp"
    "modules/webServer/webServer.cpp:modules/webServer/WebServerManager.cpp"
    "modules/webServer/webServer.hpp:modules/webServer/WebServerManager.hpp"
    "modules/wifiManager/wifiManager.cpp:modules/wifiManager/WifiManager.cpp"
    "modules/wifiManager/wifiManager.hpp:modules/wifiManager/WifiManager.hpp"
    "modules/u8g2Display/u8g2DisplayModule.cpp:modules/u8g2Display/U8g2DisplayModule.cpp"
    "modules/u8g2Display/u8g2DisplayModule.hpp:modules/u8g2Display/U8g2DisplayModule.hpp"
    "modules/u8g2Display/fonts/fonts.hpp:modules/u8g2Display/fonts/Fonts.hpp"
    "modules/u8g2Display/boards/bootBoard.cpp:modules/u8g2Display/boards/BootBoard.cpp"
    "modules/u8g2Display/boards/bootBoard.hpp:modules/u8g2Display/boards/BootBoard.hpp"
    "modules/u8g2Display/boards/configuringStatusBoard.cpp:modules/u8g2Display/boards/ConfiguringStatusBoard.cpp"
    "modules/u8g2Display/boards/configuringStatusBoard.hpp:modules/u8g2Display/boards/ConfiguringStatusBoard.hpp"
    "modules/u8g2Display/boards/diagnosticsBoard.cpp:modules/u8g2Display/boards/DiagnosticsBoard.cpp"
    "modules/u8g2Display/boards/diagnosticsBoard.hpp:modules/u8g2Display/boards/DiagnosticsBoard.hpp"
    "modules/u8g2Display/boards/errorBoard.cpp:modules/u8g2Display/boards/ErrorBoard.cpp"
    "modules/u8g2Display/boards/errorBoard.hpp:modules/u8g2Display/boards/ErrorBoard.hpp"
    "modules/u8g2Display/boards/messageBoardBase.cpp:modules/u8g2Display/boards/MessageBoardBase.cpp"
    "modules/u8g2Display/boards/messageBoardBase.hpp:modules/u8g2Display/boards/MessageBoardBase.hpp"
    "modules/u8g2Display/boards/otaProgressBoard.cpp:modules/u8g2Display/boards/OtaProgressBoard.cpp"
    "modules/u8g2Display/boards/otaProgressBoard.hpp:modules/u8g2Display/boards/OtaProgressBoard.hpp"
    "modules/u8g2Display/boards/screensaverBoard.cpp:modules/u8g2Display/boards/ScreensaverBoard.cpp"
    "modules/u8g2Display/boards/screensaverBoard.hpp:modules/u8g2Display/boards/ScreensaverBoard.hpp"
    "modules/u8g2Display/boards/wifiWizardBoard.cpp:modules/u8g2Display/boards/WiFiWizardBoard.cpp"
    "modules/u8g2Display/boards/wifiWizardBoard.hpp:modules/u8g2Display/boards/WiFiWizardBoard.hpp"
    "modules/u8g2Display/boards/interfaces/iBoardLayout.hpp:modules/u8g2Display/boards/interfaces/IBoardLayout.hpp"
    "modules/u8g2Display/widgets/iGfxWidget.hpp:modules/u8g2Display/widgets/IGfxWidget.hpp"
    "modules/u8g2Display/widgets/clockWidget.cpp:modules/u8g2Display/widgets/ClockWidget.cpp"
    "modules/u8g2Display/widgets/clockWidget.hpp:modules/u8g2Display/widgets/ClockWidget.hpp"
    "modules/u8g2Display/widgets/drawingPrimitives.cpp:modules/u8g2Display/widgets/DrawingPrimitives.cpp"
    "modules/u8g2Display/widgets/drawingPrimitives.hpp:modules/u8g2Display/widgets/DrawingPrimitives.hpp"
    "modules/u8g2Display/widgets/imageWidget.cpp:modules/u8g2Display/widgets/ImageWidget.cpp"
    "modules/u8g2Display/widgets/imageWidget.hpp:modules/u8g2Display/widgets/ImageWidget.hpp"
    "modules/u8g2Display/widgets/labelWidget.cpp:modules/u8g2Display/widgets/LabelWidget.cpp"
    "modules/u8g2Display/widgets/labelWidget.hpp:modules/u8g2Display/widgets/LabelWidget.hpp"
    "modules/u8g2Display/widgets/progressBarWidget.cpp:modules/u8g2Display/widgets/ProgressBarWidget.cpp"
    "modules/u8g2Display/widgets/progressBarWidget.hpp:modules/u8g2Display/widgets/ProgressBarWidget.hpp"
    "modules/u8g2Display/widgets/scrollingTextWidget.cpp:modules/u8g2Display/widgets/ScrollingTextWidget.cpp"
    "modules/u8g2Display/widgets/scrollingTextWidget.hpp:modules/u8g2Display/widgets/ScrollingTextWidget.hpp"
    "modules/u8g2Display/widgets/scrollingMessagePoolWidget.cpp:modules/u8g2Display/widgets/ScrollingMessagePoolWidget.cpp"
    "modules/u8g2Display/widgets/scrollingMessagePoolWidget.hpp:modules/u8g2Display/widgets/ScrollingMessagePoolWidget.hpp"
    "modules/u8g2Display/widgets/systemMessageWidget.cpp:modules/u8g2Display/widgets/SystemMessageWidget.cpp"
    "modules/u8g2Display/widgets/systemMessageWidget.hpp:modules/u8g2Display/widgets/SystemMessageWidget.hpp"
    "modules/u8g2Display/widgets/wifiStatusWidget.cpp:modules/u8g2Display/widgets/WifiStatusWidget.cpp"
    "modules/u8g2Display/widgets/wifiStatusWidget.hpp:modules/u8g2Display/widgets/WifiStatusWidget.hpp"
)

echo "=== Fixing git index entries ==="
for pair in "${RENAMES[@]}"; do
    old="${pair%%:*}"
    new="${pair##*:}"
    # Remove old entry from index
    git update-index --force-remove "$old" 2>/dev/null || true
    # Compute blob hash from current file content and add with new name
    hash=$(git hash-object -w "$new")
    git update-index --add --cacheinfo 100644,"$hash","$new"
    echo "  ✓ $old → $new"
done

echo ""
echo "=== Verification ==="
echo "deviceCrypto index:"
git ls-files lib/deviceCrypto/
echo "configManager index:"
git ls-files modules/configManager/
echo "widgets index:"
git ls-files modules/u8g2Display/widgets/ | head -5

git config core.ignorecase true
echo ""
echo "=== Done ==="
