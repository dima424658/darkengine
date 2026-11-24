#!/usr/bin/env python3
"""Run a stationary retail mission without GDB and summarize renderer timings."""
import argparse
import json
import math
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
parser.add_argument('--raytracing',type=int,choices=(0,1))
parser.add_argument('--dlss',default='off',choices=('off','quality','balanced','performance','dlaa'))
parser.add_argument('--gi',type=int,choices=(0,1))
parser.add_argument('--visibility',type=int,choices=(0,1))
parser.add_argument('--portal-drawing',type=int,choices=(0,1))
parser.add_argument('--windowed',action='store_true',help='windowed run; still requires the exact requested world extent')
parser.add_argument('--size',type=int,nargs=2,default=(2560,1440),metavar=('WIDTH','HEIGHT'))
parser.add_argument('--trace',type=Path)
parser.add_argument('--log',type=Path)
parser.add_argument('--output',type=Path)
parser.add_argument('--route',action='store_true',help='Linux native input route; requires complete 480-frame cycles after warmup')
options=parser.parse_args()
if min(options.size)<=0:parser.error('--size dimensions must be positive')
if options.raytracing==0:parser.error('this tool measures modern lighting; disabled RTX falls back to the native renderer')
if options.frames<4 or not 0<=options.warmup<options.frames-2:
    parser.error('require at least four frames and two measured frames after warmup')
