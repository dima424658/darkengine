#ifndef __D3D11SCENE_H
#define __D3D11SCENE_H

#pragma pack(push, 8)
#include <d3d11.h>
#pragma pack(pop)
#include <string.h>

#include "render_backend.h"

// Kept in one place so the production presenter and the offscreen regression
// rig compile and exercise exactly the same scene shaders and blend equations.
static const char kD3D11SceneVS[] =
    "cbuffer C:register(b0){float w;float h;float fe;float ut;float4 "
    "fc;float at;float3 pad;}struct I{float4 p:POSITION;float4 "
    "c:COLOR0;float f:FOG;float2 "
    "u:TEXCOORD0;float2 v:TEXCOORD1;};struct O{float4 "
    "p:SV_Position;noperspective float4 "
    "c:COLOR0;noperspective float f:FOG;float2 u:TEXCOORD0;float2 "
    "v:TEXCOORD1;};O main(I i){O o;float "
    "q=1/max(i.p.w,.000001);o.p=float4((i.p.x/w*2-1)*q,(1-i.p.y/"
    "h*2)*q,i.p.z*q,q);o.c=i.c;o.f=i.f;o.u=i.u;o.v=i.v;return o;}";

static const char kD3D11ScenePS[] =
    "cbuffer C:register(b0){float w;float h;float fe;float ut;float4 "
    "fc;float at;float3 pad;}Texture2D a:register(t0);Texture2D "
    "b:register(t1);SamplerState "
    "x:register(s0);SamplerState y:register(s1);struct I{float4 "
    "p:SV_Position;noperspective float4 c:COLOR0;noperspective float "
    "f:FOG;float2 u:TEXCOORD0;float2 "
    "v:TEXCOORD1;};float4 main(I i):SV_Target{float4 "
    "c=i.c*a.Sample(x,i.u);if(ut>.5)c*=b.Sample(y,i.v);if(at>.5)clip(c.a-.5);"
    "if(fe>.5)c.rgb=lerp(fc.rgb,c.rgb,saturate(i.f));return c;}";

static inline void D3D11DescribeBlend(int mode, D3D11_BLEND_DESC *desc) {
  D3D11_RENDER_TARGET_BLEND_DESC *target;
  memset(desc, 0, sizeof(*desc));
  target = &desc->RenderTarget[0];
  target->RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
  if (mode == kRenderBackendBlendOpaque)
    return;

  target->BlendEnable = TRUE;
  target->BlendOp = D3D11_BLEND_OP_ADD;
  target->BlendOpAlpha = D3D11_BLEND_OP_ADD;
  target->SrcBlendAlpha = D3D11_BLEND_ONE;

  if (mode == kRenderBackendBlendMultiply) {
    target->SrcBlend = D3D11_BLEND_DEST_COLOR;
    target->DestBlend = D3D11_BLEND_ZERO;
    target->DestBlendAlpha = D3D11_BLEND_ZERO;
  } else if (mode == kRenderBackendBlendAdd) {
    target->SrcBlend = D3D11_BLEND_SRC_ALPHA;
    target->DestBlend = D3D11_BLEND_ONE;
    target->DestBlendAlpha = D3D11_BLEND_ONE;
  } else {
    target->SrcBlend = D3D11_BLEND_SRC_ALPHA;
    target->DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    target->DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
  }
}

#endif
