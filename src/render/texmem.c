/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/texmem.c,v 1.50 2000/02/14 20:35:21 bfarquha Exp $
// this system tracks and deals with current textures in memory
// knows how to load them and convert into portal format
// as well as how to clear out existing ones and reinit and so on

// @TODO: doesnt really support shipping verse nonshipping
//  must add versions with minimal overhead/silliness

// the idea is that this module does the following:

// - it is the custodian of the actual texture data
//   ie. you ask it (often through macros from textures.h) for all textures or
//   texture handles or whatever, you dont directly access the globals
// - it promises that all textures will be valid at any given time, though many
//   or indeed all of them might be random made up patterns, they will be there
// - it maintains the current active count, and manages swizzling in of names to
//   to the current handle set, doing both block and one by one loads
// - it is primarily used by the family code to set things up correctly

// internally, it stores the following data as it goes

// - current "active count", ie. highest index of a non-dummy texture
// then the per texture sort of data is held in a bunch of arrays
// - an array of grs_bitmap *'s, which are really "r3s_textures"
//     a null entry is stored as a ptr to default_tmap, since it is what we
//     want to draw if a bad reference is made, so dont check here for validity
// - an array of rn_hnd's, which are the resname handle for each texture
//     once again, LRES_NO_HND doesnt mean it is unused, since it could be a
//     texture from another source.
// - an array of integer "load_id's"
//     here is where we can do "loaded or not".  a load_id of 0 means the object
//     is not loaded.  a load_id of -1 means it was anonymously loaded, and >0 is
//     used to indicate a real load_id
//   as each texture load operation happens (single or multiple) it is given
//     a load id.  thus when you load over a family or want to undo the effects
//     of a load op, you can tell the texture system to remove all textures from
//     a particular load set
// - note that texmem[0] is the dummy tmap, and you cannot load over it or replace
//     or free on anything.  currently we check for this, which is ugly

// the basic API it provides is
// - texmemClearAll
//     clears all textures, frees all portal texture memory, resets all internal state
//     leaves the database in a minimal but valid state (sort of the null texture state)
// - int texmemLoadSingleAtIdx(char *dirname, char *fname, int idx)
//     when you know what idx you want to use for texture, returns -1 if it cant do so
//     this will load over the slot if it is currently in use, so be warned
// - int texmemLoadSingle(char *dirname, char *fname)
//     loads fname as a new textures, returns the texmem handle for it, or -1 if none
// - int texmemLoadDir(char *dirname)
//     loads all textures in dirname, returns the load_id for the set
// - int texmemFreeSingle(int handle)
//     goes and frees the texture actually at handle
// - int texmemFreeLoadedSet(int load_id)
//     frees all textures whose load_id matches the passed in one, which presumeably
//     is the one returned by LoadDir when it was called
// note: for the load singles, it uses resname, so you can just set a with up yourself
//   and pass in null for dirname and get the current default with from resname

// it provides the conversion into and out of the portal texture memory setup
//   in the form of a RN_ScanCallback function and a generic load function
//   as well as free calls based on the texture handle (and a RN_thing too...)
// - grs_bitmap *texmem_convert_tmap(grs_bitmap *bm)
//     just does the conversion to portal texture format, returns new map to you
// - void texmem_convert_resname_tmap(grs_bitmap *bm, int idx, rn_hnd hnd, char *udata, void **chunk)
//     does convert tmap, then sets texmem_ globals based on handle
//     and resname userdata and chunk to make the backpointing work
//     note: you can pass NULL udata and chunk to ignore them, and NO_HND for hnd if you want
// the RN_ScanCallback version looks like
// - BOOL _texmem_load(char *name, rn_hnd hnd, int type, char *user_data, void **chunk)
//     this basically parses the setup some, checks the name, then picks the appropriate
//     idx and calls in to resname_tmap_convert
// and of course
// - void texmem_free_tmap(grs_bitmap *bm)
//     frees the actual portal texture map pointed at by bm
// - void texmem_free_idx(int idx)
//     frees up the texture and array usage associated with idx

#include <stdlib.h>
#include <string.h>

#include <lg.h>
#include <g2.h>
#include <mprintf.h>
#include <appagg.h>

#include <portal.h>

#include <storeapi.h>
#include <resapi.h>
#include <imgrstyp.h>
#include <palrstyp.h>
#include <reshack.h>
#include <resdata.h>

#include <objtxtrp.h> // for the horror of texture replace

#include <mipmap.h>
#include <texmem.h>
#include <rnd.h>
#include <rand.h>
#include <palmgr.h>

#include <anim_txt.h> // @ECTS HACK

#include <dbmem.h>

