#!/usr/bin/env python3
from pathlib import Path
import datetime as dt
from zoneinfo import ZoneInfo

rows = []
for year in range(2025,2031):
    for month,day in [(3,d) for d in range(7,16)]+[(11,d) for d in range(1,9)]:
        for hour in range(24):
            utc = dt.datetime(year,month,day,hour,30,tzinfo=dt.timezone.utc)
            local = utc.astimezone(ZoneInfo('America/New_York'))
            rows.append(','.join(str(x) for x in [int(utc.timestamp()*1000),local.year,local.month,local.day,local.hour,local.minute]))
(Path(__file__).resolve().parent/'weather/test/eastern.csv').write_text('\n'.join(rows)+'\n')
