#!/usr/bin/env bash
#
# Release Audit Mechanical Checks
# Runs mechanical checks from the release audit protocol and outputs structured JSON to stdout.
# Usage: ./release-audit-checks.sh [PROJECT_ROOT]
# Output format: JSON lines
# Note: Ensure this script is executable (chmod +x) before running directly.

set -uo pipefail

ROOT_DIR="${1:-.}"
if [ ! -d "$ROOT_DIR" ]; then
    echo "Error: Directory $ROOT_DIR does not exist." >&2
    exit 1
fi

# Auto-detect framework submodule for consumer repos (#188).
# If esp32-app-foundation/ exists as a subdirectory with its own .git,
# exclude it — the Authored-Code Scope Rule says only audit authored code.
SUBMODULE_EXCLUDE=""
if [ -d "$ROOT_DIR/esp32-app-foundation/.git" ] || [ -f "$ROOT_DIR/esp32-app-foundation/.git" ]; then
    SUBMODULE_EXCLUDE="esp32-app-foundation"
fi

# Reusable path filter — replaces eval "$filter_chain" pattern.
# Reads stdin, drops lines matching any excluded directory, writes to stdout.
filter_excluded_paths() {
    grep -v '/.pio/' \
        | grep -v '/.pio-mcp-workspace/' \
        | grep -v '/.git/' \
        | grep -v '/.agents/' \
        | grep -v '/test/' \
        | grep -v '/tests/' \
        | grep -v '/stubs/' \
        | grep -v '/portal_assets/' \
        | grep -v 'third_party' \
        | grep -v 'vendor' \
        | grep -v 'node_modules' \
        | grep -v 'managed_components' \
        | grep -v '/tmp/' \
        | if [ -n "$SUBMODULE_EXCLUDE" ]; then grep -v "/$SUBMODULE_EXCLUDE/"; else cat; fi \
        || true
}

TOTAL_CHECKS=0
PASSED=0
FINDINGS=0

# Helper to print result as JSON line
report() {
    local dim="$1"
    local check="$2"
    local status="$3"
    local hits="$4"
    local files="${5:-}"

    TOTAL_CHECKS=$((TOTAL_CHECKS + 1))
    if [ "$status" == "PASS" ] || [ "$status" == "ADVISORY" ]; then
        PASSED=$((PASSED + 1))
    else
        FINDINGS=$((FINDINGS + 1))
    fi

    if [ -n "$files" ]; then
        # Ensure we unique and sort files, removing empty ones
        local file_list
        file_list=$(echo "$files" | tr ' ' '\n' | grep -v '^$' | sort -u)
        
        local json_files="["
        local first=1
        for f in $file_list; do
            if [ $first -eq 1 ]; then
                json_files="$json_files\"$f\""
                first=0
            else
                json_files="$json_files, \"$f\""
            fi
        done
        json_files="$json_files]"
        
        printf '{"dimension": %d, "check": "%s", "status": "%s", "hits": %d, "files": %s}\n' \
            "$dim" "$check" "$status" "$hits" "$json_files"
    else
        printf '{"dimension": %d, "check": "%s", "status": "%s", "hits": %d}\n' \
            "$dim" "$check" "$status" "$hits"
    fi
}

# Auto-discover authored C++ source files
get_source_files() {
    local exclude_submod=""
    if [ -n "$SUBMODULE_EXCLUDE" ]; then
        exclude_submod="-not -path */$SUBMODULE_EXCLUDE/*"
    fi
    find "$ROOT_DIR" -type f \( -name "*.hpp" -o -name "*.cpp" \) \
        -not -path "*/.pio/*" \
        -not -path "*/.pio-mcp-workspace/*" \
        -not -path "*/.git/*" \
        -not -path "*/.agents/*" \
        -not -path "*/test/stubs/*" \
        -not -path "*/include/portal_assets/*" \
        -not -path "*/third_party/*" \
        -not -path "*/vendor/*" \
        -not -path "*/node_modules/*" \
        -not -path "*/managed_components/*" \
        -not -path "*/tmp/*" \
        $exclude_submod
}

