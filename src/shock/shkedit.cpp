/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkedit.cpp,v 1.5 2000/02/19 13:20:16 toml Exp $

#include <matrix.h>

#include <editbr.h>
#include <editbr_.h>
#include <undoredo.h>
#include <brundo.h>
#include <gedit.h>
#include <vbrush.h>
#include <primal.h>
#include <brrend.h>
#include <media.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#define cur vBrush_editBrush_Get

#ifdef EDITOR

EXTERN short *_geditGetTxPtr(editBrush *br);

static void synch_tex(editBrush *modBrush)
{
   short *tx;
   int i;
   short *synchtex;
   int curface;

   synchtex = _geditGetTxPtr(modBrush);
   curface = modBrush->cur_face;
   for (i=0; i<modBrush->num_faces; i++)
   {
      modBrush->cur_face=i;
      tx=_geditGetTxPtr(modBrush);
      *tx=*synchtex;
      if (gedit_reassign_texture(modBrush,TRUE))
         gedit_redraw_3d();
   }
   modBrush->cur_face = curface;
}

void ShockEditSynchBrushTextures(void)
{
   editBrush *modBrush=cur();
   editUndoStoreStart(modBrush);
   synch_tex(modBrush);
   editUndoStoreBrush(modBrush);
}

static void synch_scale(editBrush *modBrush)
{
   short scale;
   int i;
   int curface;

   scale = modBrush->txs[modBrush->cur_face].tx_scale;
   curface = modBrush->cur_face;
   for (i=0; i < modBrush->num_faces; i++)
   {
      modBrush->cur_face = i;
      modBrush->txs[modBrush->cur_face].tx_scale = scale;
      // do we want this here?  -- X
      if (gedit_reassign_texture(modBrush,TRUE))
         gedit_redraw_3d();
   }
   modBrush->cur_face = curface;
}

void ShockEditSynchBrushScales(void)
{
   editBrush *modBrush=cur();
   editUndoStoreStart(modBrush);
   synch_scale(modBrush);
   editUndoStoreBrush(modBrush);
}

/*
static void synch_tex_and_scale(editBrush *modBrush)
{
   short scale;
   short *tx;
   int i;
   short *synchtex;

   scale = modBrush->txs[modBrush->cur_face].tx_scale;
   synchtex = _geditGetTxPtr(modBrush);

   for (i=0; i < modBrush->num_faces; i++)
   {
      modBrush->cur_face = i;
      modBrush->txs[modBrush->cur_face].tx_scale = scale;
      // do we want this here?  -- X
      if (gedit_reassign_texture(modBrush,TRUE))
         gedit_redraw_3d();
   }

   for (i=0; i<modBrush->num_faces; i++)
   {
      modBrush->cur_face=i;
      tx=_geditGetTxPtr(modBrush);
      *tx=*synchtex;
      if (gedit_reassign_texture(modBrush,TRUE))
         gedit_redraw_3d();
   }
}
*/

void ShockEditSynchBrushFull(void)
{
   editBrush *modBrush=cur();
   editUndoStoreStart(modBrush);
   synch_tex(modBrush);
   synch_scale(modBrush);
   editUndoStoreBrush(modBrush);
}

void ShockEditMakeDecalBrush(char *args)
{
   int w,h;
   editBrush *bp, *guybrush;
   int ptidx,numpts;
   int *facepts;
   mxs_vector *brushvecs;
   mxs_vector p1, p2;
   mxs_vector temp;
   mxs_vector normal, midpt;
   mxs_matrix rotmat;

   // get current brush
   bp = vBrush_GetSel();
   if (bp->cur_face == -1)
      return;

   facepts = primalBr_FacePts(bp->primal_id, bp->cur_face, &numpts);
   brushvecs = brushBuildSolo(bp);

   // compute midpoint & normal of selected face
   for (ptidx = 0; ptidx < numpts; ptidx++)
   {
      mx_addeq_vec(&midpt, &brushvecs[facepts[ptidx]]);
   }
   mx_diveq_vec(&midpt,numpts);

   // to compute normal, we find to coplanar vectors, and then take their cross product
   mx_sub_vec(&p1, &brushvecs[facepts[1]], &brushvecs[facepts[0]]);
   mx_sub_vec(&p2, &brushvecs[facepts[2]], &brushvecs[facepts[0]]);
   mx_cross_norm_vec(&normal, &p2, &p1);
   mx_scale_vec(&temp, &normal, 0.5F);
   // move midpoint along normal 
   mx_addeq_vec(&midpt, &temp);

   // create new brush
   sscanf(args,"%d,%d",&w,&h);
   editUndoStoreBlock(TRUE);
   guybrush = brushInstantiate(PRIMAL_CUBE_IDX);
   guybrush->media = MEDIA_SOLID;

   // position it
   mx_copy_vec(&(guybrush->pos), &midpt);
   // resize it
   guybrush->sz.el[0] = 0.5F;
   guybrush->sz.el[1] = w/2;
   guybrush->sz.el[2] = h/2;
   // rotate it
   mx_mk_move_x_mat(&rotmat, &normal);
   mx_mat2ang(&guybrush->ang, &rotmat);

   gedit_full_create_brush(guybrush,NULL,GEDIT_CREATE_AT_END,GEDIT_DEFAULT_BRTYPE);
   guybrush->tx_id = 0; // default texture
   vBrush_NewBrush(guybrush);  // build and notify about the brush
   editUndoStoreBlock(FALSE);
}
#endif