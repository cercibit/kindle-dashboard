#!/usr/bin/env python3
import copy
import ctypes
import datetime as dt
import json
from pathlib import Path
import random
import subprocess
import tempfile
from zoneinfo import ZoneInfo

ROOT = Path(__file__).resolve().parent
SAMPLE = {
    "current_units": {"temperature_2m": "°F", "wind_speed_10m": "mp/h"},
    "current": {
        "time": 1793644200, "temperature_2m": 54.2,
        "apparent_temperature": 51.6, "relative_humidity_2m": 62,
        "wind_speed_10m": 8.4, "weather_code": 2,
    },
    "daily": {"temperature_2m_max": [57.1], "temperature_2m_min": [39.2]},
}
SERVER_SAMPLE = {
    "ok": True,
    "latency_ms": 165,
    "timestamp": "2026-11-05 08:51:41 EST",
    "system": {
        "hostname": "mehmet-lenovo",
        "uptime_human": "39d 6h 19m",
        "load_avg": [0.19, 0.14, 0.15],
    },
    "cpu": {
        "threads": 8,
        "usage_pct": 14.6,
        "temp_c": 61.0,
        "avg_freq_mhz": 1242,
    },
    "memory": {
        "total_bytes": 8266452992,
        "used_bytes": 2080858112,
        "used_pct": 25.2,
        "swap_used_pct": 8.5,
    },
    "gpu": {
        "name": "NVIDIA GeForce GT 750M",
        "temp_c": 57,
        "mem_used_mib": 4,
        "mem_total_mib": 1995,
    },
    "storage": {
        "mounts": [
            {
                "mountpoint": "/",
                "size_bytes": 226788651008,
                "used_bytes": 174675681280,
                "avail_bytes": 40565948416,
                "use_pct": 82,
            },
            {
                "mountpoint": "/boot/efi",
                "size_bytes": 100663296,
                "used_bytes": 33998848,
                "avail_bytes": 66664448,
                "use_pct": 34,
            },
            {
                "mountpoint": "/media/mehmet/Lenovo_HDD",
                "size_bytes": 983354093568,
                "used_bytes": 307850657792,
                "avail_bytes": 630720745472,
                "use_pct": 33,
            },
        ]
    },
    "processes": [
        {"pid": 85706, "user": "server", "cpu_pct": 3.7, "mem_pct": 0.5, "command": "sshd"},
        {"pid": 3662, "user": "server", "cpu_pct": 3.6, "mem_pct": 0.2, "command": "python3"},
        {"pid": 663588, "user": "server", "cpu_pct": 2.7, "mem_pct": 5.4, "command": "syncthing"},
        {"pid": 2088, "user": "mysql", "cpu_pct": 0.8, "mem_pct": 1.1, "command": "mysqld"},
        {"pid": 943724, "user": "server", "cpu_pct": 0.2, "mem_pct": 2.8, "command": "containerd"},
    ],
}


