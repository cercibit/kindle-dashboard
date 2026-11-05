#!/bin/sh
# Run by the native app:
#   fetch.sh             -> hourly/manual weather update (when not on server page)
#   fetch.sh --server    -> 5-second Lenovo server telemetry update (only when on server page)
#   fetch.sh --wifi-off  -> turn off Wi-Fi when leaving server page in battery-saving mode
set -eu
if [ "${1:-}" = "--wifi-off" ]; then
    if [ ! -f /tmp/weather-clock-maintenance ]; then
        lipc-set-prop com.lab126.wifid enable 0 2>/dev/null || true
    fi
    exit 0
fi
echo $$ > /tmp/weather-clock-fetch.pid
APP=/mnt/us/extensions/weather-clock
OUT="$APP/weather.https.$$.new"
SERVER_OUT="$APP/server.http.$$.new"
MODE="${1:-}"
cleanup() {
    rm -f "$OUT" "$SERVER_OUT" /tmp/weather-clock-fetch.pid
    if [ "$MODE" != "--server" ] && [ ! -f /tmp/weather-clock-maintenance ]; then
        lipc-set-prop com.lab126.wifid enable 0 2>/dev/null || true
    fi
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM
if [ "$(lipc-get-prop com.lab126.wifid cmState 2>/dev/null)" != CONNECTED ]; then
    lipc-set-prop com.lab126.wifid enable 1
    attempt=0
    while [ "$(lipc-get-prop com.lab126.wifid cmState 2>/dev/null)" != CONNECTED ]; do
        attempt=$((attempt+1))
        [ "$attempt" -lt 40 ] || { echo 'Wi-Fi connection timed out' >&2; exit 1; }
        sleep 1
    done
fi
if [ "$MODE" = "--server" ]; then
    SERVER_URL='http://10.0.0.159:8080/api/specs'
    "$APP/curl-compatible" -4 --http1.1 --fail --silent --show-error --proto '=http' \
        --connect-timeout 3 --max-time 4 --max-filesize 16383 \
        --output "$SERVER_OUT" "$SERVER_URL"
    "$APP/weather-clock" --validate-server "$SERVER_OUT"
    mv "$SERVER_OUT" "$APP/server.json"
    exit 0
fi
URL='https://api.open-meteo.com/v1/forecast?latitude=42.549329&longitude=-71.251724&current=temperature_2m,apparent_temperature,relative_humidity_2m,wind_speed_10m,weather_code&daily=temperature_2m_max,temperature_2m_min,weather_code,precipitation_probability_max,wind_speed_10m_max&temperature_unit=fahrenheit&wind_speed_unit=mph&timezone=America%2FNew_York&timeformat=unixtime&forecast_days=3'
# Resolve using the Kindle libc: modern curl DNS threads need a newer kernel.
IP=$(getent hosts api.open-meteo.com | awk '$1 ~ /^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$/ {print $1; exit}')
[ -n "$IP" ] || { echo 'Weather DNS lookup failed' >&2; exit 1; }
"$APP/curl-compatible" -4 --http1.1 --resolve "api.open-meteo.com:443:$IP" --fail --silent --show-error --proto '=https' \
    --connect-timeout 10 --max-time 35 --max-filesize 16383 \
    --cacert /etc/ssl/certs/ca-certificates.crt --output "$OUT" "$URL"
"$APP/weather-clock" --validate "$OUT"
mv "$OUT" "$APP/weather.json"
