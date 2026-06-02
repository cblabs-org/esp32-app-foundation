#!/bin/bash
# Fix remaining class/interface references that macOS sed \b didn't catch
# Using explicit word-boundary patterns for BSD sed
set -e

cd "$(git rev-parse --show-toplevel)"

# Helper: apply sed to all C++ source files including tests
fix_all() {
    find modules lib include src test -type f \( -name "*.hpp" -o -name "*.cpp" -o -name "*.h" \) \
        ! -path "*/portal_assets/*" \
        -exec sed -i '' "$1" {} +
}

echo "=== Fixing remaining class/interface name references ==="

# Interface class declarations and references
# The pattern: replace standalone old name with new, being careful not to match substrings
# Using [^a-zA-Z0-9_] as word boundary for BSD sed

echo "  Fixing iConfigurable..."
fix_all 's/[[:<:]]iConfigurable[[:>:]]/IConfigurable/g'

echo "  Fixing iDataSource..."
fix_all 's/[[:<:]]iDataSource[[:>:]]/IDataSource/g'

echo "  Fixing iGfxWidget..."
fix_all 's/[[:<:]]iGfxWidget[[:>:]]/IGfxWidget/g'

echo "  Fixing iBoardLayout..."
fix_all 's/[[:<:]]iBoardLayout[[:>:]]/IBoardLayout/g'

# Class renames
echo "  Fixing otaUpdateManager..."
fix_all 's/[[:<:]]otaUpdateManager[[:>:]]/OtaUpdateManager/g'

echo "  Fixing clockWidget..."
fix_all 's/[[:<:]]clockWidget[[:>:]]/ClockWidget/g'

echo "  Fixing imageWidget..."
fix_all 's/[[:<:]]imageWidget[[:>:]]/ImageWidget/g'

echo "  Fixing labelWidget..."
fix_all 's/[[:<:]]labelWidget[[:>:]]/LabelWidget/g'

echo "  Fixing progressBarWidget..."
fix_all 's/[[:<:]]progressBarWidget[[:>:]]/ProgressBarWidget/g'

echo "  Fixing scrollingMessagePoolWidget..."
fix_all 's/[[:<:]]scrollingMessagePoolWidget[[:>:]]/ScrollingMessagePoolWidget/g'

echo "  Fixing scrollingTextWidget..."
fix_all 's/[[:<:]]scrollingTextWidget[[:>:]]/ScrollingTextWidget/g'

echo "  Fixing systemMessageWidget..."
fix_all 's/[[:<:]]systemMessageWidget[[:>:]]/SystemMessageWidget/g'

echo "  Fixing wifiStatusWidget..."
fix_all 's/[[:<:]]wifiStatusWidget[[:>:]]/WifiStatusWidget/g'

echo ""
echo "=== Verification ==="
echo "Checking for remaining old names..."

REMAINING=$(grep -rn 'class otaUpdateManager\|class clockWidget\|class imageWidget\|class labelWidget\|class progressBarWidget\|class scrollingTextWidget\|class scrollingMessagePoolWidget\|class systemMessageWidget\|class wifiStatusWidget\|class iGfxWidget\|class iBoardLayout\|class iConfigurable\|class iDataSource' modules/ lib/ include/ src/ --include="*.hpp" --include="*.cpp" --include="*.h" 2>/dev/null | grep -v portal_assets || true)

if [ -z "$REMAINING" ]; then
    echo "  ✅ All class declarations updated"
else
    echo "  ⚠ Still remaining:"
    echo "$REMAINING"
fi

echo ""
echo "=== Fix Complete ==="