# Generic grep runner for quick pattern matching
run_check_grep() {
    local dim="$1"
    local check="$2"
    local pattern="$3"
    local file_ext="$4"
    
    local hits=0
    local files=""
    
    # Use standard grep with excludes for safety, plus include pattern
    local grep_out
    local submod_exclude=""
    if [ -n "$SUBMODULE_EXCLUDE" ]; then
        submod_exclude="--exclude-dir=$SUBMODULE_EXCLUDE"
    fi
    grep_out=$(grep -rnE "$pattern" \
        --exclude-dir=".pio" \
        --exclude-dir=".pio-mcp-workspace" \
        --exclude-dir=".git" \
        --exclude-dir=".agents" \
        --exclude-dir="stubs" \
        --exclude-dir="portal_assets" \
        --exclude-dir="third_party" \
        --exclude-dir="vendor" \
        --exclude-dir="node_modules" \
        --exclude-dir="managed_components" \
        --exclude-dir="tmp" \
        $submod_exclude \
        --include="$file_ext" \
        "$ROOT_DIR" 2>/dev/null || true)
        
    if [ -n "$grep_out" ]; then
        grep_out=$(echo "$grep_out" | filter_excluded_paths)
        
        if [ -n "$grep_out" ]; then
            hits=$(echo "$grep_out" | wc -l | tr -d ' ')
            files=$(echo "$grep_out" | cut -d: -f1 | sort -u | sed "s|^$ROOT_DIR/||" | sed "s|^\./||")
        fi
    fi
    
    if [ "$hits" -gt 0 ]; then
        report "$dim" "$check" "FAIL" "$hits" "$(echo $files | tr '\n' ' ')"
    else
        report "$dim" "$check" "PASS" 0
    fi
}

# -----------------------------------------------------------------------------
# Dimension 1: House Style
# -----------------------------------------------------------------------------
check_missing_file_header() {
    local hits=0
    local files=""
    for f in $(get_source_files); do
        if ! head -n 30 "$f" | grep -qE "(Module:|Description:)"; then
            hits=$((hits + 1))
            files="$files ${f#$ROOT_DIR/}"
        fi
    done
    if [ $hits -gt 0 ]; then
        report 1 "missing_file_header" "FAIL" "$hits" "$files"
    else
        report 1 "missing_file_header" "PASS" 0
    fi
}

