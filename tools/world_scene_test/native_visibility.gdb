set pagination off
set confirm off
set debuginfod enabled off
set $fixture_frame=0
break RenderBackendRenderWorld
commands
silent
set $fixture_frame=$fixture_frame+1
set $module=((void*(*)(const char*,int)) &dlopen)($fixture_helper,2)
set $check=(int(*)(void*,int))((void*(*)(void*,const char*)) &dlsym)($module,"CheckTerrainVisibility")
set $scene=((void*(*)()) &'dark::GetWorldScene()')()
set $passed=$check($scene,$fixture_visibility)
if !$passed
quit 3
end
if $fixture_visibility
set $model_check=(int(*)(void*,void*,void*,unsigned,void*,unsigned,void*))((void*(*)(void*,const char*)) &dlsym)($module,"CheckModelVisibility")
set $system=*(int*)&mCellRefSys
set $firsts=(*(void***)&gFirstRefs)[$system]
set $infos=*(void**)&ObjRefInfos
set $models=$model_check($scene,$firsts,$infos,*(unsigned*)&gMaxRefs,(void*)&active_regions,*(unsigned*)&r_sorted_count,(void*)&ObjPosGet)
if !$models
quit 5
end
end
if $fixture_visibility && $fixture_frame==1
call ((void(*)(void*,unsigned)) &WorldSceneCaptureVisibleCells)(0,0)
call ((void(*)()) &WorldSceneCapture)()
set $fallback=$check($scene,0)
if !$fallback
quit 4
end
end
continue
end
run