// global arrays holding the texture data
grs_bitmap *texmem_tm[TEXMEM_MAX];
IRes       *texmem_hnd[TEXMEM_MAX];
int         texmem_loadid[TEXMEM_MAX];
char        texmem_align_set[TEXMEM_MAX];  // is this slot's align field valid?
int         texmem_cur_max=0;
int         texmem_cur_loadid=0;
char        texmem_cur_align=TEXMEM_NO_ALIGN;
BOOL        texmem_find_align=FALSE;
BOOL        texmem_out_of_space=FALSE;

// set these "global" callback idxs when you need to get data in to RN callbacks 'n such
// NOTE: all internal users of these set them back to these values
//   so default behavior of no idx setting or anything
// callback_idx of 0 means find a free one
int texmem_callback_idx=TEXMEM_NO_HND;
int texmem_callback_loadid=TEXMEM_LOADID_ANON;

#define TEXMEM_FIRST_RESERVED WATERIN_IDX

#define TEXMEM_NUM_DEFAULT_MAPS 2
static grs_bitmap *texmem_default_maps[TEXMEM_NUM_DEFAULT_MAPS]={NULL,NULL};

static grs_bitmap  _raw_inviso_map;
static grs_bitmap *texmem_inviso_maps=NULL;

#ifdef DBG_ON
static int tm_cnt=0;
#endif

// sky star toggle state control, for now
static int skystar_value=SKYSTAR_USESTARS;

//////////////////////////////////////
// misc internals

//#define NO_CHECKERBOARD

#if defined(NO_CHECKERBOARD) || !defined(PLAYTEST)
grs_bitmap *texmemBuildCheckerboardTmap(int shift, int seed, BOOL trans)
{
   grs_bitmap *bm;               // where we will go
   uchar *dummy_tm;              // store the damn bitmap
   uchar *ipal=palmgr_get_ipal(0);
   int color=ipal?ipal[gr_index_brgb(gr_bind_rgb(5,5,5))]:2;
   int size=1<<shift;

   // need to get a brown/grey
   dummy_tm=(uchar *)Malloc(size*size);
   if (dummy_tm==NULL) return NULL;
   bm=(grs_bitmap *)Malloc(sizeof(grs_bitmap));
   if (bm==NULL) { Free(dummy_tm); return NULL; }
   memset(dummy_tm,color,size*size);
   gr_init_bm(bm,dummy_tm,BMT_FLAT8,0,size,size);
   if (trans) bm->flags|=BMF_TRANS;
   return bm;
}
#else  // playtest
// shift is log 2 of size, seed is if you want a particular thing - stolen from old testcore.c
grs_bitmap *texmemBuildCheckerboardTmap(int shift, int seed, BOOL trans)
{
   grs_bitmap *bm;               // where we will go
   uchar *dummy_tm;              // store the damn bitmap
   int x, y, i;                  // loop vars
   int size, sdown;              // pixel size (not exp), shift down
   int v[4];                     // the 4 colors to use
   RNDSTREAM_STD(rs);

   RndSeed(&rs, seed);     // setup colors
   for (i=0; i<4; i++)     // @TODO: make these colors have contrast
      v[i]=192+(Rnd(&rs)%63); // ie. use the ipal to figure rgb sum, make sure 2 have sums > 128 or something
   if (trans)
      v[Rnd(&rs)&3]=v[Rnd(&rs)&3]=0; // ok, this is really goofy, i admit

   size=1<<shift;                // we are 2n pixels across
   sdown = (shift>=4)?shift-3:0; // 8 or fewer checkers on the texture

   dummy_tm=(uchar *)Malloc(size*size);
   if (dummy_tm==NULL) return NULL;
   bm=(grs_bitmap *)Malloc(sizeof(grs_bitmap));
   if (bm==NULL) { Free(dummy_tm); return NULL; }
   for (x=0; x<size; x++)
      for (y=0; y<size; y++)
         dummy_tm[(x*size)+y]=
            (((x>>sdown)+(y>>sdown))&1) ?
                  ((2*abs((size>>1)-x))>y)?v[0]:v[1] :
                  ((2*abs((size>>1)-x))>y)?v[2]:v[3] ;
   gr_init_bm(bm,dummy_tm,BMT_FLAT8,0,size,size);
   if (trans) bm->flags|=BMF_TRANS;
   return bm;
}
#endif

#ifdef PLAYTEST
uchar texmemUGradient(int x, int y, int size)
{
   return (256*x)/size;
}

uchar texmemVGradient(int x, int y, int size)
{
   return (256*y)/size;
}

uchar texmemUVGradient(int x, int y, int size)
{
   return (128*(x+y))/size;
}

