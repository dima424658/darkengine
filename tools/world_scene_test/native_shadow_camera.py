#!/usr/bin/env python3
"""Linux x86-64 render-camera yaw sweep for shadow runtime validation/profiling."""
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
parser.add_argument('--frames',type=int,default=180)
parser.add_argument('--mission',type=int,choices=(1,2),default=2)
parser.add_argument('--visibility',type=int,choices=(0,1),help='override portal visibility; omitted tests the renderer default')
parser.add_argument('--portal-drawing',type=int,choices=(0,1),help='override native opaque drawing')
parser.add_argument('--yaw-step',type=int,default=182,help='angle units/frame; zero keeps the native camera')
parser.add_argument('--static-depth',action='store_true')
parser.add_argument('--trace',type=Path)
parser.add_argument('--log',type=Path)
options=parser.parse_args()
if options.frames<4:parser.error('--frames must be at least 4')
with tempfile.TemporaryDirectory(prefix='dark-shadow-camera-') as temporary:
    log=Path(temporary)/'game.log'
    environment=os.environ.copy()
    if options.trace:environment['DARK_RENDER_TRACE']=str(options.trace.resolve())
    args=['gdb','-q','-batch','-ex',f'set $fixture_yaw_step={options.yaw_step}',
          '-x',str(Path(__file__).with_suffix('.gdb')),'--args',
          str(options.build.resolve()/'thief-runner'),f'game_path={options.game_path.resolve()}',
          'content=vanilla',f'mission_loop_test={options.mission}',f'auto_stop={options.frames}','renderer_mode=modern',
          'game_screen_size=1920 1080','game_screen_fit=0',f'renderer_shadow_static_cache={int(options.static_depth)}']
    if options.visibility is not None:args.append(f'renderer_portal_visibility={options.visibility}')
    if options.portal_drawing is not None:args.append(f'renderer_portal_rendering={options.portal_drawing}')
    with log.open('w') as out:
        process=subprocess.Popen(args,env=environment,stdout=out,stderr=subprocess.STDOUT,start_new_session=True)
        try:process.wait(timeout=120)
        except subprocess.TimeoutExpired:
            process.send_signal(signal.SIGINT)
            try:process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid,signal.SIGKILL);process.wait()
            print(log.read_text(errors='replace'))
            raise SystemExit('shadow camera fixture timed out')
    text=log.read_text(errors='replace')
    if options.log:options.log.write_text(text)
    row=re.search(r'Fixture shadow camera frames=(\d+) yaw_step=(-?\d+)',text)
    errors=any(x in text for x in ('Validation Error','VUID-','SYNC-HAZARD','FATAL:','Modern world disabled:'))
    if process.returncode or 'exited normally' not in text or row is None or int(row[1])!=options.frames or int(row[2])!=options.yaw_step or 'Modern world lighting enabled' not in text or errors:
        print(text)
        raise SystemExit('native shadow camera regression failed')
    print(row[0])
    print('PASS render-camera-only yaw sweep, modern renderer and clean shutdown; GDB overhead invalidates frame-rate claims')
