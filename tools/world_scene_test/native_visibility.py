#!/usr/bin/env python3
"""Linux x86-64 portal terrain visibility and offscreen caster regression."""
import argparse
import os
from pathlib import Path
import signal
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('game_path', type=Path)
parser.add_argument('--build', type=Path, default=root / 'build-linux')
parser.add_argument('--visibility', type=int, choices=(0,1), default=1)
parser.add_argument('--portal-drawing', type=int, choices=(0,1), default=1)
parser.add_argument('--mission', type=int, choices=(1,2), default=1)
options = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='dark-native-visibility-') as temporary:
    folder = Path(temporary)
    helper = folder / 'reader.so'
    subprocess.run(['c++', '-std=c++17', '-shared', '-fPIC', '-I'+str(root/'h'),
                    '-I'+str(root/'libsrc/lgd3d'), str(Path(__file__).with_name('native_visibility_reader.cpp')),
                    '-o', str(helper)], check=True)
    log = folder / 'game.log'
    command = ['gdb', '-q', '-batch', '-ex', f'set $fixture_helper="{helper}"',
               '-ex', f'set $fixture_visibility={options.visibility}',
               '-x', str(Path(__file__).with_suffix('.gdb')), '--args',
               str(options.build.resolve() / 'thief-runner'),
               f'game_path={options.game_path.resolve()}', 'content=vanilla',
               f'mission_loop_test={options.mission}', 'auto_stop=2', 'renderer_mode=modern', 'renderer_cell_lights=1',
               f'renderer_portal_visibility={options.visibility}',
               f'renderer_portal_rendering={options.portal_drawing}']
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
            raise SystemExit('native visibility fixture timed out')
    text = log.read_text(errors='replace')
    success = text.count(f'Fixture terrain visibility enabled={options.visibility} ') == 2
    errors = any(marker in text for marker in ('Validation Error', 'VUID-', 'SYNC-HAZARD', 'FATAL:'))
    if process.returncode or 'exited normally' not in text or not success or errors:
        print(text)
        raise SystemExit('native visibility regression failed')
    for line in text.splitlines():
        if line.startswith(('Fixture terrain visibility ', 'Fixture model visibility ')): print(line)
    print('PASS native terrain/model visibility, spanning-cell references and retained offscreen shadow casters')