if options.route and (options.frames-options.warmup<480 or (options.frames-options.warmup)%480):
    parser.error('--route requires one or more complete 480-frame cycles after warmup')

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
    if options.route:
        # Resolve the exact executable's C entry point; do not patch game code
        # or attach a debugger during timing. The preload runs on its submit thread.
        binary=options.build.resolve()/'thief-runner'
        symbols=subprocess.run(['nm','--defined-only','--format=posix',str(binary)],capture_output=True,text=True,check=True).stdout
        symbol=re.search(r'^DarkInputKey T ([0-9a-fA-F]+) ',symbols,re.M)
        if symbol is None:raise SystemExit('--route requires the unstripped Linux executable with DarkInputKey')
        hook=folder/'route.so'
        subprocess.run(['c++','-std=c++17','-shared','-fPIC',str(Path(__file__).with_name('native_route_hook.cpp')),
                        f'-DDARK_INPUT_KEY_OFFSET=0x{symbol[1]}','-ldl','-o',str(hook)],check=True)
        environment['LD_PRELOAD']=str(hook)+(':'+environment['LD_PRELOAD'] if environment.get('LD_PRELOAD') else '')
        environment['DARK_TEST_ROUTE_WARMUP']=str(options.warmup)
    (folder/'saves').mkdir()
    args=[str(options.build.resolve()/'thief-runner'),f'game_path={options.game_path.resolve()}',f'save_path={folder/"saves"}',
          'content=vanilla',f'mission_loop_test={options.mission}',f'auto_stop={options.frames}',
          'renderer_mode=modern',f'game_screen_size={options.size[0]} {options.size[1]}','game_screen_fit=0',
          'SlowFrame=0','min_frame_time=0','vsync=0',f'game_full_screen={0 if options.windowed else 1}']
    for key,value in [('renderer_dlss',options.dlss),('renderer_raytracing',options.raytracing),('renderer_gi',options.gi),
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
    if options.dlss and options.dlss!='off' and f'DLSS enabled: {options.dlss}' not in text:
        raise SystemExit('requested DLSS mode was not enabled')
    upscaling={tuple(map(int,row)) for row in re.findall(r'world-upscaling dlss=(\d+) render=(\d+)x(\d+) output=(\d+)x(\d+)',timings)}
    patterns={
        'di':(r'world-di-phases initial_ms=([\d.-]+) temporal_ms=([\d.-]+) spatial_shading_ms=([\d.-]+)',
              ['di_initial_gpu','di_temporal_gpu','di_spatial_shading_gpu']),
        'gpu':(r'world-gpu-phases surface_ms=([\d.-]+) di_ms=([\d.-]+) gi_ms=([\d.-]+) nrd_ms=([\d.-]+) history_ms=([\d.-]+) composition_ms=([\d.-]+) transparency_ms=([\d.-]+)',
               ['surface_gpu','di_gpu','gi_gpu','nrd_gpu','history_gpu','composition_gpu','transparency_gpu']),
        'frame':(r'frame-pacing modern=1 interval_ms=([\d.-]+) render_submit_ms=([\d.-]+) acquire_ms=([\d.-]+) canvas_ms=([\d.-]+) submit_present_ms=([\d.-]+)',
                 ['interval','render_submit','acquire','canvas','submit_present']),
        'world':(r'world-lighting .*previous_world_gpu_ms=([\d.-]+) record_cpu_ms=([\d.-]+)',
                 ['world_gpu','record_cpu']),
        'cpu':(r'world-record-cpu setup_ms=([\d.-]+) uploads_tiles_ms=([\d.-]+) ray_scene_ms=([\d.-]+) lighting_ms=([\d.-]+)',
               ['setup_cpu','preparation_cpu','ray_scene_cpu','lighting_cpu']),
        'capture':(r'world-capture cpu_ms=([\d.-]+) preceding_native_ms=([\d.-]+)',
                   ['capture_cpu','preceding_native_cpu'])}
    samples={key:[tuple(map(float,row)) for row in re.findall(pattern,timings)]
             for key,(pattern,_) in patterns.items()}
    if (process.returncode or errors or 'Modern world lighting enabled' not in text or
        any(len(samples[key])!=options.frames for key in ('world','cpu','capture','gpu','di')) or
        not options.frames-1<=len(samples['frame'])<=options.frames or
        any(len(rows)<=options.warmup for rows in samples.values())):
        print(text[-6000:]);raise SystemExit('native performance run failed completeness/validation checks')
    extents={tuple(map(int,xy)) for xy in re.findall(r'world-lighting .*extent=(\d+)x(\d+)',timings)}
    if extents!={tuple(options.size)}:raise SystemExit(f'world extent mismatch: requested {options.size}, measured {sorted(extents)}')
    motion=None
    if options.route:
        report=re.search(r'Fixture native route world_frames=(\d+) input_events=(\d+)',text)
        cameras=[tuple(map(float,values)) for values in re.findall(
            r'world-camera eye=([\d.eE+-]+),([\d.eE+-]+),([\d.eE+-]+) forward=([\d.eE+-]+),([\d.eE+-]+),([\d.eE+-]+)',timings)]
        measured=cameras[options.warmup:]
        travel=sum(math.dist(a[:3],b[:3]) for a,b in zip(measured,measured[1:]))
        span=math.dist([min(p[i] for p in measured) for i in range(3)],
                       [max(p[i] for p in measured) for i in range(3)]) if measured else 0
        horizontal_span=math.hypot(max(p[0] for p in measured)-min(p[0] for p in measured),
                                   max(p[1] for p in measured)-min(p[1] for p in measured)) if measured else 0
        turn=max((math.dist(a[3:],b[3:]) for a,b in zip(measured,measured[1:])),default=0)
        forward_span=max((math.dist(measured[0][3:],p[3:]) for p in measured),default=0) if measured else 0
        motion={'path_length':travel,'position_span':span,'horizontal_span':horizontal_span,
                'forward_span':forward_span,'max_forward_step':turn,
                'input_events':int(report[2]) if report else 0}
        # The final auto_stop frame can be recorded without being presented.
        if (not report or not options.frames-1<=int(report[1])<=options.frames or
            int(report[2])<16*((options.frames-options.warmup)//480) or
            len(cameras)!=options.frames or travel<2 or horizontal_span<1 or forward_span<.1):
            raise SystemExit(f'native route did not prove movement/turning: {motion}, cameras={len(cameras)}')
    result={'mission':options.mission,'frames':options.frames,'warmup':options.warmup,
            'configuration':{'raytracing':options.raytracing,'gi':options.gi,'requested_extent':options.size,'visibility':options.visibility,
                             'portal_drawing':options.portal_drawing,'route':options.route},'command':args,'measured_extents':sorted(extents),
            'motion':motion,
            'limitations':('Scripted native input near the mission start, no GDB; not broad mission coverage.' if options.route else 'Stationary view, no GDB; not traversal FPS.')+
                          ' Presentation/compositor, tracing and hardware clocks affect timings.',
            'metrics':{}}
    result['dlss']=options.dlss or 'off'
    result['upscaling']=[{'enabled':bool(row[0]),'render_size':row[1:3],'output_size':row[3:5]} for row in sorted(upscaling)]
    for group,(_,names) in patterns.items():
        rows=samples[group][options.warmup:]
        for index,name in enumerate(names):
            values=[row[index] for row in rows if row[index]>=0]
            if not values:
                raise SystemExit(f'no valid {name} timing samples')
            result['metrics'][name]=summarize(values)
    intervals=[row[0] for row in samples['frame'][options.warmup:] if row[0]>0]
    result['frame_budget']={'budget_ms':1000/60,'over_budget':sum(value>1000/60 for value in intervals),
                            'samples':len(intervals),'fps_from_mean_interval':1000/statistics.mean(intervals)}
    output=json.dumps(result,indent=2)+'\n'
    if options.output:options.output.write_text(output)
    print(output,end='')