// shift is log 2 of size,
// which = 0 for u gradient, 1 for v, 2 for uv
grs_bitmap *texmemBuildGradientTmap(int shift,uchar (*pixfunc)(int x, int y, int size))
{
   grs_bitmap *bm;               // where we will go
   uchar *dummy_tm;              // store the damn bitmap
   int x, y;                  // loop vars
   int size;              // pixel size (not exp), shift down

   size=1<<shift;                // we are 2n pixels across

   if(!pixfunc)
      return NULL;
   dummy_tm=(uchar *)Malloc(size*size);
   if (dummy_tm==NULL) return NULL;
   bm=(grs_bitmap *)Malloc(sizeof(grs_bitmap));
   if (bm==NULL) { Free(dummy_tm); return NULL; }
   for (x=0; x<size; x++)
      for (y=0; y<size; y++)
         dummy_tm[(x*size)+y]=
            pixfunc(x,y,size);
   gr_init_bm(bm,dummy_tm,BMT_FLAT8,0,size,size);
   return bm;
}
#endif // else !__PLAYTEST

// scan for a free handle, since with multifamily we may have holes in the texture list
int _texmem_get_and_use_free_handle(void)
{
   int i=0;

   for (i=0; i<texmem_cur_max; i++)
      if (texmem_loadid[i]==TEXMEM_LOADID_NULL)
         break;
   if (i>=TEXMEM_FIRST_RESERVED)
   {
      texmem_out_of_space=TRUE;
      Warning(("texmem_get_handle: failed to find new handle\n"));
      return TEXMEM_NO_HND;
   }
   if (i==texmem_cur_max)
      texmem_cur_max++;   // adding to end of list, so move said end back
   return i;
}

static void _texmem_unuse_handle(int handle)
{
   if (handle==texmem_cur_max-1)
      texmem_cur_max--;
}

///////////////////////////////////////
// core texture conversion and set up

// wrapper on the portal create mipmapped texture stuff
// in dbg, keeps a count to try and stay on top of usage
grs_bitmap *texmem_convert_tmap(grs_bitmap *bm)
{
   DoIfDBG(tm_cnt++);
   return create_mipmapped_texture(bm);
}

// actually frees the texture mem from the pointer
void texmem_free_tmap(grs_bitmap *bm)
{
   Assrt(bm!=NULL);
#ifdef DBG_ON
   AssertMsg((tm_cnt--)>0,"texmem_free_tmap but cnt is already 0\n");
#endif
   if (bm->align!=0)
      palmgr_release_slot(bm->align);
   free_mipmapped_texture(bm,TRUE);
}

// raw free based on idx - silent check
static BOOL texmem_core_free_idx(int idx)
{
   if (texmem_loadid[idx]<=TEXMEM_LOADID_NULL)
      return FALSE;   //
   ectsAnimTxtCheckFree(texmem_hnd[idx]);
   if (texmem_tm[idx]!=NULL)
   {
      texmem_free_tmap(texmem_tm[idx]);
      texmem_tm[idx]=texmem_default_maps[0];
   }
   if (texmem_hnd[idx] != NULL)
   {
      IResHack *pResHack;
      ObjTxtReplaceDropIRes(texmem_hnd[idx]);
      FreeAppRes(texmem_hnd[idx]);
      IRes_Unlock(texmem_hnd[idx]);
      if (SUCCEEDED(COMQueryInterface(texmem_hnd[idx],
                                      IID_IResHack,
                                      (void **) &pResHack)))
      {
         IResHack_SetData(pResHack, NULL);
         SafeRelease(pResHack);
      }
      SafeRelease(texmem_hnd[idx]);
   }
   texmem_loadid[idx]=TEXMEM_LOADID_NULL;
   if (texmem_align_set[idx]!=TEXMEM_NO_ALIGN)
      texmem_align_set[idx]=TEXMEM_NO_ALIGN;
   if (idx==texmem_cur_max-1)   // in other words, idx is last one in the list
   {
      while (idx>0)             // while there are places
         if (texmem_loadid[idx]==TEXMEM_LOADID_NULL)
            idx--;              // this was null, keep going down
         else
            break;              // oops, real data, better stop
      texmem_cur_max=idx+1;     // cur max is one past the one with data
   }
   return TRUE;
}

// free based on idx, with check for validity
static BOOL texmem_free_idx(int idx)
{
   if (texmem_loadid[idx]<=TEXMEM_LOADID_NULL)
   {
      Warning(("free_idx: called with a bad idx %d loadid %d\n",idx,texmem_loadid[idx]));
      return FALSE;
   }
   return texmem_core_free_idx(idx);
}

///////////////////////////////////////
// resname callbacks

