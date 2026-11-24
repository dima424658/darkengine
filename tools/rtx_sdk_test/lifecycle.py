#!/usr/bin/env python3
"""Linux/GDB: verify SDK rendering through resize and a private save/load cycle."""
import argparse
import json
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
parser.add_argument('--case', choices=('all', 'resize', 'save-load'), default='all')
parser.add_argument('--log-dir', type=Path)
options = parser.parse_args()
if options.log_dir:
    options.log_dir.mkdir(parents=True, exist_ok=True)

for case in ('resize', 'save-load') if options.case == 'all' else (options.case,):
    with tempfile.TemporaryDirectory(prefix='dark-sdk-lifecycle-') as temporary:
        folder = Path(temporary)
        (folder / 'saves').mkdir()
        save = json.dumps(str(folder / 'saves/lifecycle.sav'))
        script = folder / 'lifecycle.gdb'
        script.write_text('''set pagination off
set confirm off
set debuginfod enabled off
set startup-with-shell off
set $frames=0
set $world_frame=0
set $saved=-1
set $loaded=-1
set $resized=0
break RenderBackendRenderWorld
commands
silent
set $world_frame=1
continue
end
break DarkRenderEndApplicationFrame
commands
silent
if $world_frame
set $world_frame=0
set $frames=$frames+1
''' + ('''if $frames==2
call ((int(*)(void*,int,int))SDL_SetWindowSize)(((void*(*)())'SDLDisplayGetWindow()')(),1600,900)
end
if $frames==10
set $resized=((int(*)(void*,int,int))SDL_SetWindowSize)(((void*(*)())'SDLDisplayGetWindow()')(),1280,720)
end
''' if case == 'resize' else f'''if $frames==30
set $saved=((long(*)(const char*,const char*))DarkSaveGameFile)("SDK lifecycle regression",{save})
end
if $frames==60 && $saved==0
set $loaded=((long(*)(const char*))DarkQuickLoadGameFile)({save})
end
''') + '''end
continue
end
run
printf "Fixture lifecycle frames=%u saved=%ld loaded=%ld resized=%d\\n",$frames,$saved,$loaded,$resized
''')
        log = folder / 'game.log'
        trace = folder / 'trace.log'
        environment = os.environ.copy()
        environment['DARK_RENDER_TRACE'] = str(trace)
        command = ['gdb', '-q', '-batch', '-x', str(script), '--args',
                   str(options.build.resolve() / 'thief-runner'),
                   f'game_path={options.game_path.resolve()}', f'save_path={folder / "saves"}',
                   'content=vanilla', 'mission_loop_test=2', 'renderer_mode=modern',
                   'renderer_raytracing=1', 'renderer_gi=1', 'game_full_screen=0',
                   'SlowFrame=0', 'min_frame_time=0', 'vsync=0',
                   f'auto_stop={40 if case == "resize" else 180}']
        with log.open('w') as output:
            process = subprocess.Popen(command, env=environment, stdout=output,
                                       stderr=subprocess.STDOUT, start_new_session=True)
            timed_out = False
            try:
                process.wait(timeout=120)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGTERM)
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.wait()
                timed_out = True
        text = log.read_text(errors='replace')
        timings = trace.read_text(errors='replace') if trace.exists() else ''
        if options.log_dir:
            (options.log_dir / f'{case}.log').write_text(text)
            (options.log_dir / f'{case}.trace').write_text(timings)
        if timed_out:
            print(text[-10000:])
            raise SystemExit(f'{case}: lifecycle fixture timed out')
        match = re.search(r'Fixture lifecycle frames=(\d+) saved=(-?\d+) loaded=(-?\d+) resized=(\d+)', text)
        errors = any(marker in text or marker in timings for marker in
                     ('Validation Error', 'VUID-', 'SYNC-HAZARD', 'FATAL:', 'Modern world disabled:'))
        extents = {tuple(map(int, xy)) for xy in re.findall(r'world-lighting .*extent=(\d+)x(\d+)', timings)}
        valid = process.returncode == 0 and 'exited normally' in text and match is not None and not errors
        if valid and case == 'resize':
            valid = int(match[1]) >= 30 and match[4] == '1' and len(extents) >= 2 and (1280, 720) in extents
        if valid and case == 'save-load':
            modern_frames=timings.count('frame-pacing modern=1 ')
            valid = int(match[1]) >= 180 and modern_frames>=180 and match[2] == match[3] == '0'
        if not valid:
            print(text[-10000:])
            raise SystemExit(f'{case}: lifecycle checks failed (extents={sorted(extents)})')
        print(f'PASS {case}: {match[0]}, world extents={sorted(extents)}; no validation errors')
