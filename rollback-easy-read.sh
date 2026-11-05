#!/bin/sh
# Restore the exact on-device backup, retaining that archive for future use.
set -eu
TARGET=${1:-kindle-weather}
ssh -o BatchMode=yes -o ConnectTimeout=10 "$TARGET" '
    test -s /mnt/us/weather-clock-before-easy-read.tar.gz
    if test -d /tmp/kindle-weather-clock.lock || test -d /tmp/kindle-calculator.lock; then
        echo "Press X to exit the active app before rollback." >&2
        exit 1
    fi
    tar -xzf /mnt/us/weather-clock-before-easy-read.tar.gz -C /mnt/us/extensions
    sync
    md5sum /mnt/us/extensions/weather-clock/weather-clock
    sh /mnt/us/extensions/weather-clock/launch.sh
'
