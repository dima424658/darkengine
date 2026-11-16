// ShadowView layout matches world_shadows.hpp. Include after defining shadows,
// localShadowMaps, sunShadowMaps and the geometric receiverPlaneNormal. Stored depth
// is linear world distance. Sun taps compensate the planar receiver slope.
// Golden-angle disk positions at the maximum sample count. Scaling by the
// requested count preserves the original distribution without per-tap trig.
const vec2 shadowDisk64[64]=vec2[64](
    vec2(0.088388348, 0.000000000),
    vec2(-0.112886094, 0.103412909),
    vec2(0.017279026, -0.196885589),
    vec2(0.142285610, 0.185586382),
    vec2(-0.261111594, -0.046186965),
    vec2(0.247347696, -0.157342357),
    vec2(-0.082732964, 0.307762500),
    vec2(-0.157780733, -0.303797202),
    vec2(0.342321081, 0.125015110),
    vec2(-0.356128043, 0.147004478),
    vec2(0.171677250, -0.366864310),
    vec2(0.126865119, 0.404465996),
    vec2(-0.382372945, -0.221592939),
    vec2(0.448566992, -0.098616193),
    vec2(-0.273753454, 0.389386115),
    vec2(-0.063243384, -0.488044849),
    vec2(0.388252361, 0.327219504),
    vec2(-0.522465978, 0.021605606),
    vec2(0.381098826, -0.379244229),
    vec2(-0.025496991, 0.551395868),
    vec2(-0.362616752, -0.434536064),
    vec2(0.574424977, 0.077288072),
    vec2(-0.486709123, 0.338639527),
    vec2(0.132996930, -0.591184672),
    vec2(0.307614969, 0.536829145),
    vec2(-0.601357837, -0.191849556),
    vec2(0.584142612, -0.269888698),
    vec2(-0.253065179, 0.604686295),
    vec2(-0.225855087, -0.627934694),
    vec2(0.600976157, 0.315856231),
    vec2(-0.667533301, 0.175959630),
    vec2(0.379430540, -0.590101657),
    vec2(0.120699087, 0.702313484),
    vec2(-0.572007804, -0.442995003),
    vec2(0.731701931, -0.060620002),
    vec2(-0.505760277, 0.546712029),
    vec2(0.003684035, -0.755181387),
    vec2(0.514304940, 0.566946143),
    vec2(-0.772294865, -0.071576123),
    vec2(0.625787313, -0.474950249),
    vec2(-0.142380577, 0.782649520),
    vec2(-0.428883933, -0.681539487),
    vec2(0.785920131, 0.215388134),
    vec2(-0.733485526, 0.376412651),
    vec2(0.289861685, -0.781852098),
    vec2(0.317911459, 0.780941614),
    vec2(-0.770263919, -0.365042457),
    vec2(0.823263305, -0.253820864),
    vec2(-0.440156540, 0.751049080),
    vec2(-0.184643229, -0.859851370),
    vec2(0.724177349, 0.514421682),
    vec2(-0.890157363, 0.110938585),
    vec2(0.587054272, -0.689695427),
    vec2(0.033319887, 0.913688834),
    vec2(-0.647726947, -0.657276427),
    vec2(0.930014130, 0.047552262),
    vec2(-0.724323123, 0.598471816),
    vec2(0.130975337, -0.938766723),
    vec2(0.542204878, 0.787449281),
    vec2(-0.939649018, -0.216211059),
    vec2(0.845936758, -0.479273931),
    vec2(-0.302491913, 0.932435597),
    vec2(-0.410097079, -0.899101154),
    vec2(0.916975861, 0.389027981));
vec2 shadowDisk(uint i,uint count) {
    return shadowDisk64[i]*sqrt(64.0/float(count));
}