#define MAX_POWER 11
static BOOL _texmem_check_size(int w, int h)
{
   BOOL rv=TRUE;
   int i;

   for (i=0; i < MAX_POWER; ++i)
      if (w == (1 << i)) break;
   if (i == MAX_POWER) rv=FALSE;

   for (i=0; i < MAX_POWER; ++i)
      if (h == (1 << i)) break;
   if (i == MAX_POWER) rv=FALSE;

   return rv;
}

// get the alignment field for this unknown thing
// Get the palette for this resource, which is a stream in the same
// storage, with the name "full".  Or in Deep Cover, get the palette
// from the resource itself.  Cuz we rebels.
static int _texmem_get_align(IRes *pRes)
{
   int pal_index = 0;

   // Get the storage that this thing was in:
   IStore *pStore = IRes_GetStore(pRes);
   IResMan *pResMan = AppGetObj(IResMan);
   // And get the palette from there:
   // @TBD: This is a little awkward. On the other hand, it's a pretty rare
   // case, and it isn't clear that it's worth putting extra support into
   // the resource system for it.
   // @NOTE: This will pull the palette from the same canonical path as the
   // image; this *could* be from the corresponding storage earlier in the
   // global context path. I'm considering this a feature, since it allows
   // us to play with the palette independently from the images if desired...
   IRes *pPalRes;
   char *pCanonPath;
   IStore_GetCanonPath(pStore, &pCanonPath);

#ifdef DEEPC
   // In Deep Cover we get the palette from the texture.  So if we want,
   // we can have a separate palette for every texture.
   pPalRes = IResMan_Bind(pResMan, IRes_GetName(pRes), RESTYPE_PALETTE,
                          NULL, pCanonPath, 0);
#else // DEEPC
   pPalRes = IResMan_Bind(pResMan, "full", RESTYPE_PALETTE, NULL, pCanonPath, 0);
#endif // DEEPC

   if (pPalRes) {
      // Okay, we've got the palette -- set it up and dump it
      pal_index = palmgr_alloc_pal(IRes_Lock(pPalRes));
      IRes_Unlock(pPalRes);
      SafeRelease(pPalRes);
   }
   Free(pCanonPath);


   SafeRelease(pResMan);
   SafeRelease(pStore);

   return pal_index;
}

int ectsSecretLoadIdx=0;
int ectsSecretLoadCopy[256];

//
// This replaces the old _texmem_load. Instead of being called *during*
// the resource load, it happens immediately after, or as a callback when
// we are loading many. It makes the new texture resource behave the way
// we want it to.

