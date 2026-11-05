#!/bin/sh
# Compatible with the Kindle's BusyBox 1.7 shell and HTTP-only wget.
# The app enforces the 50-second network timeout and refresh interval.
set -eu
APP=/mnt/us/extensions/weather-clock
RAW=/tmp/kindle-weather-raw.$$
OUT=/tmp/kindle-weather-json.$$
trap 'rm -f "$RAW" "$OUT"' EXIT
trap 'exit 1' HUP INT TERM
URL='http://wttr.in/42.549329,-71.251724?u&lang=en&format=%t%0A%f%0A%h%0A%w%0A%C'
wget -q -O "$RAW" "$URL"
STAMP=$(date +%s)
LC_ALL=C awk -v stamp="$STAMP" '
function number(s) {
    gsub(/[^0-9.+-]/,"",s)
    sub(/^\+/,"",s)
    if (s !~ /^-?[0-9]+([.][0-9]+)?$/) exit 1
    return s+0
}
NR==1 {temp=number($0); if (temp < -150 || temp > 160) exit 1}
NR==2 {feels=number($0); if (feels < -200 || feels > 200) exit 1}
NR==3 {humidity=number($0); if (humidity < 0 || humidity > 100) exit 1}
NR==4 {wind=number($0); if (wind < 0 || wind > 300) exit 1}
NR==5 {condition=$0; gsub(/[^A-Za-z ]/,"",condition); if (length(condition)<2 || length(condition)>60) exit 1}
END {
    if (NR != 5 || temp=="" || feels=="" || humidity=="" || wind=="" || condition=="") exit 1
    printf "{\"current_units\":{\"temperature_2m\":\"F\",\"wind_speed_10m\":\"mp/h\"},\"current\":{\"time\":%s,\"temperature_2m\":%s,\"apparent_temperature\":%s,\"relative_humidity_2m\":%s,\"wind_speed_10m\":%s,\"condition\":\"%s\"}}\n",stamp,temp,feels,humidity,wind,condition
}' "$RAW" > "$OUT"
"$APP/weather-clock" --validate "$OUT"
cp "$OUT" "$APP/weather.json.new"
mv "$APP/weather.json.new" "$APP/weather.json"
