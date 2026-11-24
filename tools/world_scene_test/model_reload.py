#!/usr/bin/env python3
"""Linux x86-64 retail Mission 1 native model-slot reload regression.

Debugger changes affect only an isolated process. Uses the native locked-resource
lifecycle, retaining cached bytes to exercise a reload at the same pointer/size.
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
with tempfile.TemporaryDirectory(prefix='dark-model-reload-') as temporary:
    trace = Path(temporary) / 'trace.log'
    log = Path(temporary) / 'game.log'
    command = ['gdb', '-q', '-batch', '-x', str(Path(__file__).with_suffix('.gdb')),
               '--args', str(options.build.resolve() / 'thief-runner'),
               f'game_path={options.game_path.resolve()}', 'content=vanilla',
               'mission_loop_test=1', 'auto_stop=12', 'renderer_mode=modern',
               'game_screen_size=1920 1080', 'game_screen_fit=0']
    with log.open('w') as output:
        process = subprocess.Popen(command, env=dict(os.environ, DARK_RENDER_TRACE=str(trace)),
                                   stdout=output, stderr=subprocess.STDOUT, start_new_session=True)
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
            raise SystemExit('native model reload fixture timed out')
    text = log.read_text(errors='replace')
    match = re.search(r'Fixture model reload obj=11 index=(\d+) found=1 loaded=(\d+) '
                      r'revision=(\d+)->(\d+) invalid=0 same_pointer=1 same_size=1 refs=(\d+)', text)
    events = re.findall(r'world-model-reload obj=11 index=(\d+) revision=(\d+)->(\d+) bytes=(\d+)->(\d+)',
                        trace.read_text() if trace.exists() else '')
    success = False
    if match:
        index, loaded, before, after, refs = map(int, match.groups())
        success = (index == loaded and before > 0 and after > before and refs > 0 and
                   f'Fixture stable revision={after} modern_has_model=1' in text and len(events) == 1 and
                   tuple(map(int, events[0][:3])) == (index, before, after) and
                   events[0][3] == events[0][4])
    validation_error = any(marker in text for marker in ('Validation Error', 'VUID-', 'SYNC-HAZARD', 'FATAL:'))
    if process.returncode or 'exited normally' not in text or not success or validation_error:
        print(text)
        print('Reload trace:', events)
        raise SystemExit('native model-slot reload regression failed')
    print(match.group(0))
    print('Modern rebuild:', events[0])
    print('PASS native same-pointer/same-size model reload, invalid-slot revision, retained ownership, and one modern rebuild')