//
// @TBD: This may yet need to get upgraded to recognize resources we don't
// want, such as non-images.
//
BOOL texmem_interpret_new_res(IRes *pRes,
                              IStore *pStore,
                              void *pClientData)
{
   int use_idx=texmem_callback_idx;
   grs_bitmap *pbm;
   grs_bitmap *pbm_new;
   IResHack *pResHack;
   int use_align=texmem_cur_align;

#ifdef DEEPC
   // In Deep Cover we get the palette from the image itself.
   IResMan *pResMan = AppGetObj(IResMan);
   IStore *pS = IRes_GetStore(pRes);
   IRes *pPalRes;
   char *pCanonPath;

   IStore_GetCanonPath(pS, &pCanonPath);
   pPalRes = IResMan_Bind(pResMan, IRes_GetName(pRes), RESTYPE_PALETTE,
                          NULL, pCanonPath, 0);
   if (pPalRes) {
      use_align = palmgr_alloc_pal(IRes_Lock(pPalRes));
      IRes_Unlock(pPalRes);
      SafeRelease(pPalRes);
   } else
      Warning(("texmem: can't find palette for %s.  Using default.\n",
               IRes_GetName(pRes)));
#endif // DEEPC

   // if our name is "full", we should punt
   if (stricmp(IRes_GetName(pRes),"full")==0)
   {
      return FALSE;
   }

   if (FAILED(COMQueryInterface(pRes,
                                IID_IResHack,
                                (void **) &pResHack)))
   {
      Warning(("texmem: Couldn't get ResHack interface for resource!\n"));
      return FALSE;
   }

   if (IResHack_HasSetData(pResHack)) {
      // It's already been dealt with, so we don't need to do anything
      // new. However, we still need to tell the animation system what
      // texture this is, through this unspeakably awful hack.
      // @HACK: We really need to replace this mechanism, which is used
      // to store the txt_id away so it can then be passed through to
      // ectsAnimTxtCheckLoad()...
      // @note: this also does align field palette setting, which we had better not
      //        "clean up" if we ever change the animation model
      use_idx = GetResTexIdx(pRes);
      // We *may* also need to establish the palette for this texture.
      if (texmem_find_align) {
         use_align=_texmem_get_align(pRes);
         if (use_align!=TEXMEM_NO_ALIGN)
         {
            pbm = (grs_bitmap *) IRes_Lock(pRes);
            palmgr_add_ref_slot(use_align);
            pbm->align=use_align;
            IRes_Unlock(pRes);
         }
      }
      if (use_idx != TEXMEM_NO_HND)
      {
         ectsSecretLoadCopy[ectsSecretLoadIdx++] = use_idx;
         texmem_align_set[use_idx] = use_align;
      }
      SafeRelease(pResHack);
      return TRUE;  // ?????
   }

   // If this is an animation frame, and anim_txt isn't ready for it,
   // then ignore it. This will return TRUE iff the name is a valid
   // animation frame name *and* anim_txt isn't prepared to deal with it.
   if (ectsAnimTxtIgnore(IRes_GetName(pRes))) {
      SafeRelease(pResHack);
      return FALSE;
   }

   pbm = (grs_bitmap *) IRes_Lock(pRes);

   if (pbm==NULL)
   {
      Warning(("fam_cvrt_img: %s cannot be locked\n",IRes_GetName(pRes)));
      IRes_Unlock(pRes);
      SafeRelease(pResHack);
      return FALSE;
   }

   if (!_texmem_check_size(pbm->w,pbm->h))
   {  // It's the wrong size
#ifndef SHIP
      mprintf("fam_cvrt_img: %s is %dx%d.. not valid\n",IRes_GetName(pRes),pbm->w,pbm->h);
#endif
      IRes_Unlock(pRes);
      SafeRelease(pResHack);
      return FALSE;
   }

   if (texmem_callback_idx==0)
   {
      use_idx=_texmem_get_and_use_free_handle();
      if (use_idx==TEXMEM_NO_HND)
      {
         // Pretty serious; we can't get ourselves a handle.
         IRes_Unlock(pRes);
         SafeRelease(pResHack);
         return FALSE;
      }
   }

   if (texmem_find_align)
      use_align=_texmem_get_align(pRes);

   if (use_align!=TEXMEM_NO_ALIGN)
   {
      palmgr_add_ref_slot(use_align);
      pbm->align=use_align;
   }

   // Create a new, mipmapped version of this bitmap, in portal memory.
   // Then substitute it in for the resource's own data, and tell it
   // to look at this index.
   pbm_new = texmem_convert_tmap(pbm);
   IRes_Unlock(pRes);
   if (use_idx != TEXMEM_NO_HND)
   {
      texmem_tm[use_idx]     = pbm_new;
      texmem_hnd[use_idx]    = pRes;
      COMAddRef(pRes);
   }
   IResHack_SetData(pResHack, pbm_new);
   SafeRelease(pResHack);
   IRes_Lock(pRes);
   SetResTexIdx(pRes, use_idx);

   if (use_idx != TEXMEM_NO_HND)
   {
      texmem_loadid[use_idx] = texmem_callback_loadid;
      texmem_align_set[use_idx] = use_align;
      ectsSecretLoadCopy[ectsSecretLoadIdx++] = use_idx;
   }

   return TRUE;
}

//////////////////////////////////////
// basic load/free API

// setup a silly default texture thing
static grs_bitmap *setup_default_tmap(int size, BOOL trans)
{
   grs_bitmap *bm, *orig_bm;

   orig_bm=texmemBuildCheckerboardTmap(size,0,trans);
   bm=texmem_convert_tmap(orig_bm);
   Free(orig_bm->bits);
   Free(orig_bm);
   return bm;
}

// a black 1x1 tmap
extern bool mipmap_enable;
static uchar _null_bits[16*16];
static grs_bitmap *setup_inviso_tmaps(void)
{
   grs_bitmap *bm;
   bool tmp=mipmap_enable;

   memset(_null_bits,0,16*16*sizeof(uchar));
   gr_init_bm(&_raw_inviso_map,_null_bits,BMT_FLAT8,0,16,16);
   mipmap_enable=FALSE;
   bm=texmem_convert_tmap(&_raw_inviso_map);
   mipmap_enable=tmp;
   return bm;
}

static void check_and_free_default_tmap(grs_bitmap **bmptr)
{
   grs_bitmap *bm;

   if (bmptr==NULL) return;
   bm=*bmptr;
   if (bm)
   {
      texmem_free_tmap(bm);
      *bmptr=NULL;
   }
}

void _texmemMakeDefMaps(void)
{
   texmem_default_maps[0]=setup_default_tmap(5,FALSE);
   texmem_default_maps[1]=setup_default_tmap(4,TRUE);
   if (texmem_inviso_maps==NULL)
      texmem_inviso_maps=setup_inviso_tmaps();
}

