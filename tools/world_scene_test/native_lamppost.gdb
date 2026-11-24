set pagination off
set confirm off
set debuginfod enabled off
break RenderBackendRenderWorld
commands
silent
set $module=((void*(*)(const char*,int)) &dlopen)($fixture_helper,2)
set $check=(int(*)(void*,void*))((void*(*)(void*,const char*)) &dlsym)($module,"CheckNativeLamppost")
set $scene=((void*(*)()) &'dark::GetWorldScene()')()
set $point=((void*(*)(unsigned long,unsigned long)) &calloc)(3,4)
set $found=((int(*)(void*,int,int)) &FindVhotLocation)($point,234,1)
if !$found
quit 2
end
set $passed=$check($scene,$point)
call ((void(*)(void*)) &free)($point)
if !$passed
quit 3
end
disable 1
continue
end
run
