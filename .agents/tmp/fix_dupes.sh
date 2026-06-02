#!/bin/bash
# Fix git index: remove ALL lowercase entries that have PascalCase equivalents
set -e
cd "$(git rev-parse --show-toplevel)"
git config core.ignorecase false

echo "=== Removing duplicate lowercase entries ==="

# Find all case-insensitive duplicates
git ls-files | sort -f | uniq -di | while read -r upper; do
    # Find the lowercase version
    lower=$(git ls-files | grep -i "^${upper}$" | grep -v "^${upper}$" | head -1)
    if [ -n "$lower" ]; then
        echo "  rm: $lower"
        git update-index --force-remove "$lower" 2>/dev/null || true
    fi
done

git config core.ignorecase true

echo ""
DUPES=$(git ls-files | sort -f | uniq -di | wc -l | tr -d ' ')
echo "Remaining duplicates: $DUPES"
echo "=== Done ==="
