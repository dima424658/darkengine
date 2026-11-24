set pagination off
set confirm off
set debuginfod enabled off
set $frame=0
break WorldSceneCaptureModels
commands
silent
set $frame=$frame+1
if $frame==2
set $setfloat=(int(*)(void*,int,float)) &'cGenericProperty<IFloatProperty, &IID_IFloatProperty, float>::Set(int, float)'
set $deletefloat=(int(*)(void*,int)) &'cProperty<IFloatProperty, &IID_IFloatProperty>::Delete(int)'
set $setbool=(int(*)(void*,int,int)) &'cGenericProperty<IBoolProperty, &IID_IBoolProperty, int>::Set(int, int)'
set $deletebool=(int(*)(void*,int)) &'cProperty<IBoolProperty, &IID_IBoolProperty>::Delete(int)'
set $shadow=*(void**)&g_pObjShadowProp
set $alpha=*(void**)&gAlphaRenderProp
set $illum=*(void**)&g_pSelfIlluminationProp
set $tm=*(void**)&'cCachedTraitQuery::TraitMan'
set $revision=(unsigned long long(*)()) &RenderMaterialPropertyRevision
set $old=$revision()
set $r=$setfloat($alpha,11,0.25)
set $r=$setfloat($illum,11,0.5)
printf "Fixture concrete edit revision %llu -> %llu\n",$old,$revision()
end
if $frame==4
set $r=$setfloat($alpha,11,0.75)
set $r=$setfloat($illum,11,0.125)
end
if $frame==6
set $r=$deletefloat($alpha,11)
set $r=$deletefloat($illum,11)
end
if $frame==8
set $getarch=(int(*)(void*,int)) &'cTraitManager::GetArchetype(int)'
set $arch=$getarch($tm,11)
set $r=$setfloat($alpha,$arch,0.6)
set $r=$setfloat($illum,$arch,0.375)
printf "Fixture archetype edit obj=11 archetype=%d\n",$arch
end
if $frame==10
set $r=$deletefloat($alpha,$arch)
set $r=$deletefloat($illum,$arch)
end
if $frame==12
set $root=((int(*)(void*)) &'cTraitManager::RootMetaProperty()')($tm)
set $meta=((int(*)(void*,const char*,int)) &'cTraitManager::CreateMetaProperty(char const*, int)')($tm,"M-RendererCacheFixture",$root)
set $r=$setfloat($alpha,$meta,0.45)
set $r=$setbool($shadow,$meta,0)
set $old=$revision()
set $r=((int(*)(void*,int,int)) &'cTraitManager::AddObjMetaProperty(int, int)')($tm,11,$meta)
printf "Fixture hierarchy add meta=%d revision %llu -> %llu\n",$meta,$old,$revision()
end
if $frame==14
set $old=$revision()
set $r=((int(*)(void*,int,int)) &'cTraitManager::RemoveObjMetaProperty(int, int)')($tm,11,$meta)
printf "Fixture hierarchy remove revision %llu -> %llu\n",$old,$revision()
end
if $frame==16
set $r=$setbool($shadow,11,0)
end
if $frame==18
set $r=$deletebool($shadow,11)
end
if $frame==20
set $r=$setbool($shadow,$arch,0)
end
if $frame==22
set $r=$deletebool($shadow,$arch)
end
continue
end
run
