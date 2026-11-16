set pagination off
set confirm off
set debuginfod enabled off
break RenderBackendRenderWorld
commands
silent
set $module=((void*(*)(const char*,int)) &dlopen)($fixture_helper,2)
set $inspect=(void(*)(void*,void*,int))((void*(*)(void*,const char*)) &dlsym)($module,"DiagnoseTorchSelection")
set $scene=((void*(*)()) &'dark::GetWorldScene()')()
call $inspect($scene,(void*)&ObjPosGet,$fixture_cache)
disable 1
continue
end
run
