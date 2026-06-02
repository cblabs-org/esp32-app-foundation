#!/bin/bash
# Final git index rebuild: fix ALL file entries to use correct PascalCase
# Walks every file on disk in modules/, lib/, include/ and ensures the index matches
set -e
cd "$(git rev-parse --show-toplevel)"
git config core.ignorecase false

echo "=== Rebuilding entire index for modules/, lib/, include/ ==="

# Remove all current entries
git rm --cached -r modules/ lib/ include/ 2>/dev/null || true

# Now use hash-object + update-index for every file, preserving the ACTUAL filesystem path
# On macOS, 'find' returns the path as it is on disk
for file in $(find modules/ lib/ include/ -type f | grep -v '.pio' | grep -v portal_assets | sort); do
    hash=$(git hash-object -w "$file")
    git update-index --add --cacheinfo 100644,"$hash","$file"
done

# Also re-add portal_assets and .gitkeep files
for file in $(find include/portal_assets -type f 2>/dev/null | sort); do
    hash=$(git hash-object -w "$file")
    git update-index --add --cacheinfo 100644,"$hash","$file"
done

# Re-add src/ and test/ normally (no case changes there)
git add src/ test/

git config core.ignorecase true

echo ""
echo "=== Verification ==="
DUPES=$(git ls-files | sort -f | uniq -di | wc -l | tr -d ' ')
echo "Duplicates: $DUPES"

echo ""
echo "Sample paths:"
git ls-files lib/DeviceCrypto/
echo "---"
git ls-files lib/Logger/
echo "---"
git ls-files modules/MCPServer/
echo "---"
git ls-files modules/U8G2Display/ | head -8

echo ""
echo "=== Done ==="
