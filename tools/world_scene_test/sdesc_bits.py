#!/usr/bin/env python3
"""Run the production named-bit parser under ASan/UBSan with engine flags."""
import json
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
root = Path(__file__).resolve().parents[2]
build = Path(sys.argv[1] if len(sys.argv) > 1 else root / 'build-linux').resolve()
entries = json.loads((build / 'compile_commands.json').read_text())
entry = next(e for e in entries if e['file'].endswith('/sdesc/sdesc.cpp'))
command = []
skip = False
for arg in entry.get('arguments') or shlex.split(entry['command']):
    if skip:
        skip = False
        continue
    if arg in ('-o', '-MF', '-MT'):
        skip = True
        continue
    if arg in ('-c', '-MD', entry['file']):
        continue
    command.append(arg)
with tempfile.TemporaryDirectory(prefix='dark-sdesc-test-') as temporary:
    executable = Path(temporary) / 'sdesc-bits'
    command += ['-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
                '-fsanitize=address,undefined', str(root / 'tools/world_scene_test/sdesc_bits.cpp'),
                '-o', str(executable)]
    subprocess.run(command, cwd=entry['directory'], check=True)
    subprocess.run([str(executable)], check=True)
