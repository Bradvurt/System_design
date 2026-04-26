#!/usr/bin/env bash
set -euo pipefail

BASE_URL="${BASE_URL:-http://127.0.0.1:8080}"

die() {
  echo "FAIL: $*" >&2
  exit 1
}

expect_status() {
  local got="$1"
  local want="$2"
  local ctx="$3"
  [[ "$got" == "$want" ]] || die "$ctx: ожидался HTTP $want, получен $got"
}

json_token() {
  python3 -c 'import json,sys; print(json.load(sys.stdin)["token"])'
}

json_field() {
  python3 -c "import json,sys; print(json.load(sys.stdin)[\"$1\"])"
}

echo "== Smoke test against $BASE_URL"

LOGIN="smoke_$(date +%s)_$$"
code=$(curl -sS -o /tmp/smoke_reg.json -w '%{http_code}' -X POST "$BASE_URL/api/v1/auth/register" \
  -H 'Content-Type: application/json' \
  -d "{\"login\":\"$LOGIN\",\"name\":\"Smoke\",\"surname\":\"Test\",\"email\":\"${LOGIN}@test.local\",\"password\":\"pw\"}")
expect_status "$code" 201 "register"
TOKEN=$(json_token < /tmp/smoke_reg.json)
USER_ID=$(python3 -c 'import json; print(json.load(open("/tmp/smoke_reg.json"))["user"]["id"])')

code=$(curl -sS -o /dev/null -w '%{http_code}' -X POST "$BASE_URL/api/v1/hotels" \
  -H 'Content-Type: application/json' \
  -d '{"name":"X","city":"Y","address":"Z"}')
expect_status "$code" 401 "create hotel without auth"

code=$(curl -sS -o /tmp/smoke_hotel.json -w '%{http_code}' -X POST "$BASE_URL/api/v1/hotels" \
  -H "Authorization: Bearer $TOKEN" \
  -H 'Content-Type: application/json' \
  -d '{"name":"Hotel A","city":"Moscow","address":"Street 1"}')
expect_status "$code" 201 "create hotel with auth"
HOTEL_ID=$(json_field id < /tmp/smoke_hotel.json)

code=$(curl -sS -o /tmp/smoke_hotels.json -w '%{http_code}' "$BASE_URL/api/v1/hotels")
expect_status "$code" 200 "list hotels"
code=$(curl -sS -o /tmp/smoke_city.json -w '%{http_code}' "$BASE_URL/api/v1/hotels?city=moscow")
expect_status "$code" 200 "hotels by city"
python3 -c 'import json,sys; d=json.load(open("/tmp/smoke_city.json")); assert len(d)==1, d'

code=$(curl -sS -o /dev/null -w '%{http_code}' "$BASE_URL/api/v1/users/by-login/$LOGIN")
expect_status "$code" 200 "user by login"

code=$(curl -sS -o /dev/null -w '%{http_code}' "$BASE_URL/api/v1/users/by-login/nobody_$(date +%s)")
expect_status "$code" 404 "user by login missing"

code=$(curl -sS -o /dev/null -w '%{http_code}' "$BASE_URL/api/v1/users/search")
expect_status "$code" 400 "search without params"

code=$(curl -sS -o /tmp/smoke_search.json -w '%{http_code}' "$BASE_URL/api/v1/users/search?name=Smo")
expect_status "$code" 200 "user search"
python3 -c 'import json; a=json.load(open("/tmp/smoke_search.json")); assert len(a)>=1'

code=$(curl -sS -o /tmp/smoke_booking.json -w '%{http_code}' -X POST "$BASE_URL/api/v1/bookings" \
  -H "Authorization: Bearer $TOKEN" \
  -H 'Content-Type: application/json' \
  -d "{\"hotel_id\":$HOTEL_ID,\"check_in\":\"2026-06-01\",\"check_out\":\"2026-06-05\"}")
expect_status "$code" 201 "create booking"
BOOKING_ID=$(json_field id < /tmp/smoke_booking.json)

code=$(curl -sS -o /tmp/smoke_bookings_list.json -w '%{http_code}' \
  "$BASE_URL/api/v1/users/$USER_ID/bookings" \
  -H "Authorization: Bearer $TOKEN")
expect_status "$code" 200 "list bookings"
python3 -c 'import json; a=json.load(open("/tmp/smoke_bookings_list.json")); assert len(a)==1'

code=$(curl -sS -o /dev/null -w '%{http_code}' -X DELETE "$BASE_URL/api/v1/bookings/$BOOKING_ID" \
  -H "Authorization: Bearer $TOKEN")
expect_status "$code" 204 "cancel booking"

code=$(curl -sS -o /dev/null -w '%{http_code}' -X DELETE "$BASE_URL/api/v1/bookings/$BOOKING_ID" \
  -H "Authorization: Bearer $TOKEN")
expect_status "$code" 409 "cancel again"

code=$(curl -sS -o /dev/null -w '%{http_code}' -X POST "$BASE_URL/api/v1/auth/login" \
  -H 'Content-Type: application/json' \
  -d "{\"login\":\"$LOGIN\",\"password\":\"wrong\"}")
expect_status "$code" 401 "login wrong password"

code=$(curl -sS -o /dev/null -w '%{http_code}' -X POST "$BASE_URL/api/v1/auth/login" \
  -H 'Content-Type: application/json' \
  -d "{\"login\":\"$LOGIN\",\"password\":\"pw\"}")
expect_status "$code" 200 "login ok"

echo "OK: все проверки прошли"
