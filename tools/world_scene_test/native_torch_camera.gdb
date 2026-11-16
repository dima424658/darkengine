set pagination off
set confirm off
set startup-with-shell off
set debuginfod enabled off
set $frame=0
break cam_render_scene
commands
silent
set $frame=$frame+1
set $pos=(void*)$rdi
if $frame==1
set $module=((void*(*)(const char*,int)) &dlopen)($fixture_helper,2)
set $move=(int(*)(void*,void*,void*,unsigned))((void*(*)(void*,const char*)) &dlsym)($module,"MoveTorchCamera")
end
set $passed=$move($pos,(void*)&ObjPosGet,(void*)&ComputeCellForLocation,$frame)
if !$passed
quit 3
end
continue
end
break RenderBackendRenderWorld
commands
silent
set $scene=((void*(*)()) &'dark::GetWorldScene()')()
set $init=(int(*)(void*,void*,void*))((void*(*)(void*,const char*)) &dlsym)($module,"InitTorchRoute")
set $found=$init($scene,(void*)&ObjPosGet,(void*)&ComputeCellForLocation)
if !$found
quit 4
end
disable 2
continue
end
break dark::VulkanWorldRenderer::Composite
commands
silent
set $renderer=(void*)$rdi
set $selection=((void*(*)(void*)) &'dark::VulkanWorldRenderer::GetShadowSelection() const')($renderer)
set $scene=((void*(*)()) &'dark::GetWorldScene()')()
set $report=(int(*)(void*,void*,unsigned))((void*(*)(void*,const char*)) &dlsym)($module,"ReportTorchSelection")
set $state=$report($scene,$selection,$frame)
continue
end
run
