#!/bin/bash
# Fix git index duplicates caused by case-insensitive git mv on macOS
# Strategy: temporarily set core.ignorecase=false, remove old entries, restore
set -e

cd "$(git rev-parse --show-toplevel)"

# Set git to case-sensitive temporarily
git config core.ignorecase false

echo "=== Removing old lowercase index entries ==="

# Get all case-insensitive duplicates
git ls-files | sort -f | uniq -di | while read -r upper; do
    # Find the lowercase version by looking for entries that differ only in case
    lower=$(git ls-files | grep -i "^${upper}$" | grep -v "^${upper}$" | head -1)
    if [ -n "$lower" ]; then
        echo "  Removing old: $lower (keeping: $upper)"
        git rm --cached "$lower" 2>/dev/null || true
    fi
done

# Restore case-insensitive mode
git config core.ignorecase true

echo ""
echo "=== Verification ==="
DUPES=$(git ls-files | sort -f | uniq -di | wc -l | tr -d ' ')
echo "Remaining duplicates: $DUPES"

echo ""
echo "=== Done ==="
