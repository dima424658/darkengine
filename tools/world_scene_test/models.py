#!/usr/bin/env python3
"""Build and run production MD/MM decoder regressions using a configured engine."""
import json
from pathlib import Path
import shlex
import subprocess
import tempfile
import argparse

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('build', nargs='?', default=str(root / 'build-linux'))
parser.add_argument('--shadow-fixture', help='Write synthetic MM pose vertices for the GPU ray-query test')
options = parser.parse_args()
build = Path(options.build).resolve()
entries = json.loads((build / 'compile_commands.json').read_text())
entry = next(e for e in entries if e['file'].endswith('/render/worldscene_models.cpp'))
args = entry.get('arguments') or shlex.split(entry['command'])
command = []
skip = False
for arg in args:
    if skip:
        skip = False
        continue
    if arg in ('-o', '-MF', '-MT'):
        skip = True
        continue
    if arg in ('-c', '-MD', '-DNDEBUG') or arg == entry['file']:
        continue
    command.append(arg)
with tempfile.TemporaryDirectory(prefix='dark-model-test-') as temporary:
    executable = Path(temporary) / 'models'
    command += ['-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
                str(root / 'tools/world_scene_test/models.cpp'),
                str(build / 'libdark_mp.a'), str(build / 'libdark_matrix.a'), str(build / 'libdark_fix.a'),
                '-lm', '-o', str(executable)]
    subprocess.run(command, cwd=entry['directory'], check=True)
    subprocess.run([str(executable)] + ([options.shadow_fixture] if options.shadow_fixture else []), check=True)
