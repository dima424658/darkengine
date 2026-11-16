#!/usr/bin/env python3
"""Linux x86-64 rendered Mission 1 emitter shadow-pool crossover regression."""
import argparse
import os
import re
from pathlib import Path
import signal
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('game_path', type=Path)
parser.add_argument('--build', type=Path, default=root / 'build-linux')
parser.add_argument('--cache', type=int, choices=(0,1), default=None,
                    help='override the distant shadow pool; omitted tests the renderer default')
parser.add_argument('--static-depth', action='store_true',
                    help='exercise stable-depth composition during the camera route')
options = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='dark-native-torch-camera-') as temporary:
    folder = Path(temporary)
    helper = folder / 'reader.so'
    subprocess.run(['c++', '-std=c++17', '-shared', '-fPIC', '-I'+str(root/'h'),
                    '-I'+str(root/'libsrc/lgd3d'), str(Path(__file__).with_name('native_torch_camera_reader.cpp')),
                    '-O2', '-o', str(helper)], check=True)
    log = folder / 'game.log'
    command = ['gdb', '-q', '-batch', '-ex', f'set $fixture_helper="{helper}"',
               '-x', str(Path(__file__).with_suffix('.gdb')), '--args',
               str(options.build.resolve() / 'thief-runner'),
               f'game_path={options.game_path.resolve()}', 'content=vanilla',
               'mission_loop_test=1', 'auto_stop=122', 'renderer_mode=modern', 'renderer_cell_lights=1']
    if options.cache is not None:
        command.append(f'renderer_shadow_cache={options.cache}')
    if options.static_depth:
        command.append('renderer_shadow_static_cache=1')
    with log.open('w') as output:
        process = subprocess.Popen(command, env=os.environ.copy(), stdout=output,
                                   stderr=subprocess.STDOUT, start_new_session=True)
        try:
            process.wait(timeout=120)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGTERM)
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
            print(log.read_text(errors='replace'))
            raise SystemExit('native torch camera fixture timed out')
    text = log.read_text(errors='replace')
    states = re.findall(r'Rendered torch state frame=(\d+) object=(\d+) light=(\d+) active=(\d+) cached=(\d+)', text)
    route = re.search(r'Visible torch route object=(\d+) extent=([\d.]+) height=([\d.]+) angle=([\d.]+)', text)
    frames = re.findall(r'Visible torch camera frame=(\d+) distance=([\d.]+) cell=(\d+)', text)
    rows = [tuple(map(int, row)) for row in states if int(row[0])>=2]
    success = (route is not None and len(states)==122 and len(rows)==121 and len(frames)==121 and
               [row[0] for row in rows]==list(range(2,123)) and
               all(row[1]==int(route[1]) and row[2]>0 for row in rows) and
               all(int(row[0])==index+2 for index,row in enumerate(frames)))
    if options.cache != 0:
        success = success and all(row[3]+row[4]==1 for row in rows) and any(row[3] for row in rows) and any(row[4] for row in rows)
    else:
        success = success and any(row[3]==0 for row in rows) and any(row[3] for row in rows) and not any(row[4] for row in rows)
    errors = any(marker in text for marker in ('Validation Error', 'VUID-', 'SYNC-HAZARD', 'FATAL:'))
    if process.returncode or 'exited normally' not in text or not success or errors:
        print(text)
        raise SystemExit('native torch camera regression failed')
    print(route[0])
    for row in rows:
        if row[0] in (2,10,110,122): print('Rendered emitter state:', row)
    print('PASS camera-only route, actual renderer selection and emitter coverage crossover' if options.cache != 0 else 'PASS reproduction of unshadowed distant emitter with the cache disabled')
