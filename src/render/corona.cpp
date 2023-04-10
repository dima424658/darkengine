/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/corona.cpp,v 1.3 2000/01/20 15:47:31 MAT Exp $

/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\
   corona.cpp


\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */


#include <math.h>

#include <dynarray.h>
#include <lgd3d.h>
#include <r3d.h>

#include <prophash.h>
#include <dataops.h>
#include <obprpimp.h>
#include <sdesbase.h>
#include <sdesc.h>

#include <vhot.h>
#include <objmodel.h>
#include <port.h>
#include <simtime.h>
#include <objpos.h>
#include <camera.h>
#include <playrobj.h>
#include <animlgts.h>
#include <litprops.h>
#include <litprop.h>

#include <corona.h>
#include <dbmem.h>
#include <initguid.h>
#include <coronag.h>


extern "C" void FindVhotLocation(mxs_vector *offset, ObjID obj, int vhot);
extern "C" void (*portal_post_render_cback)(void);
#ifdef EDITOR
extern "C" g_lgd3d;
#endif // EDITOR


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   property

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

// lime?
struct sCoronaLight
{
   // things set by code
   int m_iObjModelIdx;          // figgered out on obj set
   mxs_vector m_WorldPos;       // figgered out on set or change on obj pos

   // things set in the SDESC
   float m_fNearRadius;
   float m_fFarRadius;
   float m_fMaxDist;
   float m_fAlpha;
   char m_aszTexture[16];

   // things set by code per-frame
   float m_fStrength;           // 1.0 unless it's an animated light
};


#undef  INTERFACE
#define INTERFACE ICoronaProperty

DECLARE_PROPERTY_INTERFACE(ICoronaProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sCoronaLight *);
};

static sPropertyDesc g_CoronaDesc
= {
   "Corona",
   kPropertyNoInherit | kPropertyInstantiate, // flags
   0,           // constraints
   1, 0,        // version
   { "Renderer", "Corona", "glow rendered in front of a light" },
   kPropertyChangeLocally,
};

class cCoronaOps : public cClassDataOps<sCoronaLight>
{
};

class cCoronaStore : public cHashPropertyStore<cCoronaOps>
{
};

typedef cSpecificProperty<ICoronaProperty, &IID_ICoronaProperty, sCoronaLight *,
   cCoronaStore> cBaseCoronaProperty;

