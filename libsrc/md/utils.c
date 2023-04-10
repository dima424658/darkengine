/*
 * $Source: x:/prj/tech/libsrc/md/RCS/utils.c $
 * $Revision: 1.6 $
 * $Author: KEVIN $
 * $Date: 1997/08/14 11:03:16 $
 *
 * Model Library rendering routines
 *
 */

#include <res.h>
#include <g2.h>
#include <mdutil.h>
#include <md.h>

// Ick. only for BMF_LOADED
#include <tmgr.h>

static ulong texture_lock_flags = 0;

void md_set_texture_lock_flags(ulong flags)
{
   texture_lock_flags = flags;
}

// Locks down textures using a resource offset
// This assumes that the texture handles are set
// to be added to a resource offset to get their
// ResID.  So if you do sneakiness, this won't work
// Call this just before rendering the model
// Assumes textures are stored as a .btm

void md_mat_textures_lock(mds_model *m,ID base)
{
   mds_mat *mats,*mat;
   grs_bitmap *bm;
   int i;

   // Lock down textures
   mats = md_mat_list(m);
   for (i=0;i<m->mats;++i) {
      mat = &mats[i];
      if (mat->type==MD_MAT_TMAP) {
         bm = (grs_bitmap *)ResLockSetFlags((Id) (mat->handle + base), texture_lock_flags);

         // set bits only if texture hasn't been loaded into hardware already
         // really we just want to do this the first time the bitmap's loaded
         // from disk, not every time we lock it; then we wouldn't need to check
         // BMF_LOADED.
         if (!(bm->flags&BMF_LOADED))
            bm->bits = ((uchar *)bm)+sizeof(grs_bitmap);
         mdd_vtext_tab[mat->num] = bm;
      }
   }
}


// Sets the colors of a model, should work in both
// 8 bit mode (with grd_ipal set) and in 16 bit or
// 24 bit mode.  Call this just before rendering the
// model

void md_mat_colors_set(mds_model *m)
{
   mds_mat *mats,*mat;
   int i;

   // Free textures
   mats = md_mat_list(m);
   for (i=0;i<m->mats;++i) {
      mat = &mats[i];
      if (mat->type==MD_MAT_COLOR) {
         mdd_vcolor_tab[mat->num] = grd_ipal[mat->ipal];
      }
   }
}


// Just like md_mat_textures_lock, only it unlocks
// the textures.

void md_mat_textures_unlock(mds_model *m,ID base)
{
   mds_mat *mats,*mat;
   int i;

   // Free textures
   mats = md_mat_list(m);
   for (i=0;i<m->mats;++i) {
      mat = &mats[i];
      if (mat->type==MD_MAT_TMAP) {
         ResUnlock((Id) (mat->handle + base));
      }
   }
}
