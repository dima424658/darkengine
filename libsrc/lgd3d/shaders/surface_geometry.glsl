#ifndef DARK_SURFACE_GEOMETRY
#define DARK_SURFACE_GEOMETRY
// Identity.w: bit 0 = motion-history validity, bits 1–30 = octahedral
// geometric normal (two 15-bit UNORM coordinates). No extra MRT is needed.
vec2 surfaceSign(vec2 v){return mix(vec2(-1),vec2(1),greaterThanEqual(v,vec2(0)));}
uint packSurfaceGeometry(vec3 n,bool historyValid) {
    n/=abs(n.x)+abs(n.y)+abs(n.z);
    vec2 p=n.z>=0?n.xy:(1-abs(n.yx))*surfaceSign(n.xy);
    uvec2 q=uvec2(round(clamp(p*.5+.5,0,1)*32767));
    return ((q.x|(q.y<<15))<<1)|(historyValid?1u:0u);
}
vec3 unpackSurfaceGeometry(uint metadata) {
    uint packed=metadata>>1;
    vec2 p=vec2(packed&32767u,(packed>>15)&32767u)/32767*2-1;
    vec3 n=vec3(p,1-abs(p.x)-abs(p.y));
    if(n.z<0)n.xy=(1-abs(n.yx))*surfaceSign(n.xy);
    return normalize(n);
}
#endif
