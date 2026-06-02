#!/bin/bash
# Fix all remaining lowercase filenames on the actual filesystem
# Handles all files that were renamed in git index but not on macOS FS
set -e
cd "$(git rev-parse --show-toplevel)"

echo "=== Fixing filesystem file names ==="

# All files that need renaming (old FS name → new PascalCase name)
declare -a FS_RENAMES=(
    # lib/DeviceCrypto
    "lib/DeviceCrypto/deviceCrypto.cpp:lib/DeviceCrypto/DeviceCrypto.cpp"
    "lib/DeviceCrypto/deviceCrypto.hpp:lib/DeviceCrypto/DeviceCrypto.hpp"
    # lib/GithubClient
    "lib/GithubClient/githubClient.cpp:lib/GithubClient/GithubClient.cpp"
    "lib/GithubClient/githubClient.hpp:lib/GithubClient/GithubClient.hpp"
    # lib/Logger
    "lib/Logger/logger.cpp:lib/Logger/Logger.cpp"
    "lib/Logger/logger.hpp:lib/Logger/Logger.hpp"
    # modules/ConfigManager
    "modules/ConfigManager/configManager.cpp:modules/ConfigManager/ConfigManager.cpp"
    "modules/ConfigManager/configManager.hpp:modules/ConfigManager/ConfigManager.hpp"
    # modules/DataManager
    "modules/DataManager/dataManager.cpp:modules/DataManager/DataManager.cpp"
    "modules/DataManager/dataManager.hpp:modules/DataManager/DataManager.hpp"
    # modules/KeyManager
    "modules/KeyManager/keyManager.cpp:modules/KeyManager/KeyManager.cpp"
    "modules/KeyManager/keyManager.hpp:modules/KeyManager/KeyManager.hpp"
    # modules/TimeManager
    "modules/TimeManager/timeManager.cpp:modules/TimeManager/TimeManager.cpp"
    "modules/TimeManager/timeManager.hpp:modules/TimeManager/TimeManager.hpp"
    # modules/WifiManager
    "modules/WifiManager/wifiManager.cpp:modules/WifiManager/WifiManager.cpp"
    "modules/WifiManager/wifiManager.hpp:modules/WifiManager/WifiManager.hpp"
    # modules/U8G2Display boards (still lowercase from original)
    "modules/U8G2Display/boards/bootBoard.cpp:modules/U8G2Display/boards/BootBoard.cpp"
    "modules/U8G2Display/boards/bootBoard.hpp:modules/U8G2Display/boards/BootBoard.hpp"
    "modules/U8G2Display/boards/configuringStatusBoard.cpp:modules/U8G2Display/boards/ConfiguringStatusBoard.cpp"
    "modules/U8G2Display/boards/configuringStatusBoard.hpp:modules/U8G2Display/boards/ConfiguringStatusBoard.hpp"
    "modules/U8G2Display/boards/diagnosticsBoard.cpp:modules/U8G2Display/boards/DiagnosticsBoard.cpp"
    "modules/U8G2Display/boards/diagnosticsBoard.hpp:modules/U8G2Display/boards/DiagnosticsBoard.hpp"
    "modules/U8G2Display/boards/errorBoard.cpp:modules/U8G2Display/boards/ErrorBoard.cpp"
    "modules/U8G2Display/boards/errorBoard.hpp:modules/U8G2Display/boards/ErrorBoard.hpp"
    "modules/U8G2Display/boards/messageBoardBase.cpp:modules/U8G2Display/boards/MessageBoardBase.cpp"
    "modules/U8G2Display/boards/messageBoardBase.hpp:modules/U8G2Display/boards/MessageBoardBase.hpp"
    "modules/U8G2Display/boards/screensaverBoard.cpp:modules/U8G2Display/boards/ScreensaverBoard.cpp"
    "modules/U8G2Display/boards/screensaverBoard.hpp:modules/U8G2Display/boards/ScreensaverBoard.hpp"
    "modules/U8G2Display/boards/wifiWizardBoard.cpp:modules/U8G2Display/boards/WiFiWizardBoard.cpp"
    "modules/U8G2Display/boards/wifiWizardBoard.hpp:modules/U8G2Display/boards/WiFiWizardBoard.hpp"
    "modules/U8G2Display/boards/interfaces/iBoardLayout.hpp:modules/U8G2Display/boards/interfaces/IBoardLayout.hpp"
    "modules/U8G2Display/fonts/fonts.hpp:modules/U8G2Display/fonts/Fonts.hpp"
    # U8G2Display widgets
    "modules/U8G2Display/widgets/clockWidget.cpp:modules/U8G2Display/widgets/ClockWidget.cpp"
    "modules/U8G2Display/widgets/clockWidget.hpp:modules/U8G2Display/widgets/ClockWidget.hpp"
    "modules/U8G2Display/widgets/drawingPrimitives.cpp:modules/U8G2Display/widgets/DrawingPrimitives.cpp"
    "modules/U8G2Display/widgets/drawingPrimitives.hpp:modules/U8G2Display/widgets/DrawingPrimitives.hpp"
    "modules/U8G2Display/widgets/iGfxWidget.hpp:modules/U8G2Display/widgets/IGfxWidget.hpp"
    "modules/U8G2Display/widgets/imageWidget.cpp:modules/U8G2Display/widgets/ImageWidget.cpp"
    "modules/U8G2Display/widgets/imageWidget.hpp:modules/U8G2Display/widgets/ImageWidget.hpp"
    "modules/U8G2Display/widgets/labelWidget.cpp:modules/U8G2Display/widgets/LabelWidget.cpp"
    "modules/U8G2Display/widgets/labelWidget.hpp:modules/U8G2Display/widgets/LabelWidget.hpp"
    "modules/U8G2Display/widgets/progressBarWidget.cpp:modules/U8G2Display/widgets/ProgressBarWidget.cpp"
    "modules/U8G2Display/widgets/progressBarWidget.hpp:modules/U8G2Display/widgets/ProgressBarWidget.hpp"
    "modules/U8G2Display/widgets/scrollingTextWidget.cpp:modules/U8G2Display/widgets/ScrollingTextWidget.cpp"
    "modules/U8G2Display/widgets/scrollingTextWidget.hpp:modules/U8G2Display/widgets/ScrollingTextWidget.hpp"
    "modules/U8G2Display/widgets/scrollingMessagePoolWidget.cpp:modules/U8G2Display/widgets/ScrollingMessagePoolWidget.cpp"
    "modules/U8G2Display/widgets/scrollingMessagePoolWidget.hpp:modules/U8G2Display/widgets/ScrollingMessagePoolWidget.hpp"
    "modules/U8G2Display/widgets/systemMessageWidget.cpp:modules/U8G2Display/widgets/SystemMessageWidget.cpp"
    "modules/U8G2Display/widgets/systemMessageWidget.hpp:modules/U8G2Display/widgets/SystemMessageWidget.hpp"
    "modules/U8G2Display/widgets/wifiStatusWidget.cpp:modules/U8G2Display/widgets/WifiStatusWidget.cpp"
    "modules/U8G2Display/widgets/wifiStatusWidget.hpp:modules/U8G2Display/widgets/WifiStatusWidget.hpp"
)

for pair in "${FS_RENAMES[@]}"; do
    old="${pair%%:*}"
    new="${pair##*:}"
    if [ -f "$old" ]; then
        tmp="${old}.tmpfs"
        mv "$old" "$tmp"
        mv "$tmp" "$new"
        echo "  ✓ $(basename $old) → $(basename $new)"
    fi
done

echo ""
echo "=== Verification ==="
echo "DeviceCrypto:"
ls lib/DeviceCrypto/
echo "Logger:"
ls lib/Logger/
echo "ConfigManager:"
ls modules/ConfigManager/

echo ""
echo "=== Done ==="
