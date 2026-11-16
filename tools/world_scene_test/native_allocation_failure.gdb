set pagination off
set confirm off
set debuginfod enabled off
set startup-with-shell off
python
import gdb
state = {'armed': 0, 'failed': 0, 'native_draws': 0, 'suppressed': 0}
class ArmFailure(gdb.Breakpoint):
    def stop(self):
        self.enabled = False
        state['armed'] += 1
        gdb.execute('set *(unsigned*)&dark_skip_allocations=$fixture_skip', to_string=True)
        gdb.execute('set *(int*)&dark_fail_allocations=1', to_string=True)
        return False
class NativeDraw(gdb.Breakpoint):
    def stop(self):
        if int(gdb.parse_and_eval('*(unsigned*)&dark_failed_allocations')):
            state['failed'] = int(gdb.parse_and_eval('*(unsigned*)&dark_failed_allocations'))
            state['native_draws'] += 1
        return False
class NativeDrawing(gdb.Breakpoint):
    def stop(self):
        if int(gdb.parse_and_eval('*(unsigned*)&dark_failed_allocations')) and int(gdb.parse_and_eval('$rdi')) == 0:
            state['suppressed'] += 1
        return False
ArmFailure('RenderBackendRenderWorld', internal=True)
NativeDraw('draw_region_lgd3d', internal=True)
NativeDrawing('RenderBackendSetNativeWorldDrawing', internal=True)
end
run
python
print('Fixture allocation failure '+ ' '.join(str(k)+'='+str(v) for k,v in state.items()))
if gdb.selected_inferior().threads():
    gdb.execute('bt')
if state['armed'] != 1 or state['failed'] != 1 or state['native_draws'] == 0 or state['suppressed']:
    gdb.execute('quit 3')
end
