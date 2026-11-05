#!/usr/bin/env python3
"""Optional HTTPS weather bridge for a Kindle with an HTTP-only downloader."""
import argparse
import json
import subprocess
import sys
import time
import urllib.parse
import urllib.request


def fetch():
    query = urllib.parse.urlencode({
        "latitude": "42.549329", "longitude": "-71.251724",
        "current": "temperature_2m,apparent_temperature,relative_humidity_2m,wind_speed_10m,weather_code",
        "daily": "temperature_2m_max,temperature_2m_min,weather_code,precipitation_probability_max,wind_speed_10m_max",
        "temperature_unit": "fahrenheit", "wind_speed_unit": "mph",
        "timezone": "America/New_York", "timeformat": "unixtime", "forecast_days": "3",
    })
    request = urllib.request.Request("https://api.open-meteo.com/v1/forecast?" + query,
                                     headers={"User-Agent": "KindleWeatherClock/1.0"})
    with urllib.request.urlopen(request, timeout=25) as response:
        data = json.loads(response.read(100_000))
    if not isinstance(data.get("current"), dict) or data.get("error"):
        raise ValueError("Weather service did not return current conditions")
    return json.dumps(data, ensure_ascii=False, allow_nan=False, separators=(",", ":")).encode()


def push(target, data):
    # All remote paths are constants; location and response data are never shell code.
    command = (
        "umask 022; "
        "cat > /mnt/us/extensions/weather-clock/weather.bridge.new && "
        "/mnt/us/extensions/weather-clock/weather-clock --validate "
        "/mnt/us/extensions/weather-clock/weather.bridge.new && "
        "mv /mnt/us/extensions/weather-clock/weather.bridge.new "
        "/mnt/us/extensions/weather-clock/weather.json"
    )
    subprocess.run(["ssh", "-o", "BatchMode=yes", "-o", "ConnectTimeout=10", "--",
                    target, command], input=data, check=True, timeout=45)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target", default="root@10.0.0.219")
    parser.add_argument("--loop", action="store_true", help="Refresh every 15 minutes until Ctrl+C")
    args = parser.parse_args()
    while True:
        try:
            push(args.target, fetch())
            print("Updated Billerica weather on " + args.target, flush=True)
        except (OSError, ValueError, subprocess.SubprocessError) as exc:
            print("Weather update failed: " + str(exc), file=sys.stderr, flush=True)
            if not args.loop:
                return 1
        if not args.loop:
            return 0
        time.sleep(900)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        pass
