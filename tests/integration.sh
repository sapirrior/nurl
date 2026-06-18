#!/bin/bash
set -e

NURL=${NURL:-../nurl}
# Use postman-echo.com as httpbin.org is currently flaky
HTTPBIN="https://postman-echo.com"

# ANSI colors
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

fail() {
    echo -e "${RED}FAIL: $1${NC}"
    exit 1
}

pass() {
    echo -e "${GREEN}PASS: $1${NC}"
}

echo "Running enhanced integration tests..."

# 1. Test GET
echo -n "Test 1: GET... "
status=$(${NURL} "${HTTPBIN}/get" -w '%{http_code}' -o /dev/null -s)
[ "$status" = "200" ] || fail "GET expected 200, got $status"
pass "GET"

# 2. Test POST JSON
echo -n "Test 2: POST JSON... "
status=$(${NURL} "${HTTPBIN}/post" -j -d '{"test":"nurl"}' -w '%{http_code}' -o /dev/null -s)
[ "$status" = "200" ] || fail "POST JSON expected 200, got $status"
pass "POST JSON"

# 3. Test PUT / DELETE / PATCH
echo -n "Test 3: PUT/DELETE/PATCH... "
for method in PUT DELETE PATCH; do
    m_lower=$(echo $method | tr '[:upper:]' '[:lower:]')
    status=$(${NURL} "${HTTPBIN}/${m_lower}" -X $method -w '%{http_code}' -o /dev/null -s)
    [ "$status" = "200" ] || fail "$method expected 200, got $status"
done
pass "PUT/DELETE/PATCH"

# 4. Test Custom Headers
echo -n "Test 4: Custom Headers... "
output=$(${NURL} "${HTTPBIN}/headers" -H "X-Nurl-Test: enhanced-test" -s)
echo "$output" | grep -q "enhanced-test" || fail "Custom Header not found in response"
pass "Custom Headers"

# 5. Test Redirects
echo -n "Test 5: Redirects... "
# postman-echo.com/redirect-to?url=https://postman-echo.com/get
status=$(${NURL} "${HTTPBIN}/redirect-to?url=https%3A%2F%2Fpostman-echo.com%2Fget" -L -w '%{http_code}' -o /dev/null -s)
[ "$status" = "200" ] || fail "Redirect (Follow) expected 200, got $status"

# Test max-redirects
set +e
# This will redirect once. If we set max-redirs 0, it should fail if it tries to follow.
# Wait, max-redirs 0 means no redirects allowed.
${NURL} "${HTTPBIN}/redirect-to?url=https%3A%2F%2Fpostman-echo.com%2Fget" -L --max-redirs 0 -s > /dev/null 2>&1
ret=$?
set -e
[ $ret -ne 0 ] || fail "Max-redirs 0 should have failed"
pass "Redirects & Max-redirs"

# 6. Test Authentication
echo -n "Test 6: Authentication... "
# Basic Auth (postman-echo uses 'postman:password')
status=$(${NURL} "${HTTPBIN}/basic-auth" -u postman:password -w '%{http_code}' -o /dev/null -s)
[ "$status" = "200" ] || fail "Basic Auth expected 200, got $status"
pass "Authentication (Basic)"

# 7. Test Cookies
echo -n "Test 7: Cookies... "
cookie_file="nurl_cookies.txt"
rm -f "$cookie_file"
# Set a cookie
${NURL} "${HTTPBIN}/cookies/set?nurl=rock-solid" -c "$cookie_file" -o /dev/null -s
# Send it back (using -b to read from file)
output=$(${NURL} "${HTTPBIN}/cookies" -b "@$cookie_file" -s)
echo "$output" | grep -q "rock-solid" || fail "Cookie not found in jar"
rm -f "$cookie_file"
pass "Cookies & Cookie Jars"

# 8. Test TLS verify
echo -n "Test 8: TLS Verify... "
set +e
output=$(${NURL} https://expired.badssl.com/ -v 2>&1)
set -e
if echo "$output" | grep -iqE "TLS|certificate|failed|error"; then
    pass "TLS Verification"
else
    echo "Output was: $output"
    fail "TLS Verification"
fi

# 9. Test --fail flag
echo -n "Test 9: Fail flag... "
set +e
${NURL} "${HTTPBIN}/status/404" -f -s > /dev/null 2>&1
ret=$?
set -e
[ $ret -ne 0 ] || fail "FAIL_FLAG expected non-zero exit for 404, got $ret"
pass "Fail-on-error flag"

# 10. Test User-Agent
echo -n "Test 10: User-Agent... "
output=$(${NURL} "${HTTPBIN}/headers" -A "nurl-test-agent" -s)
echo "$output" | grep -q "nurl-test-agent" || fail "User-Agent not sent correctly"
pass "User-Agent Customization"

echo -e "\n${GREEN}All enhanced integration tests passed successfully!${NC}"


