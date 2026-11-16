set pagination off
set confirm off
set debuginfod enabled off
set $captureframe=0
set $renderframe=0
break WorldSceneCapture
commands
silent
set $captureframe=$captureframe+1
if $captureframe==2
set $position=((void*(*)(int)) &ObjPosGet)(11)
set $location=((void*(*)(unsigned long)) &malloc)(16)
set $copied=((void*(*)(void*,void*,unsigned long)) &memcpy)($location,$position,16)
set $oldr=*(int*)&hack_red
set $oldg=*(int*)&hack_green
set $oldb=*(int*)&hack_blue
set *(int*)&hack_red=255
set *(int*)&hack_green=255
set *(int*)&hack_blue=255
call ((void(*)(void*,float,float)) &add_dynamic_light)($location,128.0,1.0)
set *(int*)&hack_red=$oldr
set *(int*)&hack_green=$oldg
set *(int*)&hack_blue=$oldb
call ((void(*)(void*)) &free)($location)
end
continue
end
break RenderBackendRenderWorld
commands
silent
set $renderframe=$renderframe+1
python
import gdb,struct
inf=gdb.selected_inferior()
def mem(a,n):return bytes(inf.read_memory(a,n))
def value(s):return int(gdb.parse_and_eval(s))
frame=value('$renderframe')
if frame==1:
    module=value('((void*(*)(const char*,int))&dlopen)($fixture_helper,2)')
    gdb.set_convenience_variable('reader_module',gdb.Value(module))
    gdb.set_convenience_variable('check_object',gdb.Value(value('((void*(*)(void*,const char*))&dlsym)((void*)%d,"CheckObjectLightList")'%module)))
    gdb.set_convenience_variable('check_dynamic',gdb.Value(value('((void*(*)(void*,const char*))&dlsym)((void*)%d,"CheckDynamicLight")'%module)))
scene=value("((void*(*)())&'dark::GetWorldScene()')()")
static=value('*(int*)&num_light');dynamic=value('*(int*)&num_dyn')
position=value('((void*(*)(int))&ObjPosGet)(11)')
cell=struct.unpack_from('<h',mem(position,16),12)[0]
if cell<0:
    copy=value('((void*(*)(unsigned long))&malloc)(16)')
    inf.write_memory(copy,mem(position,16))
    cell=value('((int(*)(void*))&ComputeCellForLocation)((void*)%d)'%copy)
    gdb.execute('call ((void(*)(void*))&free)((void*)%d)'%copy,to_string=True)
assert cell>=0,cell
base=value('(void*)&wr_cell');cellptr=struct.unpack('<Q',mem(base+cell*8,8))[0]
indices=struct.unpack('<Q',mem(cellptr+104,8))[0]
count=struct.unpack('<H',mem(indices,2))[0]
ids=list(struct.unpack('<%dH'%count,mem(indices+2,count*2))) if count else []
vector=value("*(void**)&'(anonymous namespace)::lightHandles'")
start=struct.unpack('<Q',mem(vector,8))[0]
handles=list(struct.unpack('<%dQ'%(static+dynamic),mem(start,(static+dynamic)*8)))
expected=sorted(set([handles[i] for i in ids]+handles[static:]))
assert expected
buffer=value('((void*(*)(unsigned long))&malloc)(%d)'%(len(expected)*8))
inf.write_memory(buffer,struct.pack('<%dQ'%len(expected),*expected))
parts=value('((unsigned(*)(void*,int,void*,unsigned))$check_object)((void*)%d,11,(void*)%d,%d)'%(scene,buffer,len(expected)))
gdb.execute('call ((void(*)(void*))&free)((void*)%d)'%buffer,to_string=True)
assert parts>0,(frame,cell,expected)
if frame==2:
    assert dynamic==1,dynamic
    gdb.set_convenience_variable('dynamic_handle',gdb.Value(handles[static]))
    assert value('((unsigned(*)(void*,unsigned long long,unsigned))$check_dynamic)((void*)%d,$dynamic_handle,1)'%scene)==1
elif frame==3:
    assert dynamic==0,dynamic
    assert value('((unsigned(*)(void*,unsigned long long,unsigned))$check_dynamic)((void*)%d,$dynamic_handle,0)'%scene)==1
print('Fixture native cell lights frame=%d cell=%d static=%d dynamic=%d parts=%d'%(frame,cell,count,dynamic,parts))
if frame==1:
    gdb.set_convenience_variable('original_cell',gdb.Value(cell))
    gdb.set_convenience_variable('original_count',gdb.Value(count))
if frame==3:
    target=value('((void*(*)(unsigned long))&malloc)(12)')
    find=value('((void*(*)(void*,const char*))&dlsym)($reader_module,"OtherCellCenter")')
    cells=value('*(int*)&wr_num_cells')
    destination=value('((int(*)(void*,int,int,void*,void*))%d)((void*)%d,%d,%d,(void*)&ComputeCellForLocation,(void*)%d)'%(find,base,cells,cell,target))
    assert destination>=0 and destination!=cell,destination
    gdb.execute('call ((void(*)(int,void*,void*))&ObjPosUpdate)(11,(void*)%d,(void*)%d)'%(target,position+16),to_string=True)
    gdb.execute('call ((void(*)(void*))&free)((void*)%d)'%target,to_string=True)
    print('Fixture moved receiver obj=11 destination_cell=%d'%destination)
if frame==4:
    assert cell!=value('$original_cell') and count!=value('$original_count'),(cell,count)

end
continue
end
run