// assumes that someone changed the palette, so maps should be rebuilt
void texmemRebuildDefaultMaps(void)
{
   int uses_which[TEXMEM_MAX], i, j;
   for (i=0; i<TEXMEM_MAX; i++)
   {
      uses_which[i]=-1;
      for (j=0; j<TEXMEM_NUM_DEFAULT_MAPS; j++)
         if (texmem_default_maps[j]==texmem_tm[i])
         {
            uses_which[i]=j;
            break;
         }
   }
   // now go rebuild the maps
   for (i=0; i<TEXMEM_NUM_DEFAULT_MAPS; i++)
      check_and_free_default_tmap(&texmem_default_maps[i]);
   _texmemMakeDefMaps();
   for (i=0; i<TEXMEM_MAX; i++)
      if (uses_which[i]!=-1)
         texmem_tm[i]=texmem_default_maps[uses_which[i]];
}

void _texmemUnloadAll(void)
{
   int i;

   for (i=1; i<TEXMEM_MAX; i++)
      if (texmem_loadid[i]>=TEXMEM_LOADID_ANON)
         texmemFreeSingle(i);
   for (i=0; i<TEXMEM_NUM_DEFAULT_MAPS; i++)
      check_and_free_default_tmap(&texmem_default_maps[i]);
   check_and_free_default_tmap(&texmem_inviso_maps);

#ifdef DBG_ON
   if (tm_cnt>0)
      Warning(("Tried unload all texmem, but tm_cnt %d\n",tm_cnt));
   else
   {
      // this is a bad idea, cause of the surface cache, i suppose, eh?
//      portal_free_all_mem_rects();
   }
#endif
}

static void texmemSetLocked(int idx, r3s_texture tm)
{
   texmem_tm[idx]=tm;
   texmem_loadid[idx]=TEXMEM_LOADID_LOCK;
}

// returns 0 if sky is sky, 1 if it is stars
// uses SKYSTAR defines to decide what to do
int texmemSkyStarControl(int new_val)
{
   switch (new_val)
   {
      case SKYSTAR_QUERY:
         break;
      case SKYSTAR_FLIP:
         if (skystar_value==SKYSTAR_USESKY)
            new_val=SKYSTAR_USESTARS;
         else
            new_val=SKYSTAR_USESKY;
      case SKYSTAR_USESKY:
      case SKYSTAR_USESTARS:
         skystar_value=new_val;
         if (skystar_value==SKYSTAR_USESKY) {
            texmemSetLocked(BACKHACK_IDX,NULL);
            portal_setup_star_hack(-1);
         } else {
            texmemSetLocked(BACKHACK_IDX,texmem_inviso_maps);
            portal_setup_star_hack(BACKHACK_IDX);
         }
         break;
   }
   return skystar_value;
}

// sets current align palette to dirname/fname
BOOL texmemSetPalette(char *dirname, char *fname)
{
   if (fname!=NULL)
   {
      IRes *pRes;
      IResMan *pResMan = AppGetObj(IResMan);

      pRes = IResMan_Bind(pResMan, fname, RESTYPE_PALETTE, NULL, dirname, 0);
      SafeRelease(pResMan);
      if (pRes)
      {
         texmem_cur_align = palmgr_alloc_pal(IRes_Lock(pRes));
         IRes_Unlock(pRes);
         SafeRelease(pRes);
         return TRUE;
      }
      else
      {
#ifndef DEEPC
         Warning(("Could not find palette %s%s\n",dirname,fname));
#endif // ~DEEPC
         texmem_cur_align=TEXMEM_NO_ALIGN;
         return FALSE;  // dont fall through and release the slot we never allocated!!!!
      }
   }
   if (texmem_cur_align!=TEXMEM_NO_ALIGN)
      palmgr_release_slot(texmem_cur_align);
   texmem_cur_align=TEXMEM_NO_ALIGN;
   return FALSE;
}

// clears all textures, frees all portal texture memory, resets all internal state
// leaves the database in a minimal but valid state (sort of the null texture state)
void texmemClearAll(void)
{
   int i;

   _texmemUnloadAll();
   _texmemMakeDefMaps();

   for (i=0; i<TEXMEM_MAX; i++)   // this is basically a structure initialization, sadly
   {
      texmem_loadid[i]=TEXMEM_LOADID_NULL;
      texmem_hnd[i]=NULL;
      texmem_tm[i]=texmem_default_maps[0];
      texmem_align_set[i]=TEXMEM_NO_ALIGN;
   }
   texmemSetLocked(0,texmem_default_maps[0]);
   texmemSetLocked(WATERIN_IDX,texmem_default_maps[1]);
   texmemSetLocked(WATEROUT_IDX,texmem_default_maps[1]);

   // this should be NULL for normal sky_hack, inviso_maps for star_hack
   texmemSkyStarControl(skystar_value);

   texmem_cur_max=1;                      // initially start at 1, since 0 is secret
   texmem_cur_loadid=TEXMEM_LOADID_BASE;
   texmem_out_of_space=FALSE;
   texmem_cur_align=TEXMEM_NO_ALIGN;
}

