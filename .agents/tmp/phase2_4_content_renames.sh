#!/bin/bash
# Phases 2-4: Content replacements (class names, #include paths, enum values)
set -e

cd "$(git rev-parse --show-toplevel)"

# Helper: run sed in-place on all C++ source files (excludes portal_assets, test, template)
sed_all() {
    find modules lib include src -type f \( -name "*.hpp" -o -name "*.cpp" -o -name "*.h" \) \
        ! -path "*/portal_assets/*" \
        -exec sed -i '' "$1" {} +
}

# Also fix test files that reference renamed types/includes
sed_tests() {
    find test -type f \( -name "*.hpp" -o -name "*.cpp" -o -name "*.h" \) \
        -exec sed -i '' "$1" {} +
}

sed_all_and_tests() {
    sed_all "$1"
    if [ -d "test" ]; then
        sed_tests "$1"
    fi
}

echo "=== Phase 2: Class/Interface Renames ==="

# Interface prefix renames (i→I)
echo "  Renaming iConfigurable → IConfigurable..."
sed_all_and_tests 's/\biConfigurable\b/IConfigurable/g'

echo "  Renaming iDataSource → IDataSource..."
sed_all_and_tests 's/\biDataSource\b/IDataSource/g'

echo "  Renaming iGfxWidget → IGfxWidget..."
sed_all_and_tests 's/\biGfxWidget\b/IGfxWidget/g'

echo "  Renaming iBoardLayout → IBoardLayout..."
sed_all_and_tests 's/\biBoardLayout\b/IBoardLayout/g'

# Class name renames (camelCase → PascalCase)
echo "  Renaming otaUpdateManager → OtaUpdateManager..."
sed_all_and_tests 's/\botaUpdateManager\b/OtaUpdateManager/g'

echo "  Renaming clockWidget → ClockWidget..."
sed_all_and_tests 's/\bclockWidget\b/ClockWidget/g'

echo "  Renaming imageWidget → ImageWidget..."
sed_all_and_tests 's/\bimageWidget\b/ImageWidget/g'

echo "  Renaming labelWidget → LabelWidget..."
sed_all_and_tests 's/\blabelWidget\b/LabelWidget/g'

echo "  Renaming progressBarWidget → ProgressBarWidget..."
sed_all_and_tests 's/\bprogressBarWidget\b/ProgressBarWidget/g'

echo "  Renaming scrollingTextWidget → ScrollingTextWidget..."
sed_all_and_tests 's/\bscrollingTextWidget\b/ScrollingTextWidget/g'

echo "  Renaming scrollingMessagePoolWidget → ScrollingMessagePoolWidget..."
sed_all_and_tests 's/\bscrollingMessagePoolWidget\b/ScrollingMessagePoolWidget/g'

echo "  Renaming systemMessageWidget → SystemMessageWidget..."
sed_all_and_tests 's/\bsystemMessageWidget\b/SystemMessageWidget/g'

echo "  Renaming wifiStatusWidget → WifiStatusWidget..."
sed_all_and_tests 's/\bwifiStatusWidget\b/WifiStatusWidget/g'

echo ""
echo "=== Phase 3: #include Path Updates ==="

# Interface includes
echo "  Updating iConfigurable.hpp → IConfigurable.hpp..."
sed_all_and_tests 's|iConfigurable\.hpp|IConfigurable.hpp|g'

echo "  Updating iDataSource.hpp → IDataSource.hpp..."
sed_all_and_tests 's|iDataSource\.hpp|IDataSource.hpp|g'

echo "  Updating iGfxWidget.hpp → IGfxWidget.hpp..."
sed_all_and_tests 's|iGfxWidget\.hpp|IGfxWidget.hpp|g'

echo "  Updating iBoardLayout.hpp → IBoardLayout.hpp..."
sed_all_and_tests 's|iBoardLayout\.hpp|IBoardLayout.hpp|g'

# lib includes
echo "  Updating logger.hpp → Logger.hpp..."
sed_all_and_tests 's|logger\.hpp|Logger.hpp|g'

