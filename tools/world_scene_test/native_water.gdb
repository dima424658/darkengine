set pagination off
set confirm off
set debuginfod enabled off
set startup-with-shell off
break WorldSceneCaptureCamera
commands
silent
python
import gdb,struct
camera=int(gdb.parse_and_eval("((void*(*)())&'get_player_camera()')()"))
gdb.execute('call ((void(*)(void*))&CameraDetach)((void*)%d)'%camera)
# Linux x86-64 native Camera: position at +8, orientation at +20.
gdb.selected_inferior().write_memory(camera+8,struct.pack('<3f',376,-8,-8))
gdb.selected_inferior().write_memory(camera+20,struct.pack('<3H',0,8192,0))
end
disable 1
continue
end
python
import gdb,struct
state={'batches':0,'draws':0,'suppressed':0,'late_draws':0,'late_suppressed':0}
active=None
late_callbacks=set()
class WaterDraw(gdb.Breakpoint):
    def stop(self):
        if active is None and int(gdb.parse_and_eval('*(unsigned char*)&modernAlphaPass'))==0:
            return False # Opaque native model calls are deliberately suppressed.
        prefix='' if active=='water' else 'late_'
        state[prefix+'draws']+=1
        if int(gdb.parse_and_eval("*(unsigned char*)&'(anonymous namespace)::nativeWorldDrawing'"))==0:
            state[prefix+'suppressed']+=1
        return False
water_draw=WaterDraw('RenderBackendDraw',internal=True)
water_draw.enabled=False
class EndEffect(gdb.FinishBreakpoint):
    def __init__(self,previous,enabled):
        super().__init__(gdb.newest_frame(),internal=True)
        self.previous=previous
        self.enabledBefore=enabled
    def stop(self):
        global active
        active=self.previous
        water_draw.enabled=self.enabledBefore
        return False
class EffectBatch(gdb.Breakpoint):
    def __init__(self,symbol,kind):
        super().__init__(symbol,internal=True)
        self.kind=kind
    def stop(self):
        global active
        previous=active
        enabledBefore=water_draw.enabled
        if self.kind is None:
            callback=int(gdb.parse_and_eval('*(void**)&next_post_render_cback'))
            if callback and callback not in late_callbacks:
                late_callbacks.add(callback)
                print('Fixture chained callback '+gdb.execute('info symbol '+hex(callback),to_string=True).strip())
                EffectBatch('*'+hex(callback),'late')
        active=self.kind
        if self.kind=='water':state['batches']+=1
        water_draw.enabled=True
        EndEffect(previous,enabledBefore)
        return False
# Opaque object calls nested inside a post-render callback are intentionally
# suppressed in modern-only mode; they are not late effects.
EffectBatch('rq_render_queue',None)
EffectBatch('portal_render_water_polys','water')
EffectBatch('CoronaRender','late')
EffectBatch('WeatherRender','late')
end
run
python
print('Fixture native water '+' '.join(str(k)+'='+str(v) for k,v in state.items()))
if gdb.selected_inferior().threads():
    gdb.execute('bt')
if not state['batches'] or not state['draws'] or state['suppressed'] or not state['late_draws'] or state['late_suppressed']:
    gdb.execute('quit 3')
end
