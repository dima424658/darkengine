#!/usr/bin/env python3
"""Linux: mask required RTX capabilities and require native-renderer fallback."""
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
parser.add_argument('--feature',choices=('all','ray-query','nrd-storage','sharc-atomics'),default='all')
parser.add_argument('--log-dir',type=Path)
options=parser.parse_args()
if options.log_dir:options.log_dir.mkdir(parents=True,exist_ok=True)

with tempfile.TemporaryDirectory(prefix='dark-feature-fallback-') as temporary:
    folder=Path(temporary);(folder/'saves').mkdir()
    hook=folder/'missing-feature.so'
    subprocess.run(['c++','-std=c++17','-shared','-fPIC',str(Path(__file__).with_name('missing_feature.cpp')),
                    '-ldl','-o',str(hook)],check=True)
    for feature in ('ray-query','nrd-storage','sharc-atomics') if options.feature=='all' else (options.feature,):
        log=folder/f'{feature}.log';trace=folder/f'{feature}.trace'
        env=os.environ.copy();env['LD_PRELOAD']=str(hook)+(':'+env['LD_PRELOAD'] if env.get('LD_PRELOAD') else '')
        env['DARK_TEST_MISSING_FEATURE']=feature;env['DARK_RENDER_TRACE']=str(trace)
        command=[str(options.build.resolve()/'thief-runner'),f'game_path={options.game_path.resolve()}',
                 f'save_path={folder/"saves"}','content=vanilla','mission_loop_test=2','auto_stop=40',
                 'renderer_mode=modern','SlowFrame=0','min_frame_time=0','vsync=0','game_full_screen=0']
        with log.open('w') as output:
            process=subprocess.Popen(command,env=env,stdout=output,stderr=subprocess.STDOUT,start_new_session=True)
            timed_out=False
            try:process.wait(timeout=120)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid,signal.SIGTERM)
                try:process.wait(timeout=5)
                except subprocess.TimeoutExpired:os.killpg(process.pid,signal.SIGKILL);process.wait()
                timed_out=True
        text=log.read_text(errors='replace');timings=trace.read_text(errors='replace') if trace.exists() else ''
        if options.log_dir:
            (options.log_dir/f'{feature}.log').write_text(text)
            (options.log_dir/f'{feature}.trace').write_text(timings)
        masks=re.search(r'Fixture missing feature masked=([1-9]\d*)',text)
        errors=any(marker in text or marker in timings for marker in
                   ('Validation Error','VUID-','SYNC-HAZARD','FATAL:'))
        native=timings.count('frame-pacing modern=0 ')
        if (timed_out or process.returncode or not masks or errors or native<39 or
            'Modern world unavailable:' not in text or 'retaining legacy renderer' not in text or
            'Modern world lighting enabled' in text or 'frame-pacing modern=1 ' in timings):
            print(text[-10000:]);raise SystemExit(f'{feature}: capability fallback failed (native frames={native})')
        print(f'PASS {feature}: masked={masks[1]}, native frames={native}, clean validation')
