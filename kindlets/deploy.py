#!/usr/bin/env python3
"""Deploy both Kindlets and the complete native KUAL dashboard over USB."""
import argparse
import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import shutil
import zipfile

HERE = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument('mount', nargs='?', default='/media/mehmet/Kindle')
args = parser.parse_args()
mount = Path(args.mount)
if not os.path.ismount(mount) or not (mount/'documents').is_dir(): raise SystemExit('Connect Kindle in USB Drive Mode first.')
if not (mount/'developer').is_dir(): raise SystemExit('Install jailbreak, MKK and current developer certificates first.')
files = []
for name in ['Calculator.azw2','WeatherDashboard.azw2']:
    app = HERE/'dist'/name
    with zipfile.ZipFile(app) as archive:
        if archive.testzip() is not None: raise SystemExit('Corrupt artifact: '+name)
        if any(n.startswith(('com/amazon/','org/json/')) for n in archive.namelist()): raise SystemExit('Compile-only stubs included')
        if not all('META-INF/'+role+'.RSA' in archive.namelist() for role in ['DKTEST','DITEST','DNTEST']): raise SystemExit('Unsigned artifact')
    files.append((app,mount/'documents'/name))
native = HERE.parent/'package/extensions/weather-clock'
for path in sorted(native.iterdir()):
    if path.is_file(): files.append((path,mount/'extensions/weather-clock'/path.name))
files.append((HERE/'weather/helper.sh',mount/'extensions/weather-clock/kindlet-helper.sh'))
backup = HERE/'device-backups'/dt.datetime.now().strftime('%Y%m%d-%H%M%S')
records = []
for source,target in files:
    if target.exists() and source.read_bytes() != target.read_bytes():
        saved = backup/target.relative_to(mount); saved.parent.mkdir(parents=True,exist_ok=True); shutil.copy2(target,saved)
    target.parent.mkdir(parents=True,exist_ok=True); shutil.copyfile(source,target)
os.sync()
for source,target in files:
    if source.read_bytes() != target.read_bytes(): raise SystemExit('Deployment verification failed: '+str(target))
    records.append({'path':str(target.relative_to(mount)), 'sha256':hashlib.sha256(target.read_bytes()).hexdigest()})
(HERE/'deployment.json').write_text(json.dumps({'timestamp':dt.datetime.now().isoformat(),'files':records,'runtime_status':'Device launch pending'},indent=2)+'\n')
print('Copied and verified',len(files),'files. Previous versions are backed up under',backup)