void texmemShutdown(void)
{
   _texmemUnloadAll();
   // other cleanup/counting tasks
   texmem_cur_max=0xdeadbeef; // hopefully we will hear if this get used, eh?
}

// when you know what idx you want to use for texture, returns -1 if it cant do so
// this will load over the slot if it is currently in use, so be warned
int texmemLoadSingleAtIdx(char *dirname, char *fname, int idx)
{
   IResMan *pResMan;
   IStore *pStore = NULL;
   IRes *pRes;
   BOOL success;

   if ((idx<=0)||(idx>=TEXMEM_MAX))
   {
      Warning(("Hey! loadsingle at idx called with bad idx\n"));
      return TEXMEM_NO_HND;
   }
   texmem_core_free_idx(idx);  // dont want to be warned if its unused
   if (idx == texmem_cur_max)
      texmem_cur_max++;
   texmem_callback_idx=idx;
   ectsSecretLoadIdx=0;

   // Actually load the name image, then mipmap it...
   pResMan = AppGetObj(IResMan);
   pRes = IResMan_Bind(pResMan, fname, RESTYPE_IMAGE, NULL, dirname, 0);
   SafeRelease(pResMan);

   if (!pRes) {
      Warning(("Texmem: Trying to load missing image %s\n", fname));
      return -1;
   }

   success=texmem_interpret_new_res(pRes, pStore, NULL);
   texmem_callback_idx=TEXMEM_NO_HND;

   if (success && !ectsAnimLoading)
      ectsAnimTxtCheckLoad(pRes, TRUE, dirname, ectsSecretLoadCopy[0]);
   SafeRelease(pRes);
   if (!success) // since we dont want it around if it didnt load
   {
      SafeRelease(pRes);
      return TEXMEM_NO_HND;
   }
   return idx;
}

// loads fname as a new textures, returns the texmem handle for it, or -1 if none
int texmemLoadSingle(char *dirname, char *fname)
{
   int idx=_texmem_get_and_use_free_handle(), new_hnd;
   if (idx==TEXMEM_NO_HND) return idx;              // look, no free handle
   new_hnd=texmemLoadSingleAtIdx(dirname,fname,idx); // else try the load to that handle
   if (new_hnd==TEXMEM_NO_HND)
      _texmem_unuse_handle(idx);
   return new_hnd;
}

// load dirname/fname, and return the resource. Replaces
// texmemLoadSingleToRnHnd. If doMipmap is TRUE, then it will run the
// loaded image through texmem_interpret_new_res.
//
// Returns NULL if the named image couldn't be found.
IRes *texmemLoadSingleRes(char *dirname, char *fname, BOOL doMipmap)
{
   IResMan *pResMan = AppGetObj(IResMan);
   IRes *pRes;

   pRes = IResMan_Bind(pResMan, fname, RESTYPE_IMAGE, NULL, dirname, 0);
   SafeRelease(pResMan);
   if (pRes)
      if (doMipmap)
      {
         if (!texmem_interpret_new_res(pRes, NULL, NULL))
         {
            SafeRelease(pRes);
            return NULL;
         }
      }
      else  // Make sure it's locked into memory
         IRes_Lock(pRes);
   return pRes;
}

// loads all textures in dirname, returns the load_id for the set
int texmemLoadDir(char *dirname)
{
   IResMan *pResMan = AppGetObj(IResMan);
   texmem_callback_idx=0;
   texmem_callback_loadid=texmem_cur_loadid++;
   ectsSecretLoadIdx=0;

   IResMan_BindAll(pResMan, "*.*", RESTYPE_IMAGE, NULL,
                   texmem_interpret_new_res, NULL, dirname, 0);

   texmem_callback_idx=TEXMEM_NO_HND;
   texmem_callback_loadid=TEXMEM_LOADID_ANON;
   {  // @ECTS: this is really hideous ects hell
      int i;
      for (i=0; i<ectsSecretLoadIdx; i++)
         if (ectsSecretLoadCopy[i]!=0)
            ectsAnimTxtCheckLoad(texmem_hnd[ectsSecretLoadCopy[i]],TRUE,dirname,ectsSecretLoadCopy[i]);
         else break;
   }
   return texmem_cur_loadid-1;
   SafeRelease(pResMan);
}

// goes and frees the texture actually at handle, returns 1 if freed, else 0
//  ie. returns the "count" of successful frees
int texmemFreeSingle(int handle)
{
   return texmem_core_free_idx(handle)?1:0;
}

