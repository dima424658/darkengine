#!/usr/bin/env python3
"""Linux x86-64 native unloaded model-texture palette snapshot regression."""
import argparse
import os
from pathlib import Path
import re
import signal
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('game_path', type=Path)
parser.add_argument('--build', type=Path, default=root / 'build-linux')
options = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='dark-palette-snapshot-') as temporary:
    folder = Path(temporary)
    helper = folder / 'reader.so'
    subprocess.run(['c++', '-std=c++17', '-shared', '-fPIC', '-I'+str(root/'h'),
                    '-I'+str(root/'libsrc/lgd3d'), str(Path(__file__).with_name('palette_snapshot_reader.cpp')),
                    '-o', str(helper)], check=True)
    log = folder / 'game.log'
    command = ['gdb', '-q', '-batch', '-ex', f'set $fixture_helper="{helper}"',
               '-x', str(Path(__file__).with_suffix('.gdb')), '--args',
               str(options.build.resolve() / 'thief-runner'),
               f'game_path={options.game_path.resolve()}', 'content=vanilla',
               'mission_loop_test=1', 'auto_stop=4', 'renderer_mode=modern']
    with log.open('w') as output:
        process = subprocess.Popen(command, env=os.environ.copy(), stdout=output,
                                   stderr=subprocess.STDOUT, start_new_session=True)
        try:
            process.wait(timeout=60)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGTERM)
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
            print(log.read_text(errors='replace'))
            raise SystemExit('palette snapshot fixture timed out')
    text = log.read_text(errors='replace')
    pixel = re.search(r'Fixture unloaded palette handle=(\d+)->(\d+) red=0xff0000ff blue=0xffff0000', text)
    stable = re.search(r'Fixture stable palette revision=(\d+)->(\d+)', text)
    success = (pixel and pixel[1] == pixel[2] and stable and
               int(stable[1]) > 0 and stable[1] == stable[2])
    errors = any(marker in text for marker in ('Validation Error', 'VUID-', 'SYNC-HAZARD', 'FATAL:'))
    if process.returncode or 'exited normally' not in text or not success or errors:
        print(text)
        raise SystemExit('unloaded model texture palette snapshot regression failed')
    print(pixel[0])
    print(stable[0])
    print('PASS unloaded native model texture recoloring, stable scene handle, and unchanged-palette cache reuse')
