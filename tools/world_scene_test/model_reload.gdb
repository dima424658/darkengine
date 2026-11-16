set pagination off
set confirm off
set debuginfod enabled off
set $frame=0
break WorldSceneCaptureModels
commands
silent
set $frame=$frame+1
if $frame==2
set $out=((void*(*)(unsigned long)) &malloc)(4)
set $found=((int(*)(int,void*)) &ObjGetModelNumber)(11,$out)
set $index=*(int*)$out
set $revision=(unsigned long long(*)(int)) &objmodelGetModelRevision
set $data=(void*(*)(int)) &objmodelGetModel
set $size=(unsigned long(*)(int)) &objmodelGetModelDataSize
set $oldrev=$revision($index)
set $olddata=$data($index)
set $oldsize=$size($index)
set $name=((char*(*)(const char*)) &strdup)(((char*(*)(int)) &objmodelGetName)($index))
# Linux x86-64 native ModelHandle layout: two pointers, ushort numtex/refs.
# Restore reference ownership exactly as objmodelSwitchHighPoly does.
set $refs=(unsigned short*)((char*)&g_aModelHandles+$index*32+18)
set $oldrefs=*$refs
# Hold the native resource through its IUnknown/IDataSource slots while the
# model handle reloads. This prevents allocator movement and resource eviction.
set $resource=*(void**)((char*)&g_aModelHandles+$index*32)
set $vtable=*(void***)$resource
set $held=((unsigned(*)(void*))$vtable[1])($resource)
set $locked=((void*(*)(void*))$vtable[3])($resource)
set $emptyrev=$revision(-1)
set $loaded=((int(*)(char*,int)) &objmodelLoadModelIntoHandle)($name,$index)
set *$refs=$oldrefs
call ((void(*)(void*))$vtable[4])($resource)
set $released=((unsigned(*)(void*))$vtable[2])($resource)
set $newrev=$revision($index)
set $newdata=$data($index)
set $newsize=$size($index)
printf "Fixture model reload obj=11 index=%d found=%d loaded=%d revision=%llu->%llu invalid=%llu same_pointer=%d same_size=%d refs=%u\n",$index,$found,$loaded,$oldrev,$newrev,$emptyrev,$olddata==$newdata,$oldsize==$newsize,$oldrefs
call ((void(*)(void*)) &free)($out)
call ((void(*)(void*)) &free)($name)
end
if $frame==3
printf "Fixture stable revision=%llu modern_has_model=%d\n",$revision($index),((int(*)(int)) &WorldSceneHasModel)(11)
end
continue
end
run
