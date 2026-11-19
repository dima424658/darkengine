// Shared by direct visibility and indirect continuation rays. BLAS triangles
// remain non-opaque; safe opaque instances override this in the TLAS. Only
// cutouts and emitter glass need shader coverage/per-emitter candidate tests.
struct RayTriangle {vec4 positionU[3];vec4 normalV[3];};
layout(set=0,binding=11) uniform accelerationStructureEXT rayScene;
layout(set=0,binding=12,std430) readonly buffer RayTriangles {RayTriangle data[];} rayTriangles;
layout(set=0,binding=13,std430) readonly buffer RayInstances {uvec4 data[];} rayInstances;
#ifdef RAY_SCENE_HISTORY
layout(set=2,binding=2,std430) readonly buffer PreviousRayMaterials {Material data[];} previousRayMaterials;
layout(set=2,binding=7) uniform sampler2D previousRayTextures[];
layout(set=2,binding=11) uniform accelerationStructureEXT previousRayScene;
layout(set=2,binding=12,std430) readonly buffer PreviousRayTriangles {RayTriangle data[];} previousRayTriangles;
layout(set=2,binding=13,std430) readonly buffer PreviousRayInstances {uvec4 data[];} previousRayInstances;
#endif
vec2 rayUV(RayTriangle triangle,vec3 bary) {
    return vec2(dot(vec3(triangle.positionU[0].w,triangle.positionU[1].w,triangle.positionU[2].w),bary),
        dot(vec3(triangle.normalV[0].w,triangle.normalV[1].w,triangle.normalV[2].w),bary));
}
bool rayAccept(in rayQueryEXT query,int emitter,bool cullBackfaces,bool previous) {
    uint instanceIndex=rayQueryGetIntersectionInstanceCustomIndexEXT(query,false);
#ifdef RAY_SCENE_HISTORY
    uvec4 instance=previous?previousRayInstances.data[instanceIndex]:rayInstances.data[instanceIndex];
#else
    uvec4 instance=rayInstances.data[instanceIndex];
#endif
    if(emitter!=0 && int(instance.z)==emitter && instance.w!=0u)return false;
#ifdef RAY_SCENE_HISTORY
    Material material=previous?previousRayMaterials.data[instance.y]:materials.data[instance.y];
#else
    Material material=materials.data[instance.y];
#endif
    if(cullBackfaces && material.doubleSided==0u && !rayQueryGetIntersectionFrontFaceEXT(query,false))return false;
    if(material.opacity!=0u) {
        uint triangleIndex=instance.x+rayQueryGetIntersectionPrimitiveIndexEXT(query,false);
#ifdef RAY_SCENE_HISTORY
        RayTriangle triangle=previous?previousRayTriangles.data[triangleIndex]:rayTriangles.data[triangleIndex];
#else
        RayTriangle triangle=rayTriangles.data[triangleIndex];
#endif
        vec2 b=rayQueryGetIntersectionBarycentricsEXT(query,false);vec3 bary=vec3(1-b.x-b.y,b);
#ifdef RAY_SCENE_HISTORY
        float alpha=previous?textureLod(previousRayTextures[nonuniformEXT(material.textureIndex)],rayUV(triangle,bary),0).a:
            textureLod(textures[nonuniformEXT(material.textureIndex)],rayUV(triangle,bary),0).a;
#else
        float alpha=textureLod(textures[nonuniformEXT(material.textureIndex)],rayUV(triangle,bary),0).a;
#endif
        alpha*=material.baseColor.a;
        // Blended surfaces use their coverage as an alpha cutout for visibility.
        if(alpha<(material.opacity==1u?material.cutoff:0.5))return false;
    }
    return true;
}
bool rayAccept(in rayQueryEXT query,int emitter,bool cullBackfaces) {return rayAccept(query,emitter,cullBackfaces,false);}
bool rayOccluded(vec3 origin,vec3 direction,float maximum,int emitter,bool previous) {
    if(maximum<=0.002)return false;
    rayQueryEXT query;
#ifdef RAY_SCENE_HISTORY
    if(previous)rayQueryInitializeEXT(query,previousRayScene,gl_RayFlagsTerminateOnFirstHitEXT|gl_RayFlagsCullBackFacingTrianglesEXT,1,origin,0.001,direction,maximum);
    else
#endif
    rayQueryInitializeEXT(query,rayScene,gl_RayFlagsTerminateOnFirstHitEXT|gl_RayFlagsCullBackFacingTrianglesEXT,1,origin,0.001,direction,maximum);
    while(rayQueryProceedEXT(query)) {
        if(rayQueryGetIntersectionTypeEXT(query,false)==gl_RayQueryCandidateIntersectionTriangleEXT && rayAccept(query,emitter,true,previous))
            rayQueryConfirmIntersectionEXT(query);
    }
    return rayQueryGetIntersectionTypeEXT(query,true)!=gl_RayQueryCommittedIntersectionNoneEXT;
}
bool rayOccluded(vec3 origin,vec3 direction,float maximum,int emitter) {return rayOccluded(origin,direction,maximum,emitter,false);}
