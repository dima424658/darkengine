#!/usr/bin/env python3
"""Linux x86-64: fail one modern queue submission and verify rendering resumes."""
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
parser.add_argument('--portal-drawing', type=int, choices=(0,1), default=0)
parser.add_argument('--after', type=int, default=0,
                    help='allow this many modern submissions before failing one')
parser.add_argument('--equip-sword', action='store_true',
                    help='select the sword through the native inventory command before recording')
parser.add_argument('--log', type=Path, help='preserve the complete native/GDB log')
options = parser.parse_args()
if options.after < 0:
    parser.error('--after must be nonnegative')
with tempfile.TemporaryDirectory(prefix='dark-submit-failure-') as temporary:
    log = Path(temporary) / 'game.log'
    (Path(temporary)/'saves').mkdir()
    hook = Path(temporary) / 'submit.so'
    subprocess.run(['c++', '-std=c++17', '-shared', '-fPIC',
                    str(Path(__file__).with_name('native_submit_failure_hook.cpp')),
                    '-ldl', '-o', str(hook)], check=True)
    environment = os.environ.copy()
    environment['LD_PRELOAD'] = str(hook) + (':'+environment['LD_PRELOAD'] if environment.get('LD_PRELOAD') else '')
    if environment.get('DARK_NATIVE_CAPTURE'):
        environment['DARK_NATIVE_CAPTURE_LOG'] = str(log)
    command = ['gdb', '-q', '-batch', '-ex', f'set $fixture_skip={options.after}',
               '-ex', f'set $fixture_sword={int(options.equip_sword)}',
               '-ex', 'set environment LD_PRELOAD '+environment['LD_PRELOAD']+
                    (':'+environment['DARK_TEST_INFERIOR_PRELOAD'] if environment.get('DARK_TEST_INFERIOR_PRELOAD') else ''),
               '-x', str(Path(__file__).with_suffix('.gdb')),
               '--args', str(options.build.resolve() / 'thief-runner'),
               f'game_path={options.game_path.resolve()}', f'save_path={Path(temporary)/"saves"}', 'content=vanilla',
               'mission_loop_test=1', f'auto_stop={options.after+8}', 'renderer_mode=modern',
               f'renderer_portal_rendering={options.portal_drawing}']
    with log.open('w') as output:
        process = subprocess.Popen(command, env=environment, stdout=output,
                                   stderr=subprocess.STDOUT, start_new_session=True)
        try:
            process.wait(timeout=120)
        except subprocess.TimeoutExpired:
            process.send_signal(signal.SIGINT)
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
            print(log.read_text(errors='replace'))
            raise SystemExit('submission failure fixture timed out')
    text = log.read_text(errors='replace')
    if options.log:
        options.log.write_text(text)
    row = re.search(r'Fixture submit failure failed=1 recovered=([1-9]\d*) total_modern=([1-9]\d*)', text)
    restored = text.count('Modern world lighting enabled (') >= 2
    if options.equip_sword:
        restored = restored and 'Fixture sword selection requested' in text
    if row is not None:
        restored = restored and int(row[2])-int(row[1]) == options.after
    errors = any(marker in text for marker in ('Validation Error', 'VUID-', 'SYNC-HAZARD', 'FATAL:'))
    if process.returncode or 'exited normally' not in text or row is None or not restored or errors:
        print(text)
        raise SystemExit('native submission failure recovery failed')
    print(row[0])
    print('PASS rejected modern submission, renderer rebuild, resumed modern submissions and clean shutdown')