uint shadowFace(vec3 direction) {
    vec3 a=abs(direction);
    if(a.x>=a.y && a.x>=a.z)return direction.x>=0?0u:1u;
    if(a.y>=a.z)return direction.y>=0?2u:3u;
    return direction.z>=0?4u:5u;
}
float alignLocalDepth(float depth,float farDepth,vec2 offset,vec3 plane,float receiver,bool radial) {
    if(abs(plane.z)<1e-5)return depth;
    float denominator=plane.z+dot(plane.xy,offset);
    if(plane.z*denominator<=0)return farDepth;
    float expected=receiver*plane.z/denominator;
    if(radial)expected*=sqrt(1+dot(offset,offset));
    if(expected>=farDepth)return farDepth;
    return depth+receiver-expected;
}
// map contains the type, layer and first point-light layer; projection is tap-invariant.
float localShadowDepth(ivec2 texel,uint layer) {
    if((layer&0x80000000u)!=0u)return texelFetch(cachedShadowMaps,ivec3(texel,layer&0x7fffffffu),0).r;
    return texelFetch(localShadowMaps,ivec3(texel,layer),0).r;
}
float shadowDepth(uvec3 map,float farDepth,vec2 projectedUV,vec2 offset,mat3 sampleBasis,vec3 receiverPlane,float receiverDepth,float resolution) {
    if(map.x==0u) {
        vec3 direction=sampleBasis[2]+sampleBasis[0]*offset.x+sampleBasis[1]*offset.y;
        uint face=shadowFace(direction);
        vec2 uv;
        // Same forward/right/down bases as BuildShadowPlan. Their 90-degree
        // projection depends only on direction component ratios.
        float faceSign=(face&1u)==0u?1:-1;
        if(face<2u)uv=vec2(-direction.y*faceSign,-direction.z)/abs(direction.x);
        else if(face<4u)uv=vec2(direction.x*faceSign,-direction.z)/abs(direction.y);
        else uv=vec2(-direction.x*faceSign,-direction.y)/abs(direction.z);
        uv=uv*.5+.5;
        uint layer=map.z+face;
        ivec2 texel=ivec2(clamp(floor(uv*resolution),vec2(0),vec2(resolution-1)));
        vec2 center=(vec2(texel)+.5)/resolution*2-1;
        vec3 sampledDirection;
        if(face<2u)sampledDirection=vec3(faceSign,-center.x*faceSign,-center.y);
        else if(face<4u)sampledDirection=vec3(center.x*faceSign,faceSign,-center.y);
        else sampledDirection=vec3(-center.x*faceSign,-center.y,faceSign);
        float depth=localShadowDepth(texel,layer)*farDepth;
        if(abs(receiverPlane.z)<1e-5)return depth;
        // Intersect the sampled cube ray directly with the receiver plane.
        // This avoids converting it back to a tangent offset and normalizing
        // that offset a second time inside alignLocalDepth.
        // The geometric normal is already in world space. Avoid a matrix
        // transform per tap merely to take the same invariant dot product.
        float denominator=dot(receiverPlaneNormal,sampledDirection);
        if(receiverPlane.z*denominator<=0)return farDepth;
        float expected=receiverDepth*receiverPlane.z*length(sampledDirection)/denominator;
        return expected>=farDepth?farDepth:depth+receiverDepth-expected;
    }
    vec2 uv=projectedUV+offset;
    if(any(lessThan(uv,vec2(0))) || any(greaterThan(uv,vec2(1))))return farDepth;
    if(map.x==2u)return textureLod(sunShadowMaps,vec3(uv,map.y),0).r*farDepth;
    // Fetch the exact texel whose center is used for plane compensation.
    ivec2 texel=ivec2(clamp(floor(uv*resolution),vec2(0),vec2(resolution-1)));
    float depth=localShadowDepth(texel,map.y)*farDepth;
    vec2 sampledOffset=(vec2(texel)+.5)/resolution-projectedUV;
    return alignLocalDepth(depth,farDepth,sampledOffset,receiverPlane,receiverDepth,false);
}
float filteredShadow(uint viewIndex,vec3 point,uint quality,uint blockerCount,uint filterCount) {
    blockerCount=clamp(blockerCount,1u,64u);
    filterCount=clamp(filterCount,1u,64u);
    ShadowView view=shadows.data[viewIndex];
    float receiver=view.metadata.x==0u?length(point-view.originNear.xyz):dot(view.depthPlane,vec4(point,1));
    if(receiver<=view.originNear.w || receiver>=view.parameters.x)return 1;
    uvec3 map=uvec3(view.metadata.xy,view.metadata.y-(viewIndex-view.metadata.z));
    vec2 projectedUV=vec2(0);
    if(view.metadata.x!=0u) {
        vec4 clip=view.viewProjection*vec4(point,1);
        if(clip.w<=0)return 1;
        projectedUV=clip.xy/clip.w*.5+.5;
    }
    // The receiver direction and tangent frame are shared by every blocker
    // search and filtering tap, including taps crossing cubemap face edges.
    mat3 sampleBasis=mat3(1);
    if(view.metadata.x==0u) {
        vec3 direction=(point-view.originNear.xyz)/receiver;
        vec3 axis=abs(direction.z)<.99?vec3(0,0,1):vec3(0,1,0);
        vec3 right=normalize(cross(direction,axis));
        sampleBasis=mat3(right,cross(direction,right),direction);
    }
    vec3 receiverPlane=vec3(0);
    if(view.metadata.x==0u)receiverPlane=vec3(dot(receiverPlaneNormal,sampleBasis[0]),dot(receiverPlaneNormal,sampleBasis[1]),dot(receiverPlaneNormal,sampleBasis[2]));
    else if(view.metadata.x==1u) {
        vec3 rowX=vec3(view.viewProjection[0][0],view.viewProjection[1][0],view.viewProjection[2][0]);
        vec3 rowY=vec3(view.viewProjection[0][1],view.viewProjection[1][1],view.viewProjection[2][1]);
        receiverPlane=vec3(2*dot(receiverPlaneNormal,rowX)/dot(rowX,rowX),2*dot(receiverPlaneNormal,rowY)/dot(rowY,rowY),dot(receiverPlaneNormal,point-view.originNear.xyz)/receiver);
    }
    vec2 receiverSlope=vec2(0);
    if(view.metadata.x==2u) {
        vec3 rowX=vec3(view.viewProjection[0][0],view.viewProjection[1][0],view.viewProjection[2][0]);
        vec3 rowY=vec3(view.viewProjection[0][1],view.viewProjection[1][1],view.viewProjection[2][1]);
        float facing=dot(receiverPlaneNormal,view.depthPlane.xyz);
        if(abs(facing)>1e-5)receiverSlope=-2*vec2(dot(receiverPlaneNormal,rowX)/dot(rowX,rowX),dot(receiverPlaneNormal,rowY)/dot(rowY,rowY))/facing;
    }
    // Local taps now account for quantization on the geometric receiver
    // plane, so their numerical bias need not change with map resolution.
    float bias=view.metadata.x!=2u?.002:max(.002,receiver*2/view.parameters.y);
    if(quality==0u || view.parameters.z<=0)return shadowDepth(map,view.parameters.x,projectedUV,vec2(0),sampleBasis,receiverPlane,receiver,view.parameters.y)+bias>=receiver?1:0;
    float texel=1/view.parameters.y;
    float radius=texel*1.5;
    if(quality==2u) {
        float search;
        if(view.metadata.x==2u)search=tan(view.parameters.z)*receiver;
        else search=view.parameters.z*(receiver-view.originNear.w)/max(view.originNear.w,.001);
        float scale=length(vec3(view.viewProjection[0][0],view.viewProjection[1][0],view.viewProjection[2][0]));
        if(view.metadata.x!=2u)scale/=max(receiver,.001);
        if(view.metadata.x!=0u)scale*=.5;
        search=clamp(search*scale,texel,.2);
        float sum=0;uint blockers=0u;
        for(uint i=0u;i<blockerCount;++i) {
            vec2 offset=shadowDisk(i,blockerCount)*search;
            float depth=shadowDepth(map,view.parameters.x,projectedUV,offset,sampleBasis,receiverPlane,receiver,view.parameters.y)-dot(receiverSlope,offset);
            if(depth+bias<receiver){sum+=depth;++blockers;}
        }
        if(blockers==0u)return 1;
        float blocker=sum/float(blockers);
        float width=view.metadata.x==2u?tan(view.parameters.z)*(receiver-blocker):view.parameters.z*(receiver-blocker)/max(blocker,.001);
        radius=clamp(width*scale,texel*.5,.2);
    }
    float visibility=0;
    for(uint i=0u;i<filterCount;++i) {
        vec2 offset=shadowDisk(i,filterCount)*radius;
        visibility+=shadowDepth(map,view.parameters.x,projectedUV,offset,sampleBasis,receiverPlane,receiver,view.parameters.y)+bias>=receiver+dot(receiverSlope,offset)?1:0;
    }
    return visibility/float(filterCount);
}
