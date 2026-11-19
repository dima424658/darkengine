#!/usr/bin/env python3
"""Linux x86-64: fail one modern allocation and verify native terrain fallback."""
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
parser.add_argument('--log', type=Path)
parser.add_argument('--phase', choices=('world','nrd','sharc'), default='world',
                    help='arm during world setup, NRD initialization, or immediately after NRD before SHaRC allocation')
parser.add_argument('--portal-drawing', type=int, choices=(0,1), default=0)
parser.add_argument('--skip-allocations', type=int, default=0,
                    help='allow this many allocations after world recording starts before failing one')
options = parser.parse_args()
if options.skip_allocations < 0:
    parser.error('--skip-allocations must be nonnegative')
with tempfile.TemporaryDirectory(prefix='dark-allocation-failure-') as temporary:
    log = Path(temporary) / 'game.log'
    (Path(temporary)/'saves').mkdir()
    hook = Path(temporary) / 'allocation.so'
    subprocess.run(['c++', '-std=c++17', '-shared', '-fPIC',
                    str(Path(__file__).with_name('native_allocation_failure_hook.cpp')),
                    '-ldl', '-o', str(hook)], check=True)
    environment = os.environ.copy()
    environment['LD_PRELOAD'] = str(hook) + (':'+environment['LD_PRELOAD'] if environment.get('LD_PRELOAD') else '')
    command = ['gdb', '-q', '-batch', '-ex', f'set $fixture_skip={options.skip_allocations}',
               '-ex', f'set $fixture_phase={dict(world=0,nrd=1,sharc=2)[options.phase]}',
               '-x', str(Path(__file__).with_suffix('.gdb')),
               '--args', str(options.build.resolve() / 'thief-runner'),
               f'game_path={options.game_path.resolve()}', f'save_path={Path(temporary)/"saves"}', 'content=vanilla',
               'mission_loop_test=1', 'auto_stop=3', 'renderer_mode=modern',
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
            raise SystemExit('allocation failure fixture timed out')
    text = log.read_text(errors='replace')
    if options.log:options.log.write_text(text)
    row = re.search(r'Fixture allocation failure armed=1 failed=1 native_draws=([1-9]\d*) suppressed=0', text)
    expected = 'Modern world disabled:' in text and 'retaining legacy renderer' in text
    # The intentional allocation failure is not a validation error. Any Vulkan
    # validation/synchronization error still fails this fixture.
    errors = any(marker in text for marker in ('Validation Error', 'VUID-', 'SYNC-HAZARD', 'FATAL:'))
    if process.returncode or 'exited normally' not in text or row is None or not expected or errors:
        print(text)
        raise SystemExit('native allocation failure fallback failed')
    print(row[0])
    for line in text.splitlines():
        if line.startswith('Modern world disabled:'):
            print(line)
    print('PASS injected allocation failure, retained native terrain drawing and clean shutdown')
