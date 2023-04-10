/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/texmem.h,v 1.14 2000/01/31 09:53:34 adurant Exp $
// this system monitors texture usage and setup and such
#pragma once

#ifndef __TEXMEM_H
#define __TEXMEM_H

#include <g2.h>
#include <resapi.h>

#define TEXMEM_MAX           256    // actual maximum number of entries
#define TEXMEM_NO_HND         -1    // no texture handle available
#define TEXMEM_NO_ALIGN       -1    // align field not set...

#define TEXMEM_LOADID_NULL     0    // nothing loaded here
#define TEXMEM_LOADID_ANON     1    // something here, not part of a loadid group
#define TEXMEM_LOADID_BASE     2    // base "real" loadid value
#define TEXMEM_LOADID_LOCK    -1    // means this should be considered "locked"

// valid loadids start at 2 (base) and go up, therefore

EXTERN grs_bitmap *texmem_tm[TEXMEM_MAX];
EXTERN IRes       *texmem_hnd[TEXMEM_MAX];
EXTERN int         texmem_loadid[TEXMEM_MAX];
EXTERN int         texmem_cur_max;
EXTERN BOOL        texmem_out_of_space;
EXTERN BOOL        texmem_find_align;

#define texmemGetTexture(idx) (texmem_tm[idx])
#define texmemGetHandle(idx)  (texmem_hnd[idx])
#define texmemGetMax()        (texmem_cur_max)
#define texmemValidIdx(idx)   ((idx>=0)&&(idx<texmemGetMax()))
#define texmemSpaceOverrun()  (texmem_out_of_space)
#define texmemFindAlign()     (texmem_find_align)

//////////////////////////
// special texture IDX's
#define WATERIN_IDX  247
#define WATEROUT_IDX 248
#define BACKHACK_IDX 249

// and callback info
// set these "global" callback idxs when you need to get data in to RN callbacks 'n such
// NOTE: all internal users of these set them back to these values
//   so default behavior of no idx setting or anything
// callback_idx of 0 means find a free one
EXTERN int texmem_callback_idx;      // defaults to TEXMEM_NO_HND
EXTERN int texmem_callback_loadid;   // defaults to TEXMEM_LOADID_ANON

//////////////////////////
// misc utility function
EXTERN grs_bitmap *texmemBuildCheckerboardTmap(int shift, int seed, BOOL trans);
EXTERN uchar texmemUGradient(int x, int y, int size);
EXTERN uchar texmemVGradient(int x, int y, int size);
EXTERN uchar texmemUVGradient(int x, int y, int size);
EXTERN grs_bitmap *texmemBuildGradientTmap(int shift,uchar (*pixfunc)(int x, int y, int size));

// clears all textures, frees all portal texture memory, resets all internal state
// leaves the database in a minimal but valid state (sort of the null texture state)
EXTERN void texmemClearAll(void);

// shutdown completely the texture memory, totally, dealth, so on
EXTERN void texmemShutdown(void);

/////////////////////////
// conversion functions

// wrapper on the portal create mipmapped texture stuff
// in dbg, keeps a count to try and stay on top of usage
EXTERN grs_bitmap *texmem_convert_tmap(grs_bitmap *bm);

// actually frees the texture mem from the pointer
EXTERN void texmem_free_tmap(grs_bitmap *bm);

// Deals with setting up a texture resource after it's been loaded in.
// Signature is dictated by the resource system. We never actually use
// pStore, so it can be left NULL. If pClientData is non-NULL, then it
// should be set to a string giving the directory that this resource was
// actually found in; this can generally be ignored, since it is mostly
// needed for PrepAllResources.

// NOTE: this still depends as it always has on the texmem_callback variables current states
EXTERN BOOL texmem_interpret_new_res(IRes *pRes, 
                                     IStore *pStore, 
                                     void *pClientData);

//////////////////////
// api to load/save

// when you know what idx you want to use for texture, returns -1 if it cant do so
// this will load over the slot if it is currently in use, so be warned
EXTERN int texmemLoadSingleAtIdx(char *dirname, char *fname, int idx);

// loads fname as a new textures, returns the texmem handle for it, or -1 if none
EXTERN int texmemLoadSingle(char *dirname, char *fname);

// Load the named texture. If doMipmap is set, then mipmap the texture once
// it's loaded.
EXTERN IRes *texmemLoadSingleRes(char *dirname, char *fname, BOOL doMipmap);

// loads all textures in dirname, returns the load_id for the set
EXTERN int texmemLoadDir(char *dirname);

// goes and frees the texture actually at handle
EXTERN int texmemFreeSingle(int handle);

// frees a single texture by name, not handle
EXTERN int texmemFreeSinglebyName(char *dirname, char *fname);

// get a single texture by name, not handle
EXTERN int texmemGetSinglebyName(char *dirname, char *fname);

// frees the single texture pointed at by this pRes. If doMipmap, then it
// will free the corresponding mipmap memory.
EXTERN int texmemFreeSingleRes(IRes *pRes, BOOL doMipmap);

// frees all textures whose load_id matches the passed in one, which presumeably
// is the one returned by LoadDir when it was called
EXTERN int texmemFreeLoadId(int load_id);

// go through an array of resource handles and convert them to texmem_hnd
// by backpointing through user data, used for resdisk reload fun
EXTERN void texmemGetHandlesFromRes(IRes **res_list, int *hnd_list, int cnt);

// compresses the handles in place, hnd_list must be TEXMEM_MAX long
// and will be filled with the correct new handle for each old handle
// ie. hnd_list[4] will be what the new handle for what 4 was is
EXTERN void texmemCompressHandles(int *hnd_list);

// call this when you change the palette, basically
EXTERN void texmemRebuildDefaultMaps(void);

// set a palette to use with future loads (for 16bit), null for none
EXTERN BOOL texmemSetPalette(char *dirname, char *fname);

// crazy sky/star hack for now
// returns 0 if sky is now sky, 1 if it is now stars
// pass flags below to do the said thing...
EXTERN int texmemSkyStarControl(int new_val);

#define SKYSTAR_QUERY   (-2)
#define SKYSTAR_FLIP    (-1)
#define SKYSTAR_USESKY   (0)
#define SKYSTAR_USESTARS (1)

#endif  // __TEXMEM_H
