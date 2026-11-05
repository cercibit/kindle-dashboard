#!/bin/sh
# Let the launcher close before taking control of the display and keyboard.
APP=/mnt/us/extensions/weather-clock
if [ "${1:-}" != "--run" ]; then
    sh "$APP/launch.sh" --run </dev/null >>"$APP/weather-clock.log" 2>&1 &
    exit 0
fi
[ ! -d /tmp/kindle-calculator.lock ] || exit 1
mkdir /tmp/kindle-weather-clock.lock 2>/dev/null || exit 0
FRAMEWORK_PIDS=""
POWER_LOCK=0
WIFI_INITIAL=$(lipc-get-prop com.lab126.wifid enable 2>/dev/null)
APP_PID=""
cleanup() {
    trap - EXIT HUP INT TERM
    if [ -n "$APP_PID" ]; then kill -TERM "$APP_PID" 2>/dev/null; wait "$APP_PID" 2>/dev/null; fi
    if [ -f /tmp/weather-clock-fetch.pid ]; then
        fetch_pid=$(cat /tmp/weather-clock-fetch.pid)
        case "$fetch_pid" in ''|*[!0-9]*) ;; *) kill -TERM -"$fetch_pid" 2>/dev/null; sleep 2; kill -KILL -"$fetch_pid" 2>/dev/null ;; esac
        rm -f /tmp/weather-clock-fetch.pid
    fi
    echo 0 > /sys/devices/platform/mxc_rtc.0/wakeup_enable
    # Restore the user's radio setting even after an interrupted fetch.
    [ -z "$WIFI_INITIAL" ] || lipc-set-prop com.lab126.wifid enable "$WIFI_INITIAL" 2>/dev/null
    rm -f /tmp/weather-clock-maintenance
    for task_pid in $FRAMEWORK_PIDS; do kill -CONT "$task_pid" 2>/dev/null; done
    if [ "$POWER_LOCK" = 1 ]; then
        lipc-set-prop com.lab126.powerd preventScreenSaver 0 2>/dev/null
    fi
    eips -c 2>/dev/null
    lipc-set-prop com.lab126.appmgrd start app://com.lab126.booklet.home 2>/dev/null
    rmdir /tmp/kindle-weather-clock.lock 2>/dev/null
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM
sleep 2
if [ ! -x "$APP/weather-clock" ]; then
    echo "Weather Clock executable missing or USB storage mounted noexec"
    exit 1
fi
FRAMEWORK_PIDS=$(pidof cvm)
if [ -z "$FRAMEWORK_PIDS" ]; then
    echo "Cannot identify Kindle framework process; leaving UI alone"
    exit 1
fi
if lipc-set-prop com.lab126.powerd preventScreenSaver 1 2>/dev/null; then POWER_LOCK=1; fi
for task_pid in $FRAMEWORK_PIDS; do kill -STOP "$task_pid" || exit 1; done
"$APP/weather-clock" &
APP_PID=$!
wait "$APP_PID"
RESULT=$?
APP_PID=""
exit "$RESULT"
