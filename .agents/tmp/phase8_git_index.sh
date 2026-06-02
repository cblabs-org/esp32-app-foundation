#!/bin/bash
# Phase 8: Rebuild git index for all renamed directories and files
# Uses git update-index for reliable macOS case-insensitive handling
set -e
cd "$(git rev-parse --show-toplevel)"

git config core.ignorecase false

echo "=== Rebuilding git index ==="

# Remove ALL old entries for modules/, lib/, include/interfaces/
git rm --cached -r modules/ lib/ include/interfaces/ 2>/dev/null || true

# Re-add everything with correct casing
git add modules/ lib/ include/interfaces/

# Also re-add other modified files
git add include/AppFoundation.h include/AppState.hpp src/ test/

echo ""
echo "=== Verification ==="
echo "Duplicates:"
DUPES=$(git ls-files | sort -f | uniq -di | wc -l | tr -d ' ')
echo "  $DUPES"

echo ""
echo "Sample index entries:"
git ls-files modules/MCPServer/
echo "---"
git ls-files modules/OTAUpdateManager/
echo "---"
git ls-files lib/OTADownloadEngine/
echo "---"
git ls-files modules/U8G2Display/ | head -5

git config core.ignorecase true

echo ""
echo "=== Done ==="
