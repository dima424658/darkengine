#!/usr/bin/env python3
"""Linux retail Thief 2 Mission 1 material-property invalidation regression.

Uses a disposable game process; debugger edits affect only its in-memory mission.
Pass a test game_path with configuration and vanilla assets, as for mission_loop_test.
"""
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
with tempfile.TemporaryDirectory(prefix='dark-material-properties-') as temporary:
    trace = Path(temporary) / 'trace.log'
    log = Path(temporary) / 'game.log'
    environment = dict(os.environ, DARK_RENDER_TRACE=str(trace))
    command = ['gdb', '-q', '-batch', '-x', str(Path(__file__).with_suffix('.gdb')),
               '--args', str(options.build.resolve() / 'thief-runner'),
               f'game_path={options.game_path.resolve()}', 'content=vanilla',
               'mission_loop_test=1', 'auto_stop=24', 'renderer_mode=modern',
               'game_screen_size=1920 1080', 'game_screen_fit=0']
    with log.open('w') as output:
        process = subprocess.Popen(command, env=environment, stdout=output,
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
            raise SystemExit('material property fixture timed out')
    text = log.read_text(errors='replace')
    expected = [(1, 1, 1), (.25, .5, 1), (.75, .125, 1), (1, 1, 1),
                (.6, .375, 1), (1, 1, 1), (.45, 1, 0), (1, 1, 1),
                (1, 1, 0), (1, 1, 1), (1, 1, 0), (1, 1, 1)]
    values = [(float(alpha), float(illum), int(shadow)) for alpha, illum, shadow in re.findall(
        r'world-material-properties obj=11 alpha=([\d.]+) self_illum=([\d.]+) casts_shadow=(\d)',
        trace.read_text() if trace.exists() else '')]
    validation_error = 'Validation Error' in text or 'SYNC-HAZARD' in text
    if process.returncode or 'exited normally' not in text or values != expected or validation_error:
        print(text)
        raise SystemExit(f'material property invalidation failed: expected {expected}, got {values}')
    print('PASS native concrete/archetype edits, property removal, hierarchy add/remove, shadow toggles, and cached material refresh')
