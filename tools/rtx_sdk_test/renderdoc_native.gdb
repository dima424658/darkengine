set pagination off
set confirm off
set debuginfod enabled off
set startup-with-shell off
set $capture_frame=0
set $capture_world_frame=0
set $capture_weapon_requested=0
if $_isvoid($capture_equip_sword)
set $capture_equip_sword=0
end
if $_isvoid($capture_first)
set $capture_first=32
end
if $_isvoid($capture_last)
set $capture_last=$capture_first+1
end
break RenderBackendRenderWorld
commands
silent
set $capture_world_frame=1
if $capture_equip_sword && !$capture_weapon_requested
call ((char*(*)(const char*)) &CommandExecute)("inv_select sword")
set $capture_weapon_requested=1
end
continue
end
break DarkRenderEndApplicationFrame
commands
silent
if $capture_world_frame
set $capture_world_frame=0
set $capture_frame=$capture_frame+1
if $capture_frame==$capture_first
if $capture_equip_sword
set $capture_owner=*(int*)&gPlayerObj
set $capture_arm=((int(*)())&PlayerArm)()
set $capture_weapon=((int(*)(int))&GetWeaponObjID)($capture_owner)
set $capture_equipped=((int(*)(int,int))&IsWeaponEquipped)($capture_owner,$capture_weapon)
printf "Fixture foreground owner=%d arm=%d weapon=%d equipped=%d\n",$capture_owner,$capture_arm,$capture_weapon,$capture_equipped
if $capture_arm==0 || $capture_weapon==0 || !$capture_equipped
quit 3
end
end
call ((void(*)())dark_capture_start)()
end
if $capture_frame==$capture_last
call ((void(*)())dark_capture_end)()
end
end
continue
end
run
