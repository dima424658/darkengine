#!/usr/bin/env python3
"""Linux x86-64 Mission 2 lamppost material and emitter ownership regression."""
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
options = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='dark-native-lamppost-') as temporary:
    folder = Path(temporary)
    helper = folder / 'reader.so'
    subprocess.run(['c++', '-std=c++17', '-shared', '-fPIC', '-I'+str(root/'h'),
                    '-I'+str(root/'libsrc/lgd3d'), str(Path(__file__).with_name('native_lamppost_reader.cpp')),
                    '-o', str(helper)], check=True)
    log = folder / 'game.log'
    command = ['gdb', '-q', '-batch', '-ex', f'set $fixture_helper="{helper}"',
               '-x', str(Path(__file__).with_suffix('.gdb')), '--args',
               str(options.build.resolve() / 'thief-runner'),
               f'game_path={options.game_path.resolve()}', 'content=vanilla',
               'mission_loop_test=2', 'auto_stop=2', 'renderer_mode=modern', 'renderer_cell_lights=1']
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
            raise SystemExit('native lamppost fixture timed out')
    text = log.read_text(errors='replace')
    success = 'Fixture lamppost housing=5 lenses=2 owners=' in text
    errors = any(marker in text for marker in ('Validation Error', 'VUID-', 'SYNC-HAZARD', 'FATAL:'))
    if process.returncode or 'exited normally' not in text or not success or errors:
        print(text)
        raise SystemExit('native lamppost regression failed')
    print('PASS Mission 2 lamppost housing shading, authored lens glow, shadow casting and emitter ownership')
