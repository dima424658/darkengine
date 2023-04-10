/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/dyntex.cpp,v 1.3 1999/04/27 01:01:34 XEMU Exp $

#include <appagg.h>
#include <aggmemb.h>

#include <string.h>
#include <stdlib.h>

#include <lg.h>

#include <antxtype.h>
#include <animtext.h>

#include <port.h>
#include <wrdbrend.h>
#include <wrfunc.h>
#include <objpos.h>
#include <texmem.h>

#include <propface.h>
#include <propguid.h>
#include <sdesc.h>
#include <sdesbase.h>
#include <propbase.h>
#include <matrix.h>
#include <appagg.h>

#include <dyntex.h>

// Must be last header
#include <dbmem.h>
#include <initguid.h>
#include <dyniid.h>

#define LIST_MAX  1024 // pulled straight from my butt

EXTERN void uncache_surface(PortalPolygonRenderInfo *);

/* ------------------------------------------------------------ */
//////////////////////////////
// Our actual COM implementation!

IFloatProperty *gTextureRadiusProp;

static sRelativeConstraint DynTextureConstraints[] =
{
   { kConstrainAfter, &IID_IPropertyManager},
   //{ kConstrainAfter, &IID_IPortal},
   { kNullConstraint, }
};

class cDynTexture : public cCTDelegating<IDynTexture>,
                   public cCTAggregateMemberControl<kCTU_Default>
{

public:

   cDynTexture(IUnknown* pOuter)
   {
      MI_INIT_AGGREGATION_1(pOuter, IDynTexture, kPriorityNormal, DynTextureConstraints);
   }

   STDMETHOD(ChangeTexture)(ObjID objid, int id1, int id2)
   {
      ObjPos* pos = ObjPosGet(objid); 
      float rad;

      if (!gTextureRadiusProp->Get(objid,&rad))
         rad = 0.1; // ?? 
      return(ChangeTexture(&pos->loc, rad, id1, id2));
   }

   STDMETHOD(ChangeTexture)(Location *loc, float rad, int id1, int id2)
   {
      mxs_vector minvec,maxvec,radvec;
      int celllist[LIST_MAX];
      int numcells;
      int i,j;

      // if either texture was bogus, punt out
      if ((id1 == 0) || (id2 == 0))
         return E_INVALIDARG;

      radvec.x = radvec.y = radvec.z = 1.0F;
      mx_scaleeq_vec(&radvec, (float)rad);

      mx_copy_vec(&minvec,&loc->vec);
      mx_subeq_vec(&minvec,&radvec);
      mx_copy_vec(&maxvec,&loc->vec);
      mx_addeq_vec(&maxvec,&radvec);

      // go through the area and find all the cells
      numcells = PortalCellsInArea(celllist, LIST_MAX, &minvec, &maxvec, loc);

      // for each cell look through all the polys
      for (i=0; i < numcells; i++)
      {
         PortalCell *pcell;
         pcell = WR_CELL(celllist[i]);
         // for each poly check the texture id to our targets, and if so switch
         for (j=0; j < pcell->num_render_polys; j++)
         {
            PortalPolygonRenderInfo *ppri;
            ppri = &pcell->render_list[j];
            if (ppri->texture_id == id1)
            {
               ppri->texture_id = id2;
               uncache_surface(ppri);
               // add this change to our patch DB
               gTexturePatchTable.Set((celllist[i] << 8) + j, id2);
               gTextureInverseTable.Set((celllist[i] << 8) + j,id1);
            }
         }
      }
      return S_OK;
   }
};  // end of COM object
   
// creation func
void DynTextureCreate()
{
   AutoAppIPtr(Unknown); 
   new cDynTexture(pUnknown); 
}

sPropertyDesc TextureRadiusDesc =
{
   "TextureRadius", 0, 
   NULL, 1, 0, // constraints, versions
   { "Engine Features", "Retexture Radius" },
};

void AnimTextureInit(void)
{
   gTextureRadiusProp = CreateFloatProperty(&TextureRadiusDesc, kPropertyImplDense);
}

