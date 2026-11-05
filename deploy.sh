#!/bin/sh
set -eu
cd "$(dirname "$0")"
TARGET=${1:-root@10.0.0.219}
ssh -o BatchMode=yes -o ConnectTimeout=10 -- "$TARGET" '
    [ "$(id -u)" = 0 ] || exit 1
    if [ -d /tmp/kindle-calculator.lock ] || [ -d /tmp/kindle-weather-clock.lock ]; then
        echo "Exit the calculator or weather clock with X before deploying." >&2
        exit 1
    fi
    mkdir -p /mnt/us/extensions/weather-clock
'
tar -cf weather-clock-install.tar -C package/extensions/weather-clock .
ssh -o BatchMode=yes -o ConnectTimeout=10 -- "$TARGET" '
    tar -xf - -C /mnt/us/extensions/weather-clock &&
    chmod +x /mnt/us/extensions/weather-clock/weather-clock /mnt/us/extensions/weather-clock/launch.sh /mnt/us/extensions/weather-clock/fetch.sh
' < weather-clock-install.tar
if ! ssh -o BatchMode=yes -o ConnectTimeout=10 -- "$TARGET" 'sh /mnt/us/extensions/weather-clock/fetch.sh'; then
    echo 'Initial weather update failed; the app will retry while open.' >&2
fi
ssh -o BatchMode=yes -o ConnectTimeout=10 -- "$TARGET" '
    sync
    sh /mnt/us/extensions/weather-clock/launch.sh
'
echo 'Weather Clock installed and launch requested. Check the Kindle screen.'
echo 'Press X to exit; press R to refresh weather.'
