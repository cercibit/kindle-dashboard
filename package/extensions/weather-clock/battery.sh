#!/bin/sh
# Read the stock gauge once every five minutes; publish a complete sample atomically.
set -eu
OUT=/tmp/weather-clock-battery.$$.new
trap 'rm -f "$OUT"' EXIT
{
    gasgauge-info -c 2>/dev/null
    gasgauge-info -l 2>/dev/null
    gasgauge-info -m 2>/dev/null
} > "$OUT"
mv "$OUT" /tmp/weather-clock-battery
