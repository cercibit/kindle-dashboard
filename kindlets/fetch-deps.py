#!/usr/bin/env python3
"""Fetch pinned public build dependencies; nothing here is a personal signing key."""
from pathlib import Path
import hashlib
import urllib.request

folder = Path(__file__).resolve().parent/'.tools'
folder.mkdir(exist_ok=True)
dependencies = [
    ('ecj.jar', 'https://repo.maven.apache.org/maven2/org/eclipse/jdt/ecj/3.21.0/ecj-3.21.0.jar',
     '9082211f48782750093f07822d1ae481e8ece250449578f372334da626ccdead'),
    ('developer.keystore', 'https://storage.gra.cloud.ovh.net/v1/AUTH_2ac4bfee353948ec8ea7fd1710574097/mr-public/KUAL/developer.keystore',
     '9562467b94c3c02655b99c03f6750dcc6529f84ee6638c80527e954fbe8f9377'),
]
for name, url, digest in dependencies:
    path = folder/name
    data = path.read_bytes() if path.exists() else urllib.request.urlopen(url, timeout=60).read()
    if hashlib.sha256(data).hexdigest() != digest: raise SystemExit('Checksum mismatch: '+name)
    path.write_bytes(data)
    print('Verified', name)