class cCoronaProperty : public cBaseCoronaProperty
{
public:
   cCoronaProperty(const sPropertyDesc *desc)
      : cBaseCoronaProperty(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(cCoronaProperty);

   void OnListenMsg(ePropertyListenMsg type, ObjID obj, uPropListenerValue val)
   {
      cBaseCoronaProperty::OnListenMsg(type, obj, val);

      if (OBJ_IS_ABSTRACT(obj))
         return;

      if (type & (kListenPropSet | kListenPropModify)) {
         sCoronaLight *pCorona = (sCoronaLight *) val.ptrval;
         if (!pCorona)
            return;

         FindVhotLocation(&pCorona->m_WorldPos, obj, kVHotLightSource);

         if (!pCorona->m_aszTexture[0])
            return;
         pCorona->m_iObjModelIdx = objmodelLoadModel(pCorona->m_aszTexture);

#ifndef SHIP
         if (pCorona->m_iObjModelIdx == MIDX_INVALID)
            Warning(("corona: can't find texture %s on object %d.\n",
                     pCorona->m_aszTexture, obj));
#endif // ~SHIP
      }
   }

protected:
   void CreateEditor();
};


static sFieldDesc g_aCoronaFieldDesc[]
= {
   { "radius up close", kFieldTypeFloat,
     FieldLocation(sCoronaLight, m_fNearRadius) },
   { "radius at max dist", kFieldTypeFloat,
     FieldLocation(sCoronaLight, m_fFarRadius) },
   { "max. dist. visible", kFieldTypeFloat,
     FieldLocation(sCoronaLight, m_fMaxDist) },
   { "alpha", kFieldTypeFloat,
     FieldLocation(sCoronaLight, m_fAlpha) },
   { "texture", kFieldTypeString,
     FieldLocation(sCoronaLight, m_aszTexture) },
};

static sStructDesc g_CoronaStructDesc
= StructDescBuild(cCoronaProperty, kStructFlagNone,
                  g_aCoronaFieldDesc);

void cCoronaProperty::CreateEditor()
{
   AutoAppIPtr(StructDescTools);
   pStructDescTools->Register(&g_CoronaStructDesc);
   cPropertyBase::CreateEditor(this);
}

ICoronaProperty *g_pCoronaProperty;


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   editor-only machinery

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

// When a designer moves an object the designer's computer moves its corona.
// If it has one.
#ifdef EDITOR
static void ObjPosCallback(ObjID Obj, const ObjPos* pos, void* data)
{
   sCoronaLight *pCorona;
   if (g_pCoronaProperty->Get(Obj, &pCorona))
      FindVhotLocation(&pCorona->m_WorldPos, Obj, kVHotLightSource);
}
#endif // EDITOR


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   actual callback/rendering stuff

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

// next in post_rendering callback chain
static void (*g_pfnNextPostRenderCallback)(void) = 0;

static const int kMaxLightsInFrame = 128;
static sCoronaLight *g_apLightsInFrame[kMaxLightsInFrame];
static int g_iNumLightsInFrame;
static const Location *g_pCamLoc;
static mxs_matrix g_CamOrientation;

struct sR3SPlus : public r3s_point
{
   float m_afPlus[4];
};

static sR3SPlus g_aPt[4];
static sR3SPlus *g_apPt[4] = {g_aPt, g_aPt + 1, g_aPt + 2, g_aPt + 3};
static mxs_vector g_aV[4];


// post-portal render callback--renders our coronas & resets counter
// to 0 for next frame
void CoronaRender()
{
   if (g_iNumLightsInFrame) {
      int iOldStride = r3d_glob.cur_stride;
      r3d_glob.cur_stride = sizeof(sR3SPlus);

      r3_start_block();
      r3_set_clip_flags(R3_CLIP_UV | R3_CLIP_RGBA);
      r3_set_polygon_context(R3_PL_POLYGON | R3_PL_TEXTURE | R3_PL_GOURAUD);
      lgd3d_set_blend(TRUE);

      int iZWrite = lgd3d_is_zwrite_on();
      lgd3d_set_zwrite(FALSE);

      int iZCompare = lgd3d_is_zcompare_on();
      lgd3d_set_zcompare(FALSE);

      for (int i = 0; i < g_iNumLightsInFrame; ++i) {
         register sCoronaLight *pCorona = g_apLightsInFrame[i];

         float fDist = mx_dist_vec(&pCorona->m_WorldPos, &g_pCamLoc->vec);
         if (fDist > pCorona->m_fMaxDist)
            continue;

         // change size w/distance
         float fDistFrac = fDist / pCorona->m_fMaxDist;
         float fRadius = pCorona->m_fNearRadius
            + (pCorona->m_fFarRadius - pCorona->m_fNearRadius)
            * fDistFrac * fDistFrac * pCorona->m_fStrength;

         // fade w/distance
         lgd3d_set_alpha(pCorona->m_fAlpha * (1.0 - sqrt(fDistFrac))
                       * pCorona->m_fStrength);

         mxs_vector u, v;  // worldspace
         mx_scale_vec(&u, &g_CamOrientation.vec[1], fRadius);
         mx_scale_vec(&v, &g_CamOrientation.vec[2], fRadius);

         // set up our four corners in worldspace
         mx_add_vec(&g_aV[0], &pCorona->m_WorldPos, &u);
         mx_addeq_vec(&g_aV[0], &v);

         mx_sub_vec(&g_aV[1], &pCorona->m_WorldPos, &u);
         mx_addeq_vec(&g_aV[1], &v);

         mx_sub_vec(&g_aV[2], &pCorona->m_WorldPos, &u);
         mx_subeq_vec(&g_aV[2], &v);

         mx_add_vec(&g_aV[3], &pCorona->m_WorldPos, &u);
         mx_subeq_vec(&g_aV[3], &v);

         r3_transform_block(4, g_aPt, g_aV);

         r3_set_texture((r3s_texture)
                        objmodelGetModel(pCorona->m_iObjModelIdx));
         r3_draw_poly(4, (r3s_point **) g_apPt);
      }

      g_iNumLightsInFrame = 0;

      lgd3d_set_alpha(1.0);
      lgd3d_set_zcompare(iZCompare);
      lgd3d_set_zwrite(iZWrite);
      lgd3d_set_blend(FALSE);
      r3_end_block();
      r3d_glob.cur_stride = iOldStride;
   }

   if (g_pfnNextPostRenderCallback)
      g_pfnNextPostRenderCallback();
}


void CoronaInit(void)
{
   g_aPt[0].grp.u = g_aPt[3].grp.u = 0.0;
   g_aPt[1].grp.u = g_aPt[2].grp.u = 1.0;
   g_aPt[0].grp.v = g_aPt[1].grp.v = 0.0;
   g_aPt[2].grp.v = g_aPt[3].grp.v = 1.0;

   for (int i = 0; i < 4; ++i) {
      g2s_point *pGP = (g2s_point *) &g_aPt[i].grp;

      pGP->a = 1.0;
      pGP->i = 1.0;
      pGP->r = 1.0;
      pGP->g = 1.0;
      pGP->b = 1.0;
   }

   g_pfnNextPostRenderCallback = portal_post_render_cback;
   portal_post_render_cback = CoronaRender;

   g_pCoronaProperty = new cCoronaProperty(&g_CoronaDesc);

#ifdef EDITOR
   ObjPosListen(ObjPosCallback, 0);
#endif // EDITOR
}


void CoronaTerm(void)
{
}


// called from rendloop so we don't have to find camera ourselves
void CoronaFrame(const Position *pPos)
{
   g_pCamLoc = &pPos->loc;
   mx_ang2mat(&g_CamOrientation, &pPos->fac);
}


void CoronaCheckObj(ObjID Obj)
{
#ifdef EDITOR
   if (!g_lgd3d)
      return;
#endif // EDITOR

   // Does this object have a corona?
   sCoronaLight *pCorona;
   if (!g_pCoronaProperty->Get(Obj, &pCorona))
      return;

#ifndef SHIP
   if (pCorona->m_iObjModelIdx == MIDX_INVALID)
      return;
#endif // ~SHIP

   // Is the corona visible?  We only raycast against terrain, for
   // speed.
   Position *pPos = ObjPosGetUnsafe(Obj);
   Location DummyDest;
   if (!PortalRaycast((Location *) g_pCamLoc, &pPos->loc, &DummyDest, 0))
      return;

   if (g_iNumLightsInFrame == kMaxLightsInFrame) {
      Warning(("corona: More than %d lights in frame.\n", kMaxLightsInFrame));
      return;
   }

   // If this is an animated light we need to do, um, something.
   pCorona->m_fStrength = 1.0;
   sAnimLightProp *pAnimLight;
   if (ObjAnimLightGet(Obj, &pAnimLight)) {
      if (!pCorona->m_fStrength)
         return;
      pCorona->m_fStrength = pAnimLight->animation.brightness
                           / pAnimLight->animation.max_brightness;
   }

   g_apLightsInFrame[g_iNumLightsInFrame++] = pCorona;
}
