#!/usr/bin/env python3
"""Build/test/sign Java 1.4 Kindlets; compile-only stubs never enter the apps."""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import struct
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parent

def run(*args, cwd=None):
    subprocess.run([str(a) for a in args], cwd=cwd or ROOT, check=True)

def build(app):
    home = Path(os.environ.get('JAVA8_HOME', '/usr/lib/jvm/java-8-openjdk-amd64'))
    java = home / 'bin/java'
    rt = home / 'jre/lib/rt.jar'
    if not rt.exists(): rt = home / 'lib/rt.jar'
    if not java.exists() or not rt.exists(): raise SystemExit('Set JAVA8_HOME to a Java 8 JDK or JRE.')
    ecj = Path(os.environ.get('ECJ_JAR', ROOT / '.tools/ecj.jar'))
    keystore = Path(os.environ.get('KEYSTORE', ROOT / '.tools/developer.keystore'))
    if not ecj.exists() or not keystore.exists(): raise SystemExit('Run python3 kindlets/fetch-deps.py, or set ECJ_JAR and KEYSTORE.')
    password = os.environ.get('STOREPASS', 'password')
    folder = ROOT / app
    classes = folder / 'build/classes'
    tests = folder / 'build/tests'
    if classes.exists(): shutil.rmtree(classes)
    if tests.exists(): shutil.rmtree(tests)
    classes.mkdir(parents=True); tests.mkdir(parents=True)
    sources = sorted((folder/'src').rglob('*.java')) + sorted((ROOT/'stubs').rglob('*.java'))
    compile_args = [java, '-jar', ecj, '-source', '1.4', '-target', '1.4', '-bootclasspath', rt]
    run(*compile_args, '-d', classes, *sources)
    run(*compile_args, '-classpath', classes, '-d', tests, *sorted((folder/'test').glob('*.java')))
    if app == 'weather': run('python3', ROOT/'generate-time-fixture.py')
    test = 'WeatherTest' if app == 'weather' else 'CalculatorTest'
    run(java, '-Djava.awt.headless=true', '-cp', str(classes)+os.pathsep+str(tests), test, cwd=folder)
    dist = ROOT/'dist'; dist.mkdir(exist_ok=True)
    artifact = dist / ('WeatherDashboard.azw2' if app == 'weather' else 'Calculator.azw2')
    with zipfile.ZipFile(artifact, 'w', zipfile.ZIP_DEFLATED) as archive:
        archive.writestr('META-INF/MANIFEST.MF', (folder/'MANIFEST.MF').read_bytes().rstrip(b'\n')+b'\n\n')
        for p in sorted(classes.rglob('*.class')):
            relative = str(p.relative_to(classes))
            if not relative.startswith(('com/mehmet/', 'ixtab/')): continue
            if struct.unpack('>H', p.read_bytes()[6:8])[0] != 48: raise RuntimeError('Wrong Java class version')
            archive.write(p, relative)
    if os.environ.get('JARSIGNER_TOOLS_JAR'):
        signer = [java, '-cp', os.environ['JARSIGNER_TOOLS_JAR'], 'sun.security.tools.jarsigner.Main']
    else:
        jarsigner = home/'bin/jarsigner'
        if not jarsigner.exists(): raise SystemExit('A JDK 8 jarsigner is required, or set JARSIGNER_TOOLS_JAR.')
        signer = [jarsigner]
    for role in ['dktest','ditest','dntest']:
        run(*signer, '-keystore', keystore, '-storepass', password, '-keypass', password,
            '-digestalg', 'SHA-256', '-sigalg', 'SHA256withRSA', artifact, role)
    run(*signer, '-verify', artifact)
    print(hashlib.sha256(artifact.read_bytes()).hexdigest(), artifact)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(); parser.add_argument('app', choices=['calculator','weather','all'], default='all', nargs='?')
    args = parser.parse_args()
    for app in (['calculator','weather'] if args.app == 'all' else [args.app]): build(app)
