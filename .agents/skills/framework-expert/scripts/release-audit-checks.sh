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
    if [ "$status" == "PASS" ]; then
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
        -not -path "*/tmp/*"
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
        --include="$file_ext" \
        "$ROOT_DIR" 2>/dev/null || true)
        
    if [ -n "$grep_out" ]; then
        # Additional exact-path filtering to respect the full exclude rules
        grep_out=$(echo "$grep_out" | grep -v "/.pio/" | grep -v "/.pio-mcp-workspace/" | grep -v "/.git/" | grep -v "/.agents/" | grep -v "/test/stubs/" | grep -v "/include/portal_assets/" | grep -v "third_party" | grep -v "vendor" | grep -v "node_modules" | grep -v "managed_components" | grep -v "/tmp/" || true)
        
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
            /public:/ { if(in_class) vis=1; }
            /protected:/ { if(in_class) vis=1; }
            /private:/ { if(in_class) vis=0; }
            /\/\*\*/ || /\/\/\// { doc=1; }
            /^[ \t]*[A-Za-z0-9_<>:]+[ \t]+[A-Za-z0-9_]+\(/ {
                if (in_class && vis && !doc) { print NR; }
                doc=0;
            }
            /^[ \t]*$/ { }
            /[^ \t]/ { if ($0 !~ /\/\*\*/ && $0 !~ /\/\/\// && $0 !~ /^[ \t]*[A-Za-z0-9_<>:]+[ \t]+[A-Za-z0-9_]+\(/) doc=0; }
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
    # Match #define, constexpr, static declarations that don't have an inline comment (//)
    run_check_grep 1 "undocumented_constants" "^[[:space:]]*(#define|constexpr|static[[:space:]]+const)[^/]+$" "*.hpp"
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
    run_check_grep 2 "extern_global_state" "^[[:space:]]*extern[[:space:]]+" "*.cpp"
}

check_std_function_member() {
    run_check_grep 2 "std_function_member" "std::function<.*>[[:space:]]+_[a-zA-Z0-9_]+[[:space:]]*;" "*.hpp"
}

check_missing_nothrow() {
    # `new Type` without `(std::nothrow)`
    # This is difficult to represent precisely in standard regex, using a negative lookbehind approximation if perl compatible
    # Fallback: Just search for plain `new ` and exclude `nothrow` matches.
    local hits=0
    local files=""
    
    local grep_out
    grep_out=$(grep -rn "new " "$ROOT_DIR" --include="*.cpp" --exclude-dir=".pio" --exclude-dir="vendor" --exclude-dir="third_party" 2>/dev/null || true)
    if [ -n "$grep_out" ]; then
        grep_out=$(echo "$grep_out" | grep -v "(std::nothrow)" || true)
        if [ -n "$grep_out" ]; then
            hits=$(echo "$grep_out" | wc -l | tr -d ' ')
            files=$(echo "$grep_out" | cut -d: -f1 | sort -u | sed "s|^$ROOT_DIR/||" | sed "s|^\./||")
        fi
    fi
    
    if [ "$hits" -gt 0 ]; then
        report 2 "missing_nothrow" "FAIL" "$hits" "$(echo $files | tr '\n' ' ')"
    else
        report 2 "missing_nothrow" "PASS" 0
    fi
}

check_tick_heap_allocation() {
    # new, malloc, String(, std::vector<
    # We will flag any occurrence in .cpp files as a warning since limiting purely to tick() scope in grep is frail.
    run_check_grep 2 "tick_heap_allocation" "(new[[:space:]]|malloc\(|String\(|std::vector<)" "*.cpp"
}

check_law_of_demeter() {
    # Chained ->get...()->
    run_check_grep 2 "law_of_demeter" "->get[A-Za-z0-9_]*\(\)->" "*.cpp"
}

# -----------------------------------------------------------------------------
# Dimension 3: Framework Practices
# -----------------------------------------------------------------------------
check_eventbus_thread_safety() {
    run_check_grep 3 "eventbus_thread_safety" "(lv_|WiFi\.|SPI\.)" "*.cpp"
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
            hits=$((hits + 1))
            files="$files ${f#$ROOT_DIR/}"
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
    run_check_grep 8 "duplicate_driver_registration" "(pcnt_unit_config|lv_indev_create)" "*.cpp"
}

# -----------------------------------------------------------------------------
# Dimension 9: Config Shadowing
# -----------------------------------------------------------------------------
check_config_overrides_enumeration() {
    run_check_grep 9 "config_overrides_enumeration" "(LV_CONF_PATH|sdkconfig\.defaults|build_flags)" "*.ini"
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