echo "  Updating deviceCrypto.hpp → DeviceCrypto.hpp..."
sed_all_and_tests 's|deviceCrypto\.hpp|DeviceCrypto.hpp|g'

echo "  Updating githubClient.hpp → GithubClient.hpp..."
sed_all_and_tests 's|githubClient\.hpp|GithubClient.hpp|g'

echo "  Updating md5Utils.hpp → Md5Utils.hpp..."
sed_all_and_tests 's|md5Utils\.hpp|Md5Utils.hpp|g'

# module includes
echo "  Updating configManager.hpp → ConfigManager.hpp..."
sed_all_and_tests 's|configManager\.hpp|ConfigManager.hpp|g'

echo "  Updating dataManager.hpp → DataManager.hpp..."
sed_all_and_tests 's|dataManager\.hpp|DataManager.hpp|g'

echo "  Updating keyManager.hpp → KeyManager.hpp..."
sed_all_and_tests 's|keyManager\.hpp|KeyManager.hpp|g'

echo "  Updating mcpServer.hpp → McpServer.hpp..."
sed_all_and_tests 's|mcpServer\.hpp|McpServer.hpp|g'

echo "  Updating mcpToolRegistry.hpp → McpToolRegistry.hpp..."
sed_all_and_tests 's|mcpToolRegistry\.hpp|McpToolRegistry.hpp|g'

echo "  Updating mcpBuiltinTools.hpp → McpBuiltinTools.hpp..."
sed_all_and_tests 's|mcpBuiltinTools\.hpp|McpBuiltinTools.hpp|g'

echo "  Updating otaUpdateManager.hpp → OtaUpdateManager.hpp..."
sed_all_and_tests 's|otaUpdateManager\.hpp|OtaUpdateManager.hpp|g'

echo "  Updating timeManager.hpp → TimeManager.hpp..."
sed_all_and_tests 's|timeManager\.hpp|TimeManager.hpp|g'

echo "  Updating webServer.hpp → WebServerManager.hpp..."
sed_all_and_tests 's|webServer\.hpp|WebServerManager.hpp|g'

echo "  Updating wifiManager.hpp → WifiManager.hpp..."
sed_all_and_tests 's|wifiManager\.hpp|WifiManager.hpp|g'

echo "  Updating u8g2DisplayModule.hpp → U8g2DisplayModule.hpp..."
sed_all_and_tests 's|u8g2DisplayModule\.hpp|U8g2DisplayModule.hpp|g'

# board includes
echo "  Updating board includes..."
sed_all_and_tests 's|bootBoard\.hpp|BootBoard.hpp|g'
sed_all_and_tests 's|configuringStatusBoard\.hpp|ConfiguringStatusBoard.hpp|g'
sed_all_and_tests 's|diagnosticsBoard\.hpp|DiagnosticsBoard.hpp|g'
sed_all_and_tests 's|errorBoard\.hpp|ErrorBoard.hpp|g'
sed_all_and_tests 's|messageBoardBase\.hpp|MessageBoardBase.hpp|g'
sed_all_and_tests 's|otaProgressBoard\.hpp|OtaProgressBoard.hpp|g'
sed_all_and_tests 's|screensaverBoard\.hpp|ScreensaverBoard.hpp|g'
sed_all_and_tests 's|wifiWizardBoard\.hpp|WiFiWizardBoard.hpp|g'

# widget includes
echo "  Updating widget includes..."
sed_all_and_tests 's|clockWidget\.hpp|ClockWidget.hpp|g'
sed_all_and_tests 's|drawingPrimitives\.hpp|DrawingPrimitives.hpp|g'
sed_all_and_tests 's|imageWidget\.hpp|ImageWidget.hpp|g'
sed_all_and_tests 's|labelWidget\.hpp|LabelWidget.hpp|g'
sed_all_and_tests 's|progressBarWidget\.hpp|ProgressBarWidget.hpp|g'
sed_all_and_tests 's|scrollingTextWidget\.hpp|ScrollingTextWidget.hpp|g'
sed_all_and_tests 's|scrollingMessagePoolWidget\.hpp|ScrollingMessagePoolWidget.hpp|g'
sed_all_and_tests 's|systemMessageWidget\.hpp|SystemMessageWidget.hpp|g'
sed_all_and_tests 's|wifiStatusWidget\.hpp|WifiStatusWidget.hpp|g'

