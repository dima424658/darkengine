#!/usr/bin/env python3
"""Linux x86-64/GDB: observe real creature deformation in the modern world scene."""
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
parser.add_argument('--build', type=Path, default=root/'build-linux')
parser.add_argument('--mission', type=int, choices=(1, 2), default=1)
parser.add_argument('--frames', type=int, default=240)
parser.add_argument('--log', type=Path)
parser.add_argument('--capture-prefix', type=Path,
                    help='capture a close animated-creature frame using installed RenderDoc/X11')
parser.add_argument('--test-light', action='store_true',
                    help='add a controlled point source beside the captured creature (requires capture)')
parser.add_argument('--capture-frames', type=int, default=1,
                    help='number of consecutive captured world frames, 1–16 (default 1)')
options = parser.parse_args()
if options.frames < 4:
    parser.error('--frames must be at least 4')
if options.test_light and not options.capture_prefix:
    parser.error('--test-light requires --capture-prefix')
if not 1 <= options.capture_frames <= 16:
    parser.error('--capture-frames must be 1–16')
with tempfile.TemporaryDirectory(prefix='dark-native-animation-') as temporary:
    folder = Path(temporary)
    (folder/'saves').mkdir()
    hook = folder/'observer.so'
    subprocess.run(['c++', '-std=c++17', '-shared', '-fPIC', '-O2',
                    '-I'+str(root/'h'), str(Path(__file__).with_suffix('.cpp')),
                    '-ldl', '-o', str(hook)], check=True)
    script = folder/'observe.gdb'
    test_light_script = '''set $fixture_light=((int(*)(void*,void*))&dark_add_creature_light)((void*)&RenderSceneCreateLight,(void*)&RenderSceneSetInstanceLights)
if !$fixture_light
quit 3
end
''' if options.test_light else ''
    capture_script = '''set $animation_frame=0
break WorldSceneCaptureCamera
commands
silent
set $animation_frame=$animation_frame+1
if $animation_frame==32
set $animation_camera=((void*(*)())&'get_player_camera()')()
call ((void(*)(void*))&CameraDetach)($animation_camera)
set $animation_object=((int(*)(void*,void*))&dark_aim_at_creature)($animation_camera,((void*(*)())&'dark::GetWorldScene()')())
if !$animation_object
quit 3
end
''' + test_light_script + '''
end
continue
end
break DarkRenderEndApplicationFrame
commands
silent
if $animation_frame==80
call ((void(*)())&dark_capture_start)()
end
if $animation_frame==CAPTURE_END
call ((void(*)())&dark_capture_end)()
end
continue
end
''' if options.capture_prefix else ''
    capture_script = capture_script.replace('CAPTURE_END', str(80+options.capture_frames))
    if options.capture_prefix and options.frames < 83+options.capture_frames:
        parser.error('capture requires at least 83 + --capture-frames total frames')
    script.write_text('''set pagination off
set confirm off
set debuginfod enabled off
set startup-with-shell off
break RenderBackendRenderWorld
commands
silent
call ((void(*)(void*,void*))&dark_observe_animation)(((void*(*)())&'dark::GetWorldScene()')(),(void*)&CreatureGetRenderHandle)
call ((void(*)())&dark_report_animation)()
continue
end
run
'''.replace('run\n', capture_script+'run\n'))
    environment = os.environ.copy()
    environment['DARK_RENDER_TRACE'] = str(folder/'trace')
    # Apply the observer only to the inferior, not to GDB itself.
    preload = str(hook)+(':'+environment['LD_PRELOAD'] if environment.get('LD_PRELOAD') else '')
    capture_before = {}
    if options.capture_prefix:
        prefix = options.capture_prefix.resolve()
        capture_before = {p: p.stat().st_mtime_ns for p in prefix.parent.glob(prefix.name+'*.rdc')}
        capture = folder/'capture.so'
        subprocess.run(['cc', '-shared', '-fPIC', str(Path(__file__).with_name('renderdoc_native.c')),
                        '-ldl', '-o', str(capture)], check=True)
        preload += ':'+str(capture)+':librenderdoc.so'
        environment['SDL_VIDEODRIVER'] = 'x11'
        environment['DARK_NATIVE_CAPTURE'] = str(options.capture_prefix.resolve())
    command = ['gdb', '-q', '-batch', '-ex', 'set environment LD_PRELOAD '+preload,
               '-x', str(script), '--args', str(options.build.resolve()/'thief-runner'),
               f'game_path={options.game_path.resolve()}', f'save_path={folder/"saves"}',
               'content=vanilla', f'mission_loop_test={options.mission}',
               f'auto_stop={options.frames}', 'renderer_mode=modern',
               'SlowFrame=0', 'min_frame_time=0', 'vsync=0', 'game_full_screen=0']
    with (folder/'log').open('w') as log:
        process = subprocess.Popen(command, env=environment, stdout=log,
                                   stderr=subprocess.STDOUT, start_new_session=True)
        timed_out = False
        try:
            process.wait(timeout=120)
        except subprocess.TimeoutExpired:
            timed_out = True
            os.killpg(process.pid, signal.SIGTERM)
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
    text = (folder/'log').read_text(errors='replace')
    trace = (folder/'trace').read_text(errors='replace') if (folder/'trace').exists() else ''
    if options.log:
        options.log.write_text(text+'\n'+trace)
    if options.capture_prefix:
        captures = [p for p in prefix.parent.glob(prefix.name+'*.rdc')
                    if p.stat().st_size and capture_before.get(p) != p.stat().st_mtime_ns]
        if ('Native RenderDoc API available=1' not in text or
                'Native RenderDoc capture result=1' not in text or not captures):
            print(text)
            raise SystemExit('native creature capture failed')
        print('Capture: '+', '.join(str(p) for p in captures))
        if options.test_light and not re.search(r'Fixture added creature light=[1-9]\d* ', text):
            raise SystemExit('controlled creature light was not created')
    rows = list(re.finditer(r'Fixture animation frames=(\d+) creatures=(\d+) animated=(\d+) changes=(\d+) visible_changes=(\d+) max_delta=([\d.]+)', text))
    row = rows[-1] if rows else None
    ray_rows = list(re.finditer(r'Fixture animation ray_builds=(\d+) changed_ray_builds=(\d+) tlas_creature_instances=(\d+)', text))
    ray_row = ray_rows[-1] if ray_rows else None
    builds = [int(n) for n in re.findall(r'world-ray-scene blas_builds=(\d+)', trace)]
    errors = re.search(r'Validation Error|VUID-|SYNC-HAZARD|FATAL:|Modern world disabled:', text+trace)
    if (timed_out or process.returncode or 'exited normally' not in text or not row or errors or
            int(row[1]) < options.frames-1 or not int(row[3]) or
            float(row[6]) <= .0001 or not any(builds[2:]) or not ray_row or
            not all(int(ray_row[i]) for i in (1, 2, 3))):
        print(text)
        raise SystemExit('native creature deformation check failed')
    print(row[0])
    print(ray_row[0])
    print('PASS native creature deformation, matching BLAS upload bytes and TLAS references; not a visual-quality check')
