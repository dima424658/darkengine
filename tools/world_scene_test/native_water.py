#!/usr/bin/env python3
"""Linux x86-64: retain water/late draws at the Thief 2 miss2 fixture location."""
import argparse
import os
from pathlib import Path
import re
import signal
import subprocess
import tempfile

root=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('game_path',type=Path)
parser.add_argument('--build',type=Path,default=root/'build-linux')
parser.add_argument('--portal-drawing',type=int,choices=(0,1),default=0)
parser.add_argument('--log',type=Path)
options=parser.parse_args()
with tempfile.TemporaryDirectory(prefix='dark-native-water-') as temporary:
    log=Path(temporary)/'game.log'
    (Path(temporary)/'saves').mkdir()
    command=['gdb','-q','-batch','-x',str(Path(__file__).with_suffix('.gdb')),'--args',
             str(options.build.resolve()/'thief-runner'),f'game_path={options.game_path.resolve()}',
             f'save_path={Path(temporary)/"saves"}',
             'content=vanilla','mission_loop_test=2','auto_stop=12','renderer_mode=modern',
             f'renderer_portal_rendering={options.portal_drawing}']
    with log.open('w') as out:
        process=subprocess.Popen(command,env=os.environ.copy(),stdout=out,stderr=subprocess.STDOUT,start_new_session=True)
        try:
            process.wait(timeout=60)
        except subprocess.TimeoutExpired:
            process.send_signal(signal.SIGINT)
            try:process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid,signal.SIGKILL);process.wait()
            print(log.read_text(errors='replace'))
            raise SystemExit('native water fixture timed out')
    text=log.read_text(errors='replace')
    if options.log:options.log.write_text(text)
    result=re.search(r'Fixture native water batches=([1-9]\d*) draws=([1-9]\d*) suppressed=0 late_draws=([1-9]\d*) late_suppressed=0',text)
    errors=any(x in text for x in ('Validation Error','VUID-','SYNC-HAZARD','FATAL:','Modern world disabled:'))
    if process.returncode or 'exited normally' not in text or result is None or errors or 'Modern world lighting enabled' not in text:
        print(text)
        raise SystemExit('native water regression failed')
    print(result[0])
    print('PASS native water reaches backend unsuppressed, modern rendering stays enabled and shutdown succeeds')
