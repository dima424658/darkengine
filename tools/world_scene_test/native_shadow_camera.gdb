set pagination off
set confirm off
set debuginfod enabled off
set startup-with-shell off
set $fixture_frame=0
break cam_render_scene
commands
silent
set $fixture_frame=$fixture_frame+1
python
import gdb,struct
step=int(gdb.parse_and_eval('$fixture_yaw_step'))
frame=int(gdb.parse_and_eval('$fixture_frame'))
if step:
    # Linux x86-64 Position: 16-byte Location, then three uint16 angles.
    # Modify only the incoming render-camera copy, never player/camera ownership.
    position=int(gdb.parse_and_eval('$rdi'))
    gdb.selected_inferior().write_memory(position+20,struct.pack('<H',(step*frame)&65535))
end
continue
end
run
printf "Fixture shadow camera frames=%u yaw_step=%d\n",$fixture_frame,$fixture_yaw_step
