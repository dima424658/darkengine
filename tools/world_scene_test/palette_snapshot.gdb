set pagination off
set confirm off
set debuginfod enabled off
break WorldSceneCaptureModels
commands
silent
set $helper=((void*(*)(const char*,int)) &dlopen)($fixture_helper,2)
set $read=(unsigned(*)(void*,unsigned long long))((void*(*)(void*,const char*)) &dlsym)($helper,"ReadSceneTexture")
set $scene=((void*(*)()) &'dark::GetWorldScene()')()
set $bitmap=((char*(*)(unsigned long,unsigned long)) &calloc)(1,32)
set $bits=((char*(*)(unsigned long)) &malloc)(1)
set *$bits=1
set *(void**)$bitmap=$bits
set *(unsigned char*)($bitmap+8)=2
set *(unsigned char*)($bitmap+9)=255
set *(short*)($bitmap+12)=1
set *(short*)($bitmap+14)=1
set *(short*)($bitmap+16)=1
set $pal=((char*(*)(unsigned long)) &malloc)(3)
set *(unsigned char*)$pal=255
set *(unsigned char*)($pal+1)=0
set *(unsigned char*)($pal+2)=0
set $oldr=*((unsigned char*)&g_palettes+255*768+3)
set $oldg=*((unsigned char*)&g_palettes+255*768+4)
set $oldb=*((unsigned char*)&g_palettes+255*768+5)
set $setpal=(void(*)(unsigned,unsigned,void*,int)) &lgd3d_set_pal_slot
call $setpal(1,1,$pal,255)
set $copy=(unsigned long long(*)(void*)) &'(anonymous namespace)::texture(grs_bitmap*)'
set $handle=$copy($bitmap)
set $red=$read($scene,$handle)
set *(unsigned char*)$pal=0
set *(unsigned char*)($pal+2)=255
call $setpal(1,1,$pal,255)
set $next=$copy($bitmap)
set $blue=$read($scene,$next)
set $revision=(unsigned long long(*)(void*)) &lgd3d_scene_palette_revision
set $before=$revision($bitmap)
call $setpal(1,1,$pal,255)
set $after=$revision($bitmap)
printf "Fixture stable palette revision=%llu->%llu\n",$before,$after
set *(unsigned char*)$pal=$oldr
set *(unsigned char*)($pal+1)=$oldg
set *(unsigned char*)($pal+2)=$oldb
call $setpal(1,1,$pal,255)
printf "Fixture unloaded palette handle=%llu->%llu red=0x%x blue=0x%x\n",$handle,$next,$red,$blue
disable 1
continue
end
run