// use a texture name to free a given texture
#pragma off(unreferenced)
int texmemFreeSinglebyName(char *dirname, char *fname)
{
   IResMan *pResMan = AppGetObj(IResMan);
   // @TBD: does this work? This code has never actually used dirname before...
   IRes *pRes = IResMan_Lookup(pResMan, fname, RESTYPE_IMAGE, dirname);
   SafeRelease(pResMan);
   if (pRes)
   {
      uchar texhnd = GetResTexIdx(pRes);
      SafeRelease(pRes);
      return texmem_free_idx(texhnd)?1:0;
   }
   return 0;
}

int texmemGetSinglebyName(char *dirname, char *fname)
{
   IResMan *pResMan = AppGetObj(IResMan);
   IRes *pRes;
   // @TBD: does this work? This code has never actually used dirname
   // before...
   pRes = IResMan_Lookup(pResMan, fname, RESTYPE_IMAGE, dirname);
   SafeRelease(pResMan);
   if (pRes)
   {
      uchar texhnd = GetResTexIdx(pRes);
      SafeRelease(pRes);
      return (int)texhnd;
   }
   return TEXMEM_NO_HND;
}
#pragma on(unreferenced)

// frees the single texture pointed at by this rn_hnd
// this is spooky, since we first need to decide if we are the "final reference"
//  and only do the texmem_free_tmap if we are
// returns 1 iff we freed the last intance of the handle
// Releases the handle, as well as freeing it.
//
// texmemFreeSingleRnHnd has been replaced by texmemFreeSingleRes.
//
// @TBD: This whole business with doing IRes_SetData is an unspeakably
// ugly hack, and should go away ASAP. It should be replaced by a proper
// DarkMipmapTexture resource type, which does its own memory management
// and translates from the Image type.
//
// Note that this ABSOLUTELY COUNTS on pRes having been munged with
// SetData. This isn't an ordinary resource, it's an assigned one.
int texmemFreeSingleRes(IRes *pRes, BOOL doMipmap)
{
   grs_bitmap *bm = (grs_bitmap *)IRes_Lock(pRes);
   IRes_Unlock(pRes);
   // Now, free the permanent lock:
   IRes_Unlock(pRes);
   if (IRes_Drop(pRes)) {
      // Okay, there aren't any more outstanding locks:
      if (doMipmap) {
         texmem_free_tmap(bm);
      }
      FreeAppRes(pRes);
      // Release the ref on this resource. This feels really weird, but
      // it parallels texmemLoadSingleRes, whence this IRes comes...
      SafeRelease(pRes);
      return 1;
   }
   return 0;
}

// frees all textures whose load_id matches the passed in one, which presumeably
// is the one returned by LoadDir when it was called
int texmemFreeLoadId(int load_id)
{
   int i, cnt=0;

   if (load_id>0)       // if it is a valid load_id
      for (i=1; i<TEXMEM_MAX; i++)
         if (texmem_loadid[i]==load_id)
         {
            texmemFreeSingle(i);
            cnt++;
         }
   return cnt;
}

// Take the given array of resource pointers, and fill the corresponding
// array of texture handles. Replaces texmemConvertRnHndList. Used by
// family.
void texmemGetHandlesFromRes(IRes **res_list, int *hnd_list, int cnt)
{
   int i;
   for (i = 0; i < cnt; i++) {
      if (res_list[i]) {
         hnd_list[i] = (int)GetResTexIdx(res_list[i]);
      } else {
         hnd_list[i] = NULL;
      }
   }
}

// compresses the handles in place, hnd_list must be TEXMEM_MAX long
// and will be filled with the correct new handle for each old handle
// ie. hnd_list[4] will be what the new handle for what 4 was is
void texmemCompressHandles(int *hnd_list)
{
   int i, j;
   for (i=0, j=0; i<WATERIN_IDX; i++)
      if (texmem_loadid[i]!=TEXMEM_LOADID_NULL)
      {
         hnd_list[i]=j;
         if (i!=j)
         {
            texmem_tm[j]=texmem_tm[i];
            texmem_hnd[j]=texmem_hnd[i];
            texmem_loadid[j]=texmem_loadid[i];
            texmem_align_set[j]=texmem_align_set[i];
            SetResTexIdx(texmem_hnd[j], j);
            texmem_tm[i]=texmem_default_maps[0];
            texmem_hnd[i]=NULL;
            texmem_loadid[i]=TEXMEM_LOADID_NULL;
            texmem_align_set[i]=TEXMEM_NO_ALIGN;
         }
         j++;
      }
      else
         hnd_list[i]=-1;
   for (i=WATERIN_IDX; i<TEXMEM_MAX; i++)
      hnd_list[i]=i;
   texmem_cur_max=j;
   ectsAnimTxtRemapAll(hnd_list,i);
}

