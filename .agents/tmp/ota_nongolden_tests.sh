#!/bin/bash
# Non-golden path OTA test suite
# Run after flashing v0.0.3 with BUILD_VERSION fix + version guard

DEVICE="192.168.20.186"
LOGFILE="$1"
PASS=0
FAIL=0

check() {
    local name="$1" expected="$2" actual="$3"
    if echo "$actual" | grep -q "$expected"; then
        echo "  ✅ PASS: $name"
        PASS=$((PASS+1))
    else
        echo "  ❌ FAIL: $name (expected: $expected, got: $actual)"
        FAIL=$((FAIL+1))
    fi
}

echo "============================================"
echo "  NON-GOLDEN PATH OTA TESTS"
echo "============================================"
echo ""

# --- Test 1: Version match (already up to date) ---
echo "--- TEST 1: Already up-to-date (v0.0.3 == v0.0.3) ---"
curl -s -X POST "http://$DEVICE/api/ota" -H 'Content-Type: application/json' -d '{"action":"checkGithub"}' > /dev/null
sleep 15
STATUS=$(curl -s -X POST "http://$DEVICE/api/ota" -H 'Content-Type: application/json' -d '{"action":"status"}')
echo "  Status: $STATUS"
check "State returns to idle" '"state":"idle"' "$STATUS"
check "Version is 0.0.3" '"version":"0.0.3"' "$STATUS"
echo ""

# --- Test 2: Rollback without confirmation ---
echo "--- TEST 2: Rollback — no confirmation token ---"
RESP=$(curl -s -X POST "http://$DEVICE/api/ota" -H 'Content-Type: application/json' -d '{"action":"rollback"}')
echo "  Response: $RESP"
check "Returns error" '"status":"error"' "$RESP"
check "Missing confirmation msg" 'Missing or invalid confirmation' "$RESP"
echo ""

# --- TEST 3: Rollback — wrong token ---
echo "--- TEST 3: Rollback — wrong confirmation token ---"
RESP=$(curl -s -X POST "http://$DEVICE/api/ota" -H 'Content-Type: application/json' -d '{"action":"rollback","confirm":"wrong"}')
echo "  Response: $RESP"
check "Returns error" '"status":"error"' "$RESP"
echo ""

# --- TEST 4: Invalid action ---
echo "--- TEST 4: Invalid action ---"
RESP=$(curl -s -X POST "http://$DEVICE/api/ota" -H 'Content-Type: application/json' -d '{"action":"foobar"}')
echo "  Response: $RESP"
check "Returns error" '"status":"error"' "$RESP"
check "Invalid action msg" 'Invalid action' "$RESP"
echo ""

# --- TEST 5: Malformed JSON ---
echo "--- TEST 5: Malformed JSON ---"
RESP=$(curl -s -X POST "http://$DEVICE/api/ota" -H 'Content-Type: application/json' -d 'not json')
echo "  Response: $RESP"
check "Returns error" '"status":"error"' "$RESP"
echo ""

# --- TEST 6: Empty body ---
echo "--- TEST 6: Empty POST body ---"
RESP=$(curl -s -X POST "http://$DEVICE/api/ota" -H 'Content-Type: application/json' -d '')
echo "  Response: $RESP"
check "Returns error" '"status":"error"' "$RESP"
echo ""

# --- TEST 7: markValid on blessed firmware ---
echo "--- TEST 7: markValid (already blessed) ---"
RESP=$(curl -s -X POST "http://$DEVICE/api/ota" -H 'Content-Type: application/json' -d '{"action":"markValid"}')
echo "  Response: $RESP"
check "Returns ok" '"status":"ok"' "$RESP"
echo ""

# --- TEST 8: setChannel to beta ---
echo "--- TEST 8: setChannel → beta ---"
RESP=$(curl -s -X POST "http://$DEVICE/api/ota" -H 'Content-Type: application/json' -d '{"action":"setChannel","channel":"beta"}')
echo "  Response: $RESP"
check "Returns ok" '"status":"ok"' "$RESP"
STATUS=$(curl -s -X POST "http://$DEVICE/api/ota" -H 'Content-Type: application/json' -d '{"action":"status"}')
check "Channel is beta" '"channel":"beta"' "$STATUS"
echo ""

# --- TEST 9: setChannel back to stable ---
echo "--- TEST 9: setChannel → stable ---"
curl -s -X POST "http://$DEVICE/api/ota" -H 'Content-Type: application/json' -d '{"action":"setChannel","channel":"stable"}' > /dev/null
STATUS=$(curl -s -X POST "http://$DEVICE/api/ota" -H 'Content-Type: application/json' -d '{"action":"status"}')
check "Channel is stable" '"channel":"stable"' "$STATUS"
echo ""

# --- TEST 10: setChannel with empty string ---
echo "--- TEST 10: setChannel — empty string ---"
RESP=$(curl -s -X POST "http://$DEVICE/api/ota" -H 'Content-Type: application/json' -d '{"action":"setChannel","channel":""}')
echo "  Response: $RESP"
check "Returns error" '"status":"error"' "$RESP"
check "Invalid channel msg" 'Invalid channel' "$RESP"
echo ""

# --- TEST 11: Version consistency ---
echo "--- TEST 11: Version consistency (system vs OTA) ---"
SYS_VER=$(curl -s "http://$DEVICE/api/system" | python3 -c "import json,sys; print(json.load(sys.stdin)['firmware_version'])" 2>/dev/null)
OTA_VER=$(curl -s -X POST "http://$DEVICE/api/ota" -H 'Content-Type: application/json' -d '{"action":"status"}' | python3 -c "import json,sys; print(json.load(sys.stdin)['version'])" 2>/dev/null)
echo "  System API: $SYS_VER"
echo "  OTA Status: $OTA_VER"
if [ "$SYS_VER" = "$OTA_VER" ]; then
    echo "  ✅ PASS: Versions match"
    PASS=$((PASS+1))
else
    echo "  ❌ FAIL: Version mismatch ($SYS_VER vs $OTA_VER)"
    FAIL=$((FAIL+1))
fi
echo ""

# --- TEST 12: GET /api/ota (basic health) ---
echo "--- TEST 12: GET /api/ota ---"
RESP=$(curl -s "http://$DEVICE/api/ota")
echo "  Response: $RESP"
check "Returns ready status" '"status":"ready"' "$RESP"
echo ""

# --- Serial log check ---
if [ -n "$LOGFILE" ] && [ -f "$LOGFILE" ]; then
    echo "--- SERIAL LOG CHECK ---"
    ALREADY=$(grep -c "Already up to date" "$LOGFILE" 2>/dev/null)
    echo "  'Already up to date' log entries: $ALREADY"
    check "Version guard triggered" "1" "$([ "$ALREADY" -ge 1 ] && echo '1' || echo '0')"
    echo ""
fi

echo "============================================"
echo "  RESULTS: $PASS passed, $FAIL failed"
echo "============================================"
