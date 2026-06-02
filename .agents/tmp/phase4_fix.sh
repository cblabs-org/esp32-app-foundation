#!/bin/bash
# Fix remaining enum value references using BSD word boundaries
set -e

cd "$(git rev-parse --show-toplevel)"

fix_all() {
    find modules lib include src test -type f \( -name "*.hpp" -o -name "*.cpp" -o -name "*.h" \) \
        ! -path "*/portal_assets/*" \
        -exec sed -i '' "$1" {} +
}

echo "=== Fixing remaining enum value references ==="

# AppState
echo "  AppState..."
fix_all 's/AppState::BOOT/AppState::Boot/g'
fix_all 's/AppState::CONFIGURING/AppState::Configuring/g'
fix_all 's/AppState::RUNNING/AppState::Running/g'
fix_all 's/AppState::UPDATING/AppState::Updating/g'
fix_all 's/AppState::ERROR/AppState::Error/g'

# TimeSyncStatus
echo "  TimeSyncStatus..."
fix_all 's/TimeSyncStatus::IN_PROGRESS/TimeSyncStatus::InProgress/g'
fix_all 's/TimeSyncStatus::UNSYNCED/TimeSyncStatus::Unsynced/g'
fix_all 's/TimeSyncStatus::SYNCED/TimeSyncStatus::Synced/g'

# PriorityTier
echo "  PriorityTier..."
fix_all 's/PriorityTier::PRIORITY_HIGH/PriorityTier::High/g'
fix_all 's/PriorityTier::PRIORITY_NORMAL/PriorityTier::Normal/g'
fix_all 's/PriorityTier::PRIORITY_LOW/PriorityTier::Low/g'

# UpdateStatus
echo "  UpdateStatus..."
fix_all 's/UpdateStatus::SUCCESS/UpdateStatus::Success/g'
fix_all 's/UpdateStatus::NO_CHANGE/UpdateStatus::NoChange/g'
fix_all 's/UpdateStatus::RATE_LIMITED/UpdateStatus::RateLimited/g'
fix_all 's/UpdateStatus::FETCH_ERROR/UpdateStatus::FetchError/g'
fix_all 's/UpdateStatus::PARSE_ERROR/UpdateStatus::ParseError/g'
fix_all 's/UpdateStatus::AUTH_ERROR/UpdateStatus::AuthError/g'

# TextAlign
echo "  TextAlign..."
fix_all 's/TextAlign::LEFT/TextAlign::Left/g'
fix_all 's/TextAlign::CENTER/TextAlign::Center/g'
fix_all 's/TextAlign::RIGHT/TextAlign::Right/g'

# WiFiState
echo "  WiFiState..."
fix_all 's/WiFiState::WIFI_INIT/WiFiState::Init/g'
fix_all 's/WiFiState::WIFI_STA_CONNECTING/WiFiState::StaConnecting/g'
fix_all 's/WiFiState::WIFI_AP_STARTING/WiFiState::ApStarting/g'
fix_all 's/WiFiState::WIFI_READY/WiFiState::Ready/g'
fix_all 's/WiFiState::WIFI_SHUTDOWN/WiFiState::Shutdown/g'

echo ""
echo "=== Verification ==="
REMAINING=$(grep -rn 'AppState::BOOT\|AppState::CONFIGURING\|AppState::RUNNING\|AppState::UPDATING\|AppState::ERROR\|TimeSyncStatus::UNSYNCED\|TimeSyncStatus::IN_PROGRESS\|TimeSyncStatus::SYNCED\|UpdateStatus::SUCCESS\|UpdateStatus::NO_CHANGE\|PriorityTier::PRIORITY_\|TextAlign::LEFT\|TextAlign::CENTER\|TextAlign::RIGHT\|WiFiState::WIFI_' modules/ lib/ include/ src/ --include="*.hpp" --include="*.cpp" --include="*.h" 2>/dev/null | grep -v portal_assets || true)

if [ -z "$REMAINING" ]; then
    echo "  ✅ All enum values updated"
else
    echo "  ⚠ Still remaining:"
    echo "$REMAINING"
fi