def main():
    with tempfile.TemporaryDirectory(prefix="kindle-weather-test-", dir="/tmp") as folder:
        temp = Path(folder)
        wrapper = temp / "wrapper.c"
        wrapper.write_text('''#include <string.h>
#include "weather.h"
#include "server.h"
#include "navigation.h"
#include "quotes.h"
#include "battery.h"
#include "power.h"
int test_idle(int stamp,int page,int battery,int fetch){return idle_seconds(stamp,page,battery,fetch);}
int test_elapsed(void){Battery b={0};battery_sample(&b,"30%\\n-100 mA\\n400 mAh\\n");battery_elapsed(&b,1000);battery_sample(&b,"29%\\n-100 mA\\n390 mAh\\n");battery_elapsed(&b,2800);return b.minutes;}
int test_battery(const char *s,int *out){Battery b={0};if(!battery_sample(&b,s))return 0;out[0]=b.percent;out[1]=b.charging;out[2]=b.minutes;return 1;}
int test_quote(int stamp){return quote_index(eastern(stamp));}
int test_page(int page,int key){return turn_page(page,key);}
int test_forecast(const char *s,int *out){Weather w;if(!read_weather(s,&w))return -1;for(int i=0;i<w.forecast_count;i++){out[i*2]=w.forecast[i].stamp;out[i*2+1]=w.forecast[i].code;}return w.forecast_count;}
int test_parse(const char *s){Weather w;return read_weather(s,&w);}
int test_server(const char *s,int *out){Server sv;if(!read_server(s,&sv))return 0;out[0]=sv.cpu_pct;out[1]=sv.mem_pct;out[2]=sv.cpu_temp_pct;out[3]=sv.gpu_temp_pct;out[4]=sv.disk_count;out[5]=sv.proc_count;return 1;}
void test_clock(int stamp,int *out){Date d=eastern(stamp);out[0]=d.year;out[1]=d.month;out[2]=d.day;out[3]=d.hour;out[4]=d.minute;out[5]=d.weekday;out[6]=d.dst;}
''')
        subprocess.run(["cc", "-shared", "-fPIC", "-O2", "-I", str(ROOT / "src"),
                        str(wrapper), "-o", str(temp / "tests.so")], check=True)
        lib = ctypes.CDLL(str(temp / "tests.so"))
        assert lib.test_elapsed() == 1170
        assert lib.test_idle(3601,0,3600,3600) == 59
        assert lib.test_idle(3601,3,3600,3600) == 299
        assert lib.test_idle(3601,5,3600,3600) == 59
        assert lib.test_idle(3890,3,3600,3600) == 10
        assert lib.test_idle(7190,3,7100,3600) == 10
        lib.test_battery.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_int)]
        for sample, expected in [(b"30%\n-40 mA\n400 mAh\n", [30,0,600]),
                                 (b"80%\n150 mA\n1000 mAh\n", [80,1,-1]),
                                 (b"100%\n0 mA\n1300 mAh\n", [100,0,-1]),
                                 (b"0%\n-50 mA\n0 mAh\n", [0,0,0])]:
            values = (ctypes.c_int * 3)()
            assert lib.test_battery(sample, values) == 1
            assert list(values) == expected
        for sample in (b"", b"30%\n", b"101%\n-40 mA\n400 mAh\n", b"30%\n-40 mA\n-1 mAh\n"):
            assert lib.test_battery(sample, (ctypes.c_int * 3)()) == 0
        lib.test_parse.argtypes = [ctypes.c_char_p]
        lib.test_server.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_int)]
        lib.test_clock.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_int)]
        lib.test_page.argtypes = [ctypes.c_int, ctypes.c_int]
        for key, step in [(104,1),(191,1),(109,-1),(193,-1),(19,0),(30,0)]:
            for page in range(6):
                assert lib.test_page(page,key) == (page+step)%6
        srv_out = (ctypes.c_int * 6)()
        assert lib.test_server(json.dumps(SERVER_SAMPLE).encode(), srv_out) == 1
        assert list(srv_out) == [15, 25, 61, 57, 2, 5]
        for bad_srv in [b"", b"{}", b'{"ok":false}', b'{"ok":true,"system":{}}']:
            assert lib.test_server(bad_srv, srv_out) == 0
        # Quote stays fixed through a local day, including both DST transitions.
        zone = ZoneInfo("America/New_York")
        for date in (dt.date(2026, 3, 8), dt.date(2026, 11, 1), dt.date(2026, 12, 31)):
            stamps = [int(dt.datetime.combine(date, dt.time(hour), zone).timestamp()) for hour in (0, 6, 12, 23)]
            indices = [lib.test_quote(stamp) for stamp in stamps]
            assert len(set(indices)) == 1
            tomorrow = int(dt.datetime.combine(date + dt.timedelta(days=1), dt.time(), zone).timestamp())
            assert lib.test_quote(tomorrow) == (indices[0] + 1) % 31
        lib.test_forecast.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_int)]
        forecast = copy.deepcopy(SAMPLE)
        forecast["daily"] = {"time":[1793595600,1793682000,1793768400],
            "temperature_2m_max":[60.0,55.0,48.0], "temperature_2m_min":[40.0,35.0,28.0],
            "weather_code":[0,61,71], "precipitation_probability_max":[0,100,70],
            "wind_speed_10m_max":[5.0,12.0,18.0]}
        out = (ctypes.c_int * 6)()
        assert lib.test_forecast(json.dumps(forecast, ensure_ascii=False).encode(),out) == 3
        assert list(out) == [1793595600,0,1793682000,61,1793768400,71]
        for field in forecast["daily"]:
            partial = copy.deepcopy(forecast)
            partial["daily"][field] = partial["daily"][field][:1]
            assert lib.test_forecast(json.dumps(partial, ensure_ascii=False).encode(),out) == 1
            partial["daily"][field] = None
            assert lib.test_forecast(json.dumps(partial, ensure_ascii=False).encode(),out) == 0
        for bad in [None, -1, 101, "rain"]:
            partial = copy.deepcopy(forecast)
            partial["daily"]["precipitation_probability_max"][1] = bad
            assert lib.test_forecast(json.dumps(partial, ensure_ascii=False).encode(),out) == 1
        fixture = temp / "forecast.json"
        fixture.write_text(json.dumps(forecast, ensure_ascii=False))
        srv_fixture = temp / "server.json"
        srv_fixture.write_text(json.dumps(SERVER_SAMPLE, ensure_ascii=False))
        val_srv = subprocess.run(["qemu-arm", str(ROOT / "package/extensions/weather-clock/weather-clock"),
                                  "--validate-server", str(srv_fixture)], capture_output=True)
        assert val_srv.returncode == 0
        rendered = []
        for page in range(6):
            result = subprocess.run(["qemu-arm",str(ROOT / "package/extensions/weather-clock/weather-clock"),
                "--preview",str(fixture),"1793644200",str(page),"30%\n-40 mA\n400 mAh\n","2","1","0",str(srv_fixture)], capture_output=True,check=True)
            assert len(result.stdout) == 240000
            rendered.append(result.stdout)
        assert len(set(rendered)) == 6, "All six views including Lenovo Server must be distinct"
        # Minute changes must update the new dial, not only the digital fallback.
        next_minute = subprocess.run(["qemu-arm",str(ROOT / "package/extensions/weather-clock/weather-clock"),
            "--preview",str(fixture),"1793644260","4","30%\n-40 mA\n400 mAh\n","2","1","0",str(srv_fixture)],capture_output=True,check=True).stdout
        assert next_minute[160*300:650*300] != rendered[4][160*300:650*300]
        # Distinguish fetch-only Wi-Fi from explicitly enabled SSH maintenance.
        statuses = []
        for wifi, ssh in [(0,0),(1,1),(2,1),(2,0),(3,0)]:
            result = subprocess.run(["qemu-arm",str(ROOT / "package/extensions/weather-clock/weather-clock"),
                "--preview",str(fixture),"1793644200","3","30%\n-40 mA\n400 mAh\n",str(wifi),str(ssh)],
                capture_output=True,check=True)
            assert len(result.stdout) == 240000
            statuses.append(result.stdout[:80*300])
        assert len(set(statuses)) == 5
        sleeping = subprocess.run(["qemu-arm",str(ROOT / "package/extensions/weather-clock/weather-clock"),
            "--preview",str(fixture),"1793644200","3","30%\n-40 mA\n400 mAh\n","0","0","1"],
            capture_output=True,check=True).stdout
        assert len(sleeping) == 240000
        assert sleeping[:80*300] != statuses[0]
        assert sleeping[775*300:] != rendered[3][775*300:]


        # Old cached forecasts still render safely with the missing-forecast message.
        old = temp / "old.json"
        old.write_text(json.dumps(SAMPLE, ensure_ascii=False))
        result = subprocess.run(["qemu-arm",str(ROOT / "package/extensions/weather-clock/weather-clock"),
            "--preview",str(old),"1793644200","2"],capture_output=True,check=True)
        assert len(result.stdout) == 240000
        print("Passed six-page wrapping, forecast & server parsing, six distinct ARM screen renders, and Easy Read minute updates")
        zone = ZoneInfo("America/New_York")
        stamps = []
        # Every hour around both DST transitions, each supported year.
        for year in range(2020, 2038):
            for month in (3, 11):
                base = dt.datetime(year, month, 1, tzinfo=dt.timezone.utc)
                stamps.extend(int((base + dt.timedelta(hours=h)).timestamp()) for h in range(15 * 24))
        random.seed(41)
        stamps.extend(random.randrange(1577836800, 2145916800) for _ in range(1000))
        for stamp in stamps:
            actual = (ctypes.c_int * 7)()
            lib.test_clock(stamp, actual)
            expected = dt.datetime.fromtimestamp(stamp, zone)
            want = [expected.year, expected.month, expected.day, expected.hour, expected.minute,
                    (expected.weekday() + 1) % 7, int(bool(expected.dst()))]
            assert list(actual) == want, (stamp, list(actual), want)
        valid = [json.dumps(SAMPLE, ensure_ascii=False)]
        wttr = copy.deepcopy(SAMPLE)
        wttr.pop("daily")
        wttr["current_units"]["temperature_2m"] = "F"
        wttr["current"].pop("weather_code")
        wttr["current"]["condition"] = "Partly cloudy"
        valid.append(json.dumps(wttr))
        invalid = ["", "{}", "<html>service unavailable</html>", "{", valid[0][:-2], "[1,2,3]"]
        for key in SAMPLE["current"]:
            if key == "weather_code":
                continue
            sample = copy.deepcopy(SAMPLE)
            sample["current"][key] = None
            invalid.append(json.dumps(sample, ensure_ascii=False))
        for stamp in (-1, 9999999999, 2147483648):
            sample = copy.deepcopy(SAMPLE)
            sample["current"]["time"] = stamp
            invalid.append(json.dumps(sample, ensure_ascii=False))
        sample = copy.deepcopy(SAMPLE)
        sample["current_units"]["temperature_2m"] = "°C"
        invalid.append(json.dumps(sample, ensure_ascii=False))
        for index, (text, expected) in enumerate([(x, True) for x in valid] + [(x, False) for x in invalid]):
            assert bool(lib.test_parse(text.encode())) == expected, text
            fixture = temp / f"fixture-{index}.json"
            fixture.write_text(text)
            result = subprocess.run(["qemu-arm", str(ROOT / "package/extensions/weather-clock/weather-clock"),
                                     "--validate", str(fixture)], capture_output=True)
            assert (result.returncode == 0) == expected, (text, result.stderr)
        fetch_script = (ROOT / "package/extensions/weather-clock/fetch-http-legacy.sh").read_text()
        awk = fetch_script.split('LC_ALL=C awk -v stamp="$STAMP" \'\n', 1)[1].split('\' "$RAW"', 1)[0]
        conversions = [
            ("+54°F\n+51°F\n62%\n↗8mph\nPartly cloudy\n", True),
            ("-4°F\n-12°F\n0%\n↓0mph\nClear\n", True),
            ("<html>HTTPS required</html>\n", False),
            ("+54°F\n+51°F\n200%\n8mph\nRain\n", False),
        ]
        for index, (raw, expected) in enumerate(conversions):
            converted = subprocess.run(["awk", "-v", "stamp=1793644200", awk],
                                       input=raw, text=True, capture_output=True)
            assert (converted.returncode == 0) == expected, (raw, converted.stderr)
            if expected:
                fixture = temp / f"http-{index}.json"
                fixture.write_text(converted.stdout)
                checked = subprocess.run(["qemu-arm", str(ROOT / "package/extensions/weather-clock/weather-clock"),
                                          "--validate", str(fixture)], capture_output=True)
                assert checked.returncode == 0, checked.stderr
        print(f"Passed {len(stamps)} Eastern-time checks and {len(valid)+len(invalid)} weather cases on host and ARM")
        print("Passed 4 HTTP conversion cases, including Unicode, negative temperatures, and zero values")
    # Clearly marked demonstration fixture, never installed as current weather.
    (ROOT / "sample-weather.json").write_text(json.dumps(SAMPLE, ensure_ascii=False))


if __name__ == "__main__":
    main()