# font include
echo "  Updating fonts.hpp → Fonts.hpp..."
sed_all_and_tests 's|fonts/fonts\.hpp|fonts/Fonts.hpp|g'

echo ""
echo "=== Phase 4: Enum Value Renames ==="

# AppState: BOOT, CONFIGURING, RUNNING, UPDATING, ERROR → PascalCase
echo "  Updating AppState enum values..."
sed_all_and_tests 's/AppState::BOOT\b/AppState::Boot/g'
sed_all_and_tests 's/AppState::CONFIGURING\b/AppState::Configuring/g'
sed_all_and_tests 's/AppState::RUNNING\b/AppState::Running/g'
sed_all_and_tests 's/AppState::UPDATING\b/AppState::Updating/g'
sed_all_and_tests 's/AppState::ERROR\b/AppState::Error/g'

# TimeSyncStatus: UNSYNCED, IN_PROGRESS, SYNCED → PascalCase
echo "  Updating TimeSyncStatus enum values..."
sed_all_and_tests 's/TimeSyncStatus::UNSYNCED\b/TimeSyncStatus::Unsynced/g'
sed_all_and_tests 's/TimeSyncStatus::IN_PROGRESS\b/TimeSyncStatus::InProgress/g'
sed_all_and_tests 's/TimeSyncStatus::SYNCED\b/TimeSyncStatus::Synced/g'

# UpdateStatus enum values
echo "  Updating UpdateStatus enum values..."
sed_all_and_tests 's/UpdateStatus::SUCCESS\b/UpdateStatus::Success/g'
sed_all_and_tests 's/UpdateStatus::NO_CHANGE\b/UpdateStatus::NoChange/g'
sed_all_and_tests 's/UpdateStatus::RATE_LIMITED\b/UpdateStatus::RateLimited/g'
sed_all_and_tests 's/UpdateStatus::FETCH_ERROR\b/UpdateStatus::FetchError/g'
sed_all_and_tests 's/UpdateStatus::PARSE_ERROR\b/UpdateStatus::ParseError/g'
sed_all_and_tests 's/UpdateStatus::AUTH_ERROR\b/UpdateStatus::AuthError/g'

# PriorityTier enum values
echo "  Updating PriorityTier enum values..."
sed_all_and_tests 's/PriorityTier::PRIORITY_HIGH\b/PriorityTier::High/g'
sed_all_and_tests 's/PriorityTier::PRIORITY_NORMAL\b/PriorityTier::Normal/g'
sed_all_and_tests 's/PriorityTier::PRIORITY_LOW\b/PriorityTier::Low/g'

# TextAlign enum values
echo "  Updating TextAlign enum values..."
sed_all_and_tests 's/TextAlign::LEFT\b/TextAlign::Left/g'
sed_all_and_tests 's/TextAlign::CENTER\b/TextAlign::Center/g'
sed_all_and_tests 's/TextAlign::RIGHT\b/TextAlign::Right/g'

# Nested enums: ClockFormat
echo "  Updating ClockFormat enum values..."
sed_all_and_tests 's/ClockFormat::HH_MM_SS\b/ClockFormat::HhMmSs/g'
sed_all_and_tests 's/ClockFormat::HH_MM\b/ClockFormat::HhMm/g'

# Nested enums: Phase (BootBoard)
echo "  Updating Phase enum values..."
sed_all_and_tests 's/Phase::SPLASH\b/Phase::Splash/g'
sed_all_and_tests 's/Phase::LOADING\b/Phase::Loading/g'

