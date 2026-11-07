#!/bin/sh
# Fixed arguments only; no user-supplied command or URL is evaluated.
set -eu
APP=/mnt/us/extensions/weather-clock
case "${1:-weather}" in
    status)
        lipc-get-prop com.lab126.wifid cmState 2>/dev/null || echo UNKNOWN
        gasgauge-info -c 2>/dev/null || echo -1
        gasgauge-info -l 2>/dev/null || echo 0
        gasgauge-info -m 2>/dev/null || echo 0
        exit 0 ;;
    awake)
        lipc-set-prop com.lab126.wifid enable 1
        lipc-set-prop com.lab126.powerd preventScreenSaver 1
        exit 0 ;;
    sleep) lipc-set-prop com.lab126.powerd preventScreenSaver 0; exit 0 ;;
    weather|server) MODE=${1:-weather} ;;
    *) exit 2 ;;
esac
exec >>"$APP/weather-kindlet.log" 2>&1
OUT="$APP/kindlet-$MODE.$$.new"
trap 'rm -f "$OUT"' EXIT
trap 'exit 1' HUP INT TERM
if [ "$MODE" = server ]; then
    "$APP/curl-compatible" -4 --http1.1 --fail --silent --show-error --proto '=http' \
        --connect-timeout 3 --max-time 4 --max-filesize 16383 --output "$OUT" \
        'http://10.0.0.159:8080/api/specs'
    "$APP/weather-clock" --validate-server "$OUT"
    mv "$OUT" "$APP/server.json"
else
    URL='https://api.open-meteo.com/v1/forecast?latitude=42.549329&longitude=-71.251724&current=temperature_2m,apparent_temperature,relative_humidity_2m,wind_speed_10m,weather_code&daily=temperature_2m_max,temperature_2m_min,weather_code,precipitation_probability_max,wind_speed_10m_max&temperature_unit=fahrenheit&wind_speed_unit=mph&timezone=America%2FNew_York&timeformat=unixtime&forecast_days=3'
    IP=$(getent hosts api.open-meteo.com | awk '$1 ~ /^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$/ {print $1; exit}')
    [ -n "$IP" ] || { echo 'Weather DNS lookup failed' >&2; exit 1; }
    "$APP/curl-compatible" -4 --http1.1 --resolve "api.open-meteo.com:443:$IP" --fail --silent --show-error --proto '=https' \
        --connect-timeout 10 --max-time 35 --max-filesize 16383 \
        --cacert /etc/ssl/certs/ca-certificates.crt --output "$OUT" "$URL"
    "$APP/weather-clock" --validate "$OUT"
    mv "$OUT" "$APP/weather.json"
fi