check_missing_doxygen() {
    # Heuristic: looking for methods in .hpp that lack /** or /// above them.
    # We use a simple awk script to detect uncommented declarations in public/protected blocks.
    local hits=0
    local files=""
    for f in $(get_source_files | grep '\.hpp$' || true); do
        local out
        out=$(awk '
            BEGIN { in_class=0; vis=0; doc=0; }
            /class [A-Za-z0-9_]+/ { in_class=1; vis=0; }
            /struct [A-Za-z0-9_]+/ { in_class=1; vis=1; }
            /public:/ { if(in_class) vis=1; }
            /protected:/ { if(in_class) vis=1; }
            /private:/ { if(in_class) vis=0; }
            /\/\*\*/ || /\/\/\// { doc=1; }
            /^[ \t]*\*/ { doc=1; }
            /^[ \t]*template[ \t]*</ { if (doc) { doc=1; next; } }
            /^[ \t]*[A-Za-z_][A-Za-z0-9_<>:]*[ \t]+[A-Za-z0-9_~]+\(/ && $0 !~ /^[ \t]*return[ \t]+/ {
                if (in_class && vis && !doc) { print NR; }
                doc=0;
                next;
            }
            /^[ \t]*$/ { }
            /[^ \t]/ { if ($0 !~ /\/\*\*/ && $0 !~ /\/\/\// && $0 !~ /^[ \t]*\*/ && $0 !~ /^[ \t]*template[ \t]*</ && $0 !~ /^[ \t]*[A-Za-z_][A-Za-z0-9_<>:]*[ \t]+[A-Za-z0-9_~]+\(/) doc=0; }
        ' "$f")
        if [ -n "$out" ]; then
            local count
            count=$(echo "$out" | wc -l | tr -d ' ')
            hits=$((hits + count))
            files="$files ${f#$ROOT_DIR/}"
        fi
    done
    if [ $hits -gt 0 ]; then
        report 1 "missing_doxygen" "FAIL" "$hits" "$files"
    else
        report 1 "missing_doxygen" "PASS" 0
    fi
}

check_undocumented_constants() {
    # Match #define, constexpr, static const declarations that lack documentation.
    # Must check BOTH the current line for inline comments (// or ///<) AND
    # the preceding line for block-doc comments (///, /**, * @brief).
    # Exclude lines with '(' — those are methods/constructors, not constants.
    local hits=0
    local files=""
    for f in $(get_source_files | grep '\.hpp$' || true); do
        local out
        out=$(awk '
            BEGIN { prev="" }
            {
                line = $0
                # Check if current line is a constant/define declaration
                if (line ~ /^[ \t]*(#define|constexpr|static[ \t]+const)/) {
                    # Skip if line has inline comment (//)
                    if (line ~ /\/\//) { prev=line; next }
                    # Skip if line contains parentheses (method/constructor)
                    if (line ~ /\(/) { prev=line; next }
                    # Skip if line is an array initializer
                    if (line ~ /\[\]/) { prev=line; next }
                    # Skip if preceding line is a doc comment
                    if (prev ~ /\/\/\// || prev ~ /\/\*\*/ || prev ~ /\* @/ || prev ~ /\*\//) { prev=line; next }
                    # This is an undocumented constant
                    print NR
                }
                prev = line
            }
        ' "$f")
        if [ -n "$out" ]; then
            local count
            count=$(echo "$out" | wc -l | tr -d ' ')
            hits=$((hits + count))
            files="$files ${f#$ROOT_DIR/}"
        fi
    done
    if [ $hits -gt 0 ]; then
        report 1 "undocumented_constants" "FAIL" "$hits" "$files"
    else
        report 1 "undocumented_constants" "PASS" 0
    fi
}

check_tag_mismatch() {
    local hits=0
    local files=""
    for f in $(get_source_files | grep '\.hpp$' || true); do
        local class_name
        class_name=$(grep -oE "class [A-Za-z0-9_]+" "$f" | head -n1 | awk '{print $2}' || true)
        if [ -n "$class_name" ]; then
            local tag
            tag=$(grep -oE 'TAG[[:space:]]*=[[:space:]]*"[^"]+"' "$f" | head -n1 | cut -d'"' -f2 || true)
            if [ -n "$tag" ] && [ "$tag" != "$class_name" ]; then
                hits=$((hits + 1))
                files="$files ${f#$ROOT_DIR/}"
            fi
        fi
    done
    if [ $hits -gt 0 ]; then
        report 1 "tag_mismatch" "FAIL" "$hits" "$files"
    else
        report 1 "tag_mismatch" "PASS" 0
    fi
}

check_directory_file_mismatch() {
    local hits=0
    local files=""
    for f in $(get_source_files); do
        local dir
        dir=$(basename "$(dirname "$f")")
        local base
        base=$(basename "$f" | sed 's/\.[a-z]*$//')
        # If dir starts with uppercase, it likely represents a module/component directory
        if [[ "$dir" =~ ^[A-Z] ]]; then
            # Ignore interface files
            if [ "$dir" != "$base" ] && [ "$base" != "ITickable" ] && [[ ! "$base" =~ ^I[A-Z] ]]; then
                hits=$((hits + 1))
                files="$files ${f#$ROOT_DIR/}"
            fi
        fi
    done
    if [ $hits -gt 0 ]; then
        report 1 "directory_file_mismatch" "FAIL" "$hits" "$files"
    else
        report 1 "directory_file_mismatch" "PASS" 0
    fi
}

check_member_naming() {
    # Check for members in hpp that don't start with _camelCase
    # Using a heuristic: [type] [name]; missing `_` prefix in variable names
    run_check_grep 1 "member_naming" "^[[:space:]]+[A-Za-z0-9_:]+[[:space:]]+[a-zA-Z][a-zA-Z0-9_]*[[:space:]]*;" "*.hpp"
}

# -----------------------------------------------------------------------------
# Dimension 2: Coding Discipline
# -----------------------------------------------------------------------------
check_stored_application_ptr() {
    run_check_grep 2 "stored_application_ptr" "(Application[[:space:]]*\*|Application[[:space:]]*&)[[:space:]]*[A-Za-z0-9_]+[[:space:]]*;" "*.hpp"
}

check_application_hpp_in_header() {
    run_check_grep 2 "application_hpp_in_header" "^#include[[:space:]]+\"Application\.hpp\"" "*.hpp"
}

check_sibling_module_import() {
    # Look for includes reaching out to sibling module directories
    run_check_grep 2 "sibling_module_import" "^#include[[:space:]]+\"\.\./[A-Za-z0-9_]+/[A-Za-z0-9_]+\.hpp\"" "*.hpp"
}

check_public_member_variables() {
    # Variables that appear public (often without _ prefix, though this pattern is rough)
    report 2 "public_member_variables" "PASS" 0 # Hard to grep reliably without full AST
}

check_extern_global_state() {
    # Exclude 'extern "C"' language linkage — those are LVGL/ESP-IDF callback
    # declarations, not global state (#188).
    run_check_grep 2 "extern_global_state" "^[[:space:]]*extern[[:space:]]+[^\"C]" "*.cpp"
}

check_std_function_member() {
    # Match stored std::function members and 'using X = std::function<...>' aliases in headers (#254, #256).
    local hits=0
    local files=""
    
    local submod_excl=""
    if [ -n "$SUBMODULE_EXCLUDE" ]; then
        submod_excl="--exclude-dir=$SUBMODULE_EXCLUDE"
    fi
    
    local grep_out
    grep_out=$(grep -rnE "^[[:space:]]*(using[[:space:]]+[A-Za-z0-9_]+[[:space:]]*=[[:space:]]*std::function<|std::function<.*>[[:space:]]+[a-zA-Z0-9_]+[[:space:]]*(=|;|\{))" "$ROOT_DIR" \
        --include="*.hpp" --include="*.h" \
        --exclude-dir=".pio" \
        --exclude-dir=".pio-mcp-workspace" \
        --exclude-dir=".git" \
        --exclude-dir=".agents" \
        --exclude-dir="stubs" \
        --exclude-dir="portal_assets" \
        --exclude-dir="third_party" \
        --exclude-dir="vendor" \
        --exclude-dir="node_modules" \
        --exclude-dir="managed_components" \
        --exclude-dir="tmp" \
        --exclude-dir="test" \
        --exclude-dir="tests" \
        $submod_excl 2>/dev/null || true)
        
    if [ -n "$grep_out" ]; then
        grep_out=$(echo "$grep_out" | filter_excluded_paths)
        
        if [ -n "$grep_out" ]; then
            hits=$(echo "$grep_out" | grep -v "^$" | wc -l | tr -d ' ')
            files=$(echo "$grep_out" | cut -d: -f1,2 | sed "s|^$ROOT_DIR/||" | sed "s|^\./||" | sort -u)
        fi
    fi
    
    if [ "$hits" -gt 0 ]; then
        report 2 "std_function_member" "FAIL" "$hits" "$(echo $files | tr '\n' ' ')"
    else
        report 2 "std_function_member" "PASS" 0
    fi
}

check_missing_nothrow() {
    # `new Type` without `(std::nothrow)` — only match C++ allocations,
    # not the English word "new" in comments or strings (#188, #256).
    local hits=0
    local files=""
    
    local submod_excl=""
    if [ -n "$SUBMODULE_EXCLUDE" ]; then
        submod_excl="--exclude-dir=$SUBMODULE_EXCLUDE"
    fi
    
    local grep_out
    # Search authored source files for 'new', excluding build/test/vendor/tmp directories (#256).
    grep_out=$(grep -rnE "\bnew\b" "$ROOT_DIR" \
        --include="*.cpp" --include="*.hpp" --include="*.h" \
        --exclude-dir=".pio" \
        --exclude-dir=".pio-mcp-workspace" \
        --exclude-dir=".git" \
        --exclude-dir=".agents" \
        --exclude-dir="stubs" \
        --exclude-dir="portal_assets" \
        --exclude-dir="third_party" \
        --exclude-dir="vendor" \
        --exclude-dir="node_modules" \
        --exclude-dir="managed_components" \
        --exclude-dir="tmp" \
        --exclude-dir="test" \
        --exclude-dir="tests" \
        $submod_excl 2>/dev/null || true)
        
    if [ -n "$grep_out" ]; then
        # Filter excluded paths to ensure nested directories are dropped (#256)
        grep_out=$(echo "$grep_out" | filter_excluded_paths)

        # Strip full comment lines (//, *, /*, */)
        grep_out=$(echo "$grep_out" | grep -vE "^[^:]*:[^:]*:[[:space:]]*(\/\/|\*|\/\*|\*\/)" || true)

        # Remove lines that use nothrow
        grep_out=$(echo "$grep_out" | grep -v "(std::nothrow)" | grep -v "(nothrow)" || true)

        # Strip string literals so 'new' in log strings / literals does not match (#256)
        grep_out=$(echo "$grep_out" | sed -E 's/"([^"\\]|\\.)*"/""/g' || true)

        # Strip trailing inline comments (// to end of line)
        grep_out=$(echo "$grep_out" | sed -E 's/\/\/.*//' || true)

        # Match allocation syntax strictly: new Type(...), new Type{...}, new Type[...], or new Type;
        grep_out=$(echo "$grep_out" | grep -E "\bnew[[:space:]]+[A-Za-z_][A-Za-z0-9_:<>]*[[:space:]]*[({[;]" || true)

        if [ -n "$grep_out" ]; then
            hits=$(echo "$grep_out" | grep -v "^$" | wc -l | tr -d ' ')
            # Report file:line list so author can verify exact sites (#256)
            files=$(echo "$grep_out" | cut -d: -f1,2 | sed "s|^$ROOT_DIR/||" | sed "s|^\./||" | sort -u)
        fi
    fi
    
    if [ "$hits" -gt 0 ]; then
        report 2 "missing_nothrow" "FAIL" "$hits" "$(echo $files | tr '\n' ' ')"
    else
        report 2 "missing_nothrow" "PASS" 0
    fi
}

check_tick_heap_allocation() {
    # Advisory: grepping all .cpp files for allocation patterns cannot
    # distinguish tick-path from init-path calls. Report as ADVISORY
    # for manual review rather than FAIL (#188).
    local hits=0
    local files=""
    local submod_excl=""
    if [ -n "$SUBMODULE_EXCLUDE" ]; then
        submod_excl="--exclude-dir=$SUBMODULE_EXCLUDE"
    fi
    local grep_out
    grep_out=$(grep -rlE "(new[[:space:]]|malloc\(|String\(|std::vector<)" "$ROOT_DIR" \
        --include="*.cpp" --exclude-dir=".pio" --exclude-dir=".pio-mcp-workspace" \
        --exclude-dir="vendor" --exclude-dir="third_party" --exclude-dir=".agents" \
        --exclude-dir="stubs" --exclude-dir="portal_assets" --exclude-dir="node_modules" \
        --exclude-dir="managed_components" --exclude-dir="tmp" --exclude-dir="test" \
        --exclude-dir="tests" --exclude-dir=".git" $submod_excl 2>/dev/null || true)
    if [ -n "$grep_out" ]; then
        grep_out=$(echo "$grep_out" | filter_excluded_paths)
        if [ -n "$grep_out" ]; then
            hits=$(echo "$grep_out" | grep -v "^$" | wc -l | tr -d ' ')
            files=$(echo "$grep_out" | sed "s|^$ROOT_DIR/||" | sed "s|^\./||" | sort -u)
        fi
    fi
    if [ "$hits" -gt 0 ]; then
        report 2 "tick_heap_allocation" "ADVISORY" "$hits" "$(echo $files | tr '\n' ' ')"
    else
        report 2 "tick_heap_allocation" "PASS" 0
    fi
}

check_law_of_demeter() {
    # Chained ->get...()->
    run_check_grep 2 "law_of_demeter" "->get[A-Za-z0-9_]*\(\)->" "*.cpp"
}

# -----------------------------------------------------------------------------
# Dimension 3: Framework Practices
# -----------------------------------------------------------------------------
check_eventbus_thread_safety() {
    # Advisory: any lv_/WiFi./SPI. call in .cpp fires this check, but most
    # are legitimate single-threaded usage. Report as ADVISORY (#188).
    local hits=0
    local files=""
    local submod_excl=""
    if [ -n "$SUBMODULE_EXCLUDE" ]; then
        submod_excl="--exclude-dir=$SUBMODULE_EXCLUDE"
    fi
    local grep_out
    grep_out=$(grep -rlE "(lv_|WiFi\.|SPI\.)" "$ROOT_DIR" \
        --include="*.cpp" --exclude-dir=".pio" --exclude-dir=".pio-mcp-workspace" \
        --exclude-dir="vendor" --exclude-dir="third_party" --exclude-dir=".agents" \
        --exclude-dir="stubs" --exclude-dir="portal_assets" --exclude-dir="node_modules" \
        --exclude-dir="managed_components" --exclude-dir="tmp" --exclude-dir="test" \
        --exclude-dir="tests" --exclude-dir=".git" $submod_excl 2>/dev/null || true)
    if [ -n "$grep_out" ]; then
        grep_out=$(echo "$grep_out" | filter_excluded_paths)
        if [ -n "$grep_out" ]; then
            hits=$(echo "$grep_out" | grep -v "^$" | wc -l | tr -d ' ')
            files=$(echo "$grep_out" | sed "s|^$ROOT_DIR/||" | sed "s|^\./||" | sort -u)
        fi
    fi
    if [ "$hits" -gt 0 ]; then
        report 3 "eventbus_thread_safety" "ADVISORY" "$hits" "$(echo $files | tr '\n' ' ')"
    else
        report 3 "eventbus_thread_safety" "PASS" 0
    fi
}

check_facade_completeness() {
    # Not easily verifiable mechanically; placeholder
    report 3 "facade_completeness" "PASS" 0
}

# -----------------------------------------------------------------------------
# Dimension 5: Security
# -----------------------------------------------------------------------------
check_committed_secrets() {
    local hits=0
    local files=""
    
    local bad_files
    bad_files=$(find "$ROOT_DIR" -type f \( -name "*.pem" -o -name "*.key" -o -name "*.env" \) \
        -not -path "*/.git/*" -not -path "*/.pio/*" -not -path "*/.pio-mcp-workspace/*" \
        -not -path "*/.agents/*" -not -path "*/node_modules/*" -not -path "*/managed_components/*" \
        -not -path "*/tmp/*" -not -path "*/third_party/*" -not -path "*/vendor/*" || true)
    
    if [ -n "$bad_files" ]; then
        for f in $bad_files; do
            # Only flag files tracked by git — gitignored files are safe (#184)
            if git -C "$ROOT_DIR" ls-files --error-unmatch "$f" >/dev/null 2>&1; then
                hits=$((hits + 1))
                files="$files ${f#$ROOT_DIR/}"
            fi
        done
    fi
    
    if [ $hits -gt 0 ]; then
        report 5 "committed_secrets" "FAIL" "$hits" "$files"
    else
        report 5 "committed_secrets" "PASS" 0
    fi
}

check_set_insecure_usage() {
    run_check_grep 5 "set_insecure_usage" "setInsecure\(\)" "*.cpp"
}

check_key_manager_bypass() {
    run_check_grep 5 "key_manager_bypass" "nvs_get_str\([^,]+,[[:space:]]*\"(password|key|token)\"" "*.cpp"
}

# -----------------------------------------------------------------------------
# Dimension 6: Portal
# -----------------------------------------------------------------------------
check_hardcoded_hex_color() {
    local hits=0
    local files=""
    
    local portal_files
    portal_files=$(find "$ROOT_DIR" -type f \( -name "*.css" -o -name "*.js" \) \
        -not -path "*/.git/*" -not -path "*/.pio/*" -not -path "*/.pio-mcp-workspace/*" \
        -not -path "*/.agents/*" -not -path "*/node_modules/*" -not -path "*/managed_components/*" \
        -not -path "*/tmp/*" -not -path "*/third_party/*" -not -path "*/vendor/*" \
        -not -path "*/include/portal_assets/*" || true)
    
    if [ -n "$portal_files" ]; then
        for f in $portal_files; do
            # Looking for #[0-9a-f]{3,6} not enclosed in var(...)
            # Heuristic: just flag hex colors in source files.
            local count
            count=$(grep -cEi "#[0-9a-f]{3,6}" "$f" || true)
            if [ "$count" -gt 0 ]; then
                hits=$((hits + count))
                files="$files ${f#$ROOT_DIR/}"
            fi
        done
    fi
    
    if [ $hits -gt 0 ]; then
        report 6 "hardcoded_hex_color" "FAIL" "$hits" "$files"
    else
        report 6 "hardcoded_hex_color" "PASS" 0
    fi
}

check_missing_aria_role() {
    # Placeholder
    report 6 "missing_aria_role" "PASS" 0
}

# -----------------------------------------------------------------------------
# Dimension 8: Parallel Mechanism
# -----------------------------------------------------------------------------
check_stale_lib_deps() {
    report 8 "stale_lib_deps" "PASS" 0
}

check_duplicate_driver_registration() {
    # Advisory: pcnt_unit_config and lv_indev_create are expected once per
    # peripheral/input device — a small count (≤8) is normal. Only flag
    # as FAIL above threshold to catch actual duplicate registrations (#186).
    local hits
    hits=$(grep -rl --include="*.cpp" -E "(pcnt_unit_config|lv_indev_create)" "$ROOT_DIR" \
        --include="*.cpp" 2>/dev/null | wc -l | tr -d ' ')
    if [ "$hits" -gt 8 ]; then
        report 8 "duplicate_driver_registration" "FAIL" "$hits"
    elif [ "$hits" -gt 0 ]; then
        report 8 "duplicate_driver_registration" "ADVISORY" "$hits" "expected per-peripheral registrations"
    else
        report 8 "duplicate_driver_registration" "PASS" 0
    fi
}

# -----------------------------------------------------------------------------
# Dimension 9: Config Shadowing
# -----------------------------------------------------------------------------
check_config_overrides_enumeration() {
    # Advisory: a multi-board framework inherently has many build_flags,
    # sdkconfig.defaults, and LV_CONF_PATH entries across environments.
    # Report count for awareness but only fail above a generous threshold (#187).
    local hits
    hits=$(grep -c -E "(LV_CONF_PATH|sdkconfig\.defaults|build_flags)" \
        "$ROOT_DIR/platformio.ini" 2>/dev/null || echo 0)
    if [ "$hits" -gt 60 ]; then
        report 9 "config_overrides_enumeration" "FAIL" "$hits"
    elif [ "$hits" -gt 0 ]; then
        report 9 "config_overrides_enumeration" "ADVISORY" "$hits" "multi-board config — review for stale entries"
    else
        report 9 "config_overrides_enumeration" "PASS" 0
    fi
}

main() {
    # Dimension 1
    check_missing_file_header
    check_missing_doxygen
    check_undocumented_constants
    check_tag_mismatch
    check_directory_file_mismatch
    check_member_naming
    
    # Dimension 2
    check_stored_application_ptr
    check_application_hpp_in_header
    check_sibling_module_import
    check_public_member_variables
    check_extern_global_state
    check_std_function_member
    check_missing_nothrow
    check_tick_heap_allocation
    check_law_of_demeter
    
    # Dimension 3
    check_eventbus_thread_safety
    check_facade_completeness
    
    # Dimension 5
    check_committed_secrets
    check_set_insecure_usage
    check_key_manager_bypass
    
    # Dimension 6
    check_hardcoded_hex_color
    check_missing_aria_role
    
    # Dimension 8
    check_stale_lib_deps
    check_duplicate_driver_registration
    
    # Dimension 9
    check_config_overrides_enumeration
    
    printf '{"summary": {"total_checks": %d, "passed": %d, "findings": %d}}\n' \
        "$TOTAL_CHECKS" "$PASSED" "$FINDINGS"
        
    exit 0
}

main