# Nested enums: WiFiState
echo "  Updating WiFiState enum values..."
sed_all_and_tests 's/WiFiState::WIFI_INIT\b/WiFiState::Init/g'
sed_all_and_tests 's/WiFiState::WIFI_STA_CONNECTING\b/WiFiState::StaConnecting/g'
sed_all_and_tests 's/WiFiState::WIFI_AP_STARTING\b/WiFiState::ApStarting/g'
sed_all_and_tests 's/WiFiState::WIFI_READY\b/WiFiState::Ready/g'
sed_all_and_tests 's/WiFiState::WIFI_SHUTDOWN\b/WiFiState::Shutdown/g'

echo ""
echo "=== Now fix enum declarations themselves ==="

# Fix the actual enum declarations (the values without the EnumName:: prefix)
# AppState
sed -i '' 's/^    BOOT,/    Boot,/' include/AppState.hpp
sed -i '' 's/^    CONFIGURING,/    Configuring,/' include/AppState.hpp
sed -i '' 's/^    RUNNING,/    Running,/' include/AppState.hpp
sed -i '' 's/^    UPDATING,/    Updating,/' include/AppState.hpp
sed -i '' 's/^    ERROR/    Error/' include/AppState.hpp

# TimeSyncStatus
sed -i '' 's/UNSYNCED/Unsynced/g; s/IN_PROGRESS/InProgress/g; s/SYNCED/Synced/g' modules/timeManager/TimeManager.hpp

# UpdateStatus in IDataSource.hpp
sed -i '' 's/^    SUCCESS,/    Success,/' include/interfaces/IDataSource.hpp
sed -i '' 's/^    NO_CHANGE,/    NoChange,/' include/interfaces/IDataSource.hpp
sed -i '' 's/^    RATE_LIMITED,/    RateLimited,/' include/interfaces/IDataSource.hpp
sed -i '' 's/^    FETCH_ERROR,/    FetchError,/' include/interfaces/IDataSource.hpp
sed -i '' 's/^    PARSE_ERROR,/    ParseError,/' include/interfaces/IDataSource.hpp
sed -i '' 's/^    AUTH_ERROR/    AuthError/' include/interfaces/IDataSource.hpp

# PriorityTier in IDataSource.hpp
sed -i '' 's/PRIORITY_HIGH/High/g; s/PRIORITY_NORMAL/Normal/g; s/PRIORITY_LOW/Low/g' include/interfaces/IDataSource.hpp

# TextAlign
sed -i '' 's/^    LEFT,/    Left,/' modules/u8g2Display/widgets/DrawingPrimitives.hpp
sed -i '' 's/^    CENTER,/    Center,/' modules/u8g2Display/widgets/DrawingPrimitives.hpp
sed -i '' 's/^    RIGHT/    Right/' modules/u8g2Display/widgets/DrawingPrimitives.hpp

# ClockFormat
sed -i '' 's/HH_MM_SS/HhMmSs/g; s/HH_MM/HhMm/g' modules/u8g2Display/widgets/ClockWidget.hpp
sed -i '' 's/HH_MM_SS/HhMmSs/g; s/HH_MM/HhMm/g' modules/u8g2Display/widgets/ClockWidget.cpp

# Phase (BootBoard)
sed -i '' 's/SPLASH/Splash/g; s/LOADING/Loading/g' modules/u8g2Display/boards/BootBoard.hpp
sed -i '' 's/SPLASH/Splash/g; s/LOADING/Loading/g' modules/u8g2Display/boards/BootBoard.cpp

# WiFiState
sed -i '' 's/WIFI_INIT/Init/g; s/WIFI_STA_CONNECTING/StaConnecting/g; s/WIFI_AP_STARTING/ApStarting/g; s/WIFI_READY/Ready/g; s/WIFI_SHUTDOWN/Shutdown/g' modules/wifiManager/WifiManager.hpp
sed -i '' 's/WIFI_INIT/Init/g; s/WIFI_STA_CONNECTING/StaConnecting/g; s/WIFI_AP_STARTING/ApStarting/g; s/WIFI_READY/Ready/g; s/WIFI_SHUTDOWN/Shutdown/g' modules/wifiManager/WifiManager.cpp

echo ""
echo "=== Phases 2-4 Complete ==="
