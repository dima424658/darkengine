#!/usr/bin/env python3
"""Run a stationary retail mission without GDB and summarize renderer timings."""
import argparse
import json
import os
from pathlib import Path
import re
import signal
import statistics
import subprocess
import tempfile

root=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('game_path',type=Path)
parser.add_argument('--build',type=Path,default=root/'build-linux')
parser.add_argument('--mission',type=int,choices=(1,2),default=2)
parser.add_argument('--frames',type=int,default=300)
parser.add_argument('--warmup',type=int,default=60)
parser.add_argument('--static-depth',type=int,choices=(0,1))
parser.add_argument('--visibility',type=int,choices=(0,1))
parser.add_argument('--portal-drawing',type=int,choices=(0,1))
parser.add_argument('--trace',type=Path)
parser.add_argument('--log',type=Path)
parser.add_argument('--output',type=Path)
options=parser.parse_args()
if options.frames<4 or not 0<=options.warmup<options.frames-2:
    parser.error('require at least four frames and two measured frames after warmup')

def summarize(values):
    ordered=sorted(values)
    return {'samples':len(values),'median_ms':statistics.median(values),
            'mean_ms':statistics.mean(values),
            'p95_ms':ordered[int(.95*(len(ordered)-1))],
            'p99_ms':ordered[int(.99*(len(ordered)-1))],'max_ms':max(values)}

with tempfile.TemporaryDirectory(prefix='dark-native-performance-') as temporary:
    folder=Path(temporary)
    trace=options.trace.resolve() if options.trace else folder/'trace.log'
    # RenderBackendTrace appends; remove only this explicitly selected output.
    trace.write_text('')
    log=folder/'game.log'
    environment=os.environ.copy();environment['DARK_RENDER_TRACE']=str(trace)
    args=[str(options.build.resolve()/'thief-runner'),f'game_path={options.game_path.resolve()}',
          'content=vanilla',f'mission_loop_test={options.mission}',f'auto_stop={options.frames}',
          'renderer_mode=modern','game_screen_size=1920 1080','game_screen_fit=0']
    for key,value in [('renderer_shadow_static_cache',options.static_depth),
                      ('renderer_portal_visibility',options.visibility),
                      ('renderer_portal_rendering',options.portal_drawing)]:
        if value is not None:args.append(f'{key}={value}')
    with log.open('w') as out:
        process=subprocess.Popen(args,env=environment,stdout=out,stderr=subprocess.STDOUT,start_new_session=True)
        try:process.wait(timeout=120)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid,signal.SIGTERM)
            try:process.wait(timeout=5)
            except subprocess.TimeoutExpired:os.killpg(process.pid,signal.SIGKILL);process.wait()
            failure=log.read_text(errors='replace')
            if options.log:options.log.write_text(failure)
            print(failure[-6000:]);raise SystemExit('native performance run timed out')
    text=log.read_text(errors='replace');timings=trace.read_text(errors='replace')
    if options.log:options.log.write_text(text)
    errors=any(marker in text or marker in timings for marker in
               ('Validation Error','VUID-','SYNC-HAZARD','FATAL:','Modern world disabled:'))
    patterns={
        'frame':(r'frame-pacing modern=1 interval_ms=([\d.-]+) render_submit_ms=([\d.-]+) acquire_ms=([\d.-]+) canvas_ms=([\d.-]+) submit_present_ms=([\d.-]+)',
                 ['interval','render_submit','acquire','canvas','submit_present']),
        'world':(r'world-shadows .*previous_slot_gpu_ms=([\d.-]+) previous_world_gpu_ms=([\d.-]+) record_cpu_ms=([\d.-]+)',
                 ['shadow_gpu','world_gpu','record_cpu']),
        'cpu':(r'world-record-cpu plan_ms=([\d.-]+) uploads_tiles_ms=([\d.-]+) shadows_ms=([\d.-]+) lighting_ms=([\d.-]+)',
               ['plan_cpu','preparation_cpu','shadow_cpu','lighting_cpu']),
        'capture':(r'world-capture cpu_ms=([\d.-]+) preceding_native_ms=([\d.-]+)',
                   ['capture_cpu','preceding_native_cpu'])}
    samples={key:[tuple(map(float,row)) for row in re.findall(pattern,timings)]
             for key,(pattern,_) in patterns.items()}
    if (process.returncode or errors or 'Modern world lighting enabled' not in text or
        any(len(samples[key])!=options.frames for key in ('world','cpu','capture')) or
        not options.frames-1<=len(samples['frame'])<=options.frames or
        any(len(rows)<=options.warmup for rows in samples.values())):
        print(text[-6000:]);raise SystemExit('native performance run failed completeness/validation checks')
    result={'mission':options.mission,'frames':options.frames,'warmup':options.warmup,
            'configuration':{'static_depth':options.static_depth,'visibility':options.visibility,
                             'portal_drawing':options.portal_drawing},'command':args,
            'limitations':'Stationary view, no GDB. Presentation/compositor, tracing and hardware clocks affect timings; not sustained traversal FPS.',
            'metrics':{}}
    for group,(_,names) in patterns.items():
        rows=samples[group][options.warmup:]
        for index,name in enumerate(names):
            values=[row[index] for row in rows if row[index]>=0]
            if not values:raise SystemExit(f'no valid {name} timing samples')
            result['metrics'][name]=summarize(values)
    intervals=[row[0] for row in samples['frame'][options.warmup:] if row[0]>0]
    result['frame_budget']={'budget_ms':1000/60,'over_budget':sum(value>1000/60 for value in intervals),
                            'samples':len(intervals),'fps_from_mean_interval':1000/statistics.mean(intervals)}
    output=json.dumps(result,indent=2)+'\n'
    if options.output:options.output.write_text(output)
    print(output,end='')
