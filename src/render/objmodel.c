/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/objmodel.c,v 1.52 2000/02/13 20:10:37 adurant Exp $
// this module deals with loading, freeing, and texture managing of
// renderer models.

#include <lg.h>
#include <appagg.h>
#include <stdlib.h>
#include <string.h>

#include <config.h>
#include <mprintf.h>

#include <palmgr.h>

#include <storeapi.h>
#include <resapi.h>
#include <palrstyp.h>
#include <imgrstyp.h>
#include <binrstyp.h>
#include <mdlrstyp.h>

#include <objmodel.h>
#include <objtxtrp.h>

#include <status.h>
#include <texmem.h>
#include <md.h>
#include <mm.h>
#include <mms.h>

//#include <mds.h> // XXX just for testing

#include <anim_txt.h>
#include <resdata.h> 

#include <report.h>

#include <dbmem.h>

#include<lgd3d.h>
#ifndef USE_HARDWARE3D
   #include<tmgr.h>
#endif

ISearchPath *MDModelPath;
ISearchPath *MDModelTexturesPath;
ISearchPath *MeshModelPath;
ISearchPath *MeshModelTexturesPath;
ISearchPath *FXPath;
ISearchPath *FXTexturesPath;
ISearchPath *BitmapPath;
ISearchPath *BitmapTexturesPath;

#define MD_MODEL_PATH            "obj\\"
#define MESH_MODEL_PATH          "mesh\\"
#define MD_MODEL_HIGH_PATH       "obj\\hipoly\\"
#define MESH_MODEL_HIGH_PATH     "mesh\\hipoly\\"
#define FX_PATH                  "sfx\\txt\\"
#define BITMAP_PATH              "bitmap\\"

#define TXT_PATH_16              "txt16\\"
#define TXT_PATH_8               "txt\\"

typedef struct ModelHandle
{
   IRes *mhandle;
   IRes **texptr;     // pointer to texture handles
   ushort numtex;
   ushort refs;       // # objects using the model, so we can free if nobody is
   short type;
   bool used;         // cant this be a flags field with the above, or something
   uchar replace_idx; // index into the replace list array
} ModelHandle;

// for each REPL model, here is the list of which TNum IDX's to replace
// ie, if ReplaceList[i]={2,6} it means Repl0 uses vtxt[2], and Repl1 uses vtxt[6]
// replace_idx in the MH points into this, and it is inited to -1 on startup
#define MAX_REPL_MODELS  (255)
#define REPL_LIST_UNUSED (-1)
static char g_aReplaceList[MAX_REPL_MODELS][MAX_REPL_TXT];
static ModelHandle g_aModelHandles[MAX_OBJMODELS];

#ifndef SHIP         
BOOL g_bTextureDebug = FALSE;
int  g_ModelHandleMax = 0;
#endif

BOOL g_highpoly_characters=FALSE;

typedef void (*TextureLoadFunc)(ModelHandle *mh, char *objName);

// static function declarations
static void DummyTextureLoad(ModelHandle *mh, char *objName);
static void MdTextureLoad(ModelHandle *mh, char *objName);
static void MeshTextureLoad(ModelHandle *mh, char *objName);
static void SparkTextureLoad(ModelHandle *mh, char *objName);
static void BitmapTextureLoad(ModelHandle *mh, char *objName);

// should have as many entries as there are supported model types
static TextureLoadFunc g_afnTextureLoadFuncs[] =
{
    DummyTextureLoad,           // wedge
    MdTextureLoad,
    MeshTextureLoad,
    DummyTextureLoad,           // heat rendering effect: no textures
    SparkTextureLoad,           // spark effect
    DummyTextureLoad,           // particle rendering effect: no textures
    BitmapTextureLoad,          // animated bitmap
};

#define IS_VALID_MH_INDEX(idx) ((idx) >= 0 && (idx) < MAX_OBJMODELS)

#define VALIDATE_MH_INDEX(idx,fname) \
   AssertMsg2( IS_VALID_MH_INDEX(idx), \
      "%s: passed invalid model index %d\n", fname, idx);

// other globals
static grs_bitmap *g_pDummyObjTmap;

//////////////
// lets do some managing of the ReplaceList
static void init_replace_list(void)
{
   memset(g_aReplaceList,REPL_LIST_UNUSED,MAX_REPL_MODELS*MAX_REPL_TXT);
}

// find a free replace idx 
static int _get_replace_idx(void)
{
   int i;
   for (i=1; i<MAX_REPL_MODELS; i++)
      if (g_aReplaceList[i][0]==REPL_LIST_UNUSED)
         return i;
   CriticalMsg("ObjModel: Out of Space in ReplaceList");
   return -1;
}

static void _clear_replace_idx(int idx)
{
   int i;
   if (idx!=0)
      for (i=0; i<MAX_REPL_TXT; i++)
         g_aReplaceList[idx][i]=REPL_LIST_UNUSED;
}

// see if this is a replace sort of texture, ret -1 if no, else 0-MAX_REPL_TXT-1
static int _check_replace_txt(char *name)
{
   if (strnicmp(name,"replace",7)==0)
   {
      int rep_val=name[7]-'0';
      if (rep_val>=MAX_REPL_TXT)
         Warning(("ObjTxtReplace: Texture named %s has invalid replace #\n",name));
      else
         return rep_val;
   }
   return -1;
}

static BOOL _check_and_alloc_replace_txt(ModelHandle *mh, int idx, char *txt_name)
{
   int rep_num=_check_replace_txt(txt_name);
   if (rep_num==-1) return FALSE;
   if (mh->replace_idx==0)
      mh->replace_idx=_get_replace_idx();
   g_aReplaceList[mh->replace_idx][rep_num]=idx;
   return TRUE;
}

// @TBD (justin 6-12-98): What the heck is happening here? This is getting
// called back from certain resources. What is it really trying to do, and
// how can we accomplish the same goal some other way?
static void _objm_pal_callback(uchar *pal, uchar *bitmap)
{
   grs_bitmap *bm=(grs_bitmap *)bitmap;
   bm->align=palmgr_alloc_pal(pal);
}

// @TBD: For the moment, we are Locking the model resources as soon as we find
// them. It is being unlocked in FreeTextures.
// In the long run, we should try to be subtler and smarter.
static IRes * _do_raw_load(ISearchPath *pPath, char *pName)
{
   IResMan *pResMan = AppGetObj(IResMan);
   IRes *pRes = IResMan_Bind(pResMan, pName, RESTYPE_IMAGE, pPath, NULL, 0);
   SafeRelease(pResMan);
   if (pRes)
      IRes_Lock(pRes);
   return pRes;
}

#ifndef SHIP
static BOOL silent_common_load = FALSE;
#define set_silent_load(val) silent_common_load=val
#else
#define set_silent_load(val)
#endif

static BOOL allow_16bit_obj_textures = TRUE;
static BOOL using_portal_models = FALSE;
static IRes * (*_txm_load)(ISearchPath *pPath, char *pName)=_do_raw_load;


// @TBD: We don't appear to be using objName. Should we remove it?
static void common_texture_load(ModelHandle *mh, ISearchPath *pPath, 
                                char *name, int idx, char *objName)
{  // these need to be turned into portal textures, really
   IRes *pRes;
   char *pCanonPath;

   // we want to go ahead and load the replace texture too
   // so it can be seen if the props arent set, so fall through
   // here regardless of whether we did or didnt have a replace name
   _check_and_alloc_replace_txt(mh,idx,name);
   
   pRes = (*_txm_load)(pPath, name);
   mh->texptr[idx] = pRes;
   if (!pRes)
   {
#ifndef SHIP      
      if (!silent_common_load)
         Warning(("Couldn't load texture %s!\n", name));
#endif      
      return;
   }
   
   // Some textures may have their own palettes, which need to be
   // dealt with. Get the storage information about this texture, to
   // figure out whether it comes from a "16 bit" storage, which are
   // the ones that need their palettes loaded.
   if (allow_16bit_obj_textures) {
      IStore *pCanonStore = IRes_GetCanonStore(pRes);
      // If the bottom of the storage's path matches TXT_PATH_16, then
      // we need it:
      if (!stricmp(IStore_GetName(pCanonStore), TXT_PATH_16)) {
         // Okay. Get the palette...
         IResMan *pResMan = AppGetObj(IResMan);
         IRes *pPallRes = IResMan_Retype(pResMan, pRes, RESTYPE_PALETTE, 0);
         if (pPallRes) {
            void *pPall = IRes_Lock(pPallRes);
            grs_bitmap *pbm = (grs_bitmap *) IRes_Lock(pRes);
            {
               uchar b = pbm->align;
               pbm->align = palmgr_alloc_pal(pPall);
#ifdef DBG_ON
               if ((pbm->align==0) && (config_is_defined("wastedpalettes")))
                 mprintf("Object %s uses txt16, but is in game palette for %s.\n",objName,name);
#endif
               if( b != pbm->align )
                  if( (g_tmgr!=NULL)&&(pbm->flags&BMF_LOADED)){
                     lgd3d_unload_texture(pbm);
                     lgd3d_load_texture(pbm);
                  }
            }
            //pbm->align = palmgr_alloc_pal(pPall);
            IRes_Unlock(pRes);
            IRes_Unlock(pPallRes);
            // We're done with the palette; the palette manager will now
            // take charge of it. So we can drop it from memory.
            IRes_Drop(pPallRes);
            SafeRelease(pPallRes);
         }
         SafeRelease(pResMan);
      }
      SafeRelease(pCanonStore);
   }
   
   // Now, check whether this is an animated texture, and load up the
   // frames if so:
   IRes_GetCanonPath(pRes, &pCanonPath);
   ectsAnimTxtCheckLoad(pRes, using_portal_models, pCanonPath, NULL);
   if (pCanonPath)
      Free(pCanonPath);
}

/////// Ref counters  /////////

void objmodelIncRef(int idx)
{
   VALIDATE_MH_INDEX(idx,"objmodelIncRef");

   g_aModelHandles[idx].refs++;
}


static BOOL FreeModel(int idx,BOOL drop);

void objmodelDecRef(int idx)
{
   VALIDATE_MH_INDEX(idx,"objmodelDecRef");

#ifdef DBG_ON
   if(g_aModelHandles[idx].refs==0)
   {
      Warning(("objmodelDecRef: bad refs for model %d\n",idx));
      return;
   }
#endif

   g_aModelHandles[idx].refs--;
   if (g_aModelHandles[idx].refs==0)
      FreeModel(idx,FALSE);
}

static int GetFreeModelIndex(void)
{
   int i;

   for (i = 0; i < MAX_OBJMODELS; ++i)
      if (g_aModelHandles[i].used == FALSE)
      {
#ifndef SHIP         
         if (i>g_ModelHandleMax)
            g_ModelHandleMax=i;
#endif         
         return i;
      }

   Warning(("GetFreeModelIndex: no free object model idx!\n"));
   return MIDX_INVALID;
}

///////// local model texture managers ////////

// You know, on reflection, those wedges all look pretty much the same.
static void DummyTextureLoad(ModelHandle *mh, char *objName)
{
   return;
}

static void MdTextureLoad(ModelHandle *mh, char *objName)
{
   mds_model *m = (mds_model*) IRes_Lock(mh->mhandle);
   mds_mat   *mat_list = md_mat_list(m);
   int  i,j;

   mh->texptr=(IRes **)Malloc(m->mats*sizeof(*(mh->texptr)));
   if(!mh->texptr)
   {
      // malloc failed
      Warning(("MdTextureLoad: could not alloc texture space for model\n"));
      return;
   }

   // cant we just increment mh->numtex while we go??? and get rid of j?
   for (i = 0, j = 0; i < m->mats; ++i)   // register the resources
      if (mat_list[i].type == 0)
         common_texture_load(mh, MDModelTexturesPath, 
                             mat_list[i].name, j++, objName);
   mh->numtex = j;

   IRes_Unlock(mh->mhandle);
}


static void MeshTextureLoad(ModelHandle *mh, char *objName)
{
   mms_model *m = (mms_model*) IRes_Lock(mh->mhandle);
   mms_smatr *mats = mm_smatr_list(m);
   int  i, j;
   uint version = m->version;

    
   mh->texptr = (IRes **)Malloc(m->smatrs*sizeof(*(mh->texptr)));
   if (!mh->texptr)
   {
      // malloc failed
      Warning(("MeshTextureLoad: could not alloc texture space for model\n"));
      return;
   }
    
   if( version == MM_CUR_VER )
   {
       for (i=0, j=0; i<m->smatrs; ++i)   // register the resources
           if (mats[i].type==MM_MAT_TMAP)
               common_texture_load(mh, MeshModelTexturesPath, mats[i].name, j++, objName);
           mh->numtex = j;
   }
   else if( version == MM_ACCLD_VER )
   {
       mms_ssmatr* pOlMat = (mms_ssmatr*) mats;

       for (i=0, j=0; i<m->smatrs; ++i)   // register the resources
       {
           if( pOlMat[i].type == MM_MAT_TMAP )
               common_texture_load(mh, MeshModelTexturesPath, pOlMat[i].name, j++, objName);

           mh->numtex = j; 
       }
   }
   else
       Warning(("MeshTextureLoad: Model with bad version. (Obj #s)\n", objName ));

   IRes_Unlock(mh->mhandle);
}

#define MAX_BITMAP_FRAMES 100

static void BitmapTextureLoad(ModelHandle *mh, char *objName)
{
   IRes *temp_load[MAX_BITMAP_FRAMES];
   int i, sz;

   mh->texptr = temp_load;
   set_silent_load(TRUE);
   for (i=0; i < MAX_BITMAP_FRAMES; ++i)
   {
      char name[256];
      sprintf(name, "%.6s%02d", objName, i);
      common_texture_load(mh, BitmapTexturesPath, 
                          name, i, objName);
      if (temp_load[i] == NULL)
         if (i == 0) // illegal to fail to find first one, but okay for the rest:
            Warning(("Couldn't load bitmap texture %s\n", name));
         else
            break;
   }
   set_silent_load(FALSE);
   mh->numtex = i;
   sz = mh->numtex*sizeof(mh->texptr[0]);
   mh->texptr = (IRes **) Malloc(sz);
   memcpy(mh->texptr, temp_load, sz);
}

// The texture was actually loaded when we loaded the model, to reserve
// a model idx.  Now we're just hooking the handle up to the texture
// pointer.
static void SparkTextureLoad(ModelHandle *mh, char *objName)
{
   mh->texptr = (IRes **) Malloc(sizeof(*(mh->texptr)));
   if (!mh->texptr)
   {
      Warning(("SparkTextureLoad: could not alloc space for pointer\n"));
      return;
   }

   mh->texptr[0] = mh->mhandle;
   mh->numtex = 1;
}

// Some special effect rendering types are rather loopy since they
// don't have resources; this means that we can't test whether they're
// already set up by asking ResName.  We expect that all such
// rendering types have model names of the form FX_cccc.
#define MAX_FX_TYPES 20
typedef struct sFXHandle
{
   short idx;
   short type;          // We copy this into the matching ModelHandle.
   bool set;            // Has this been assigned an idx yet?
   char name[11];       // This doesn't include the FX_ prefix.
} sFXHandle;

static sFXHandle g_aFXHandle[MAX_FX_TYPES]
= {
   { MIDX_INVALID, OM_TYPE_HEAT, FALSE, "Heat" },
   { MIDX_INVALID, OM_TYPE_PARTICLES, FALSE, "Particle" },
};


// This is case-insensitive.
static bool has_prefix(char *candidate, char *prefix)
{
   int prefix_length = strlen(prefix);

   if (!prefix_length)
      return FALSE;

   if (!strnicmp(candidate, prefix, prefix_length))
      return TRUE;
   else
      return FALSE;
}


// This function DOES NOT CHECK whether the given effect is actually
// hooked up to an idx--it simply searches the array for the given
// name.  Note ingenious use of capital letters to make you actually
// read this comment.
static int FindFXHandleByName(char *name)
{
   int i;
   char *name_minus_FX_;

   if (!has_prefix(name, "FX_"))
      return -1;

   name_minus_FX_ = name + strlen("FX_");

   for (i = 0; i < MAX_FX_TYPES; ++i)
      if (has_prefix(name_minus_FX_, g_aFXHandle[i].name))
         return i;

   return -1;
}


// This reserves a ModelHandle for a special effect which has no
// resource.
static void FXSetIdx(int i, int idx)
{
   g_aFXHandle[i].idx = idx;
   g_aFXHandle[i].set = TRUE;

   g_aModelHandles[idx].mhandle = NULL;
   g_aModelHandles[idx].used = TRUE;
   g_aModelHandles[idx].type = g_aFXHandle[i].type;
}


// As with all the FX... special-casery, this is only used for
// rendering types which have no models.
static char *FindFXNameByIdx(int idx)
{
   int i;

   for (i = 0; i < MAX_FX_TYPES; ++i)
      if (g_aFXHandle[i].set
       && g_aFXHandle[i].idx == idx)
         return g_aFXHandle[i].name;

   return NULL;
}


// free textures
static void FreeTextures(ModelHandle *mh)
{
   int i;
   IRes *h;

   for (i = 0; i<mh->numtex; ++i)
   {
      h = mh->texptr[i];

      if (h==NULL) // nothing to free
         continue;
      ectsAnimTxtCheckFree(h);
      if (using_portal_models)
         texmemFreeSingleRes(h, TRUE);  // Is TRUE right here? I think so...
      else
      {
         grs_bitmap *bm=(grs_bitmap *) IRes_Lock(h);
         if (bm->align!=0)
            palmgr_release_slot(bm->align);
         IRes_Unlock(h);     // We need to double-unlock, because we have a lock from
         IRes_Unlock(h);     //    common_textures_load and the lock to get bm right above
         ReleaseAppRes(h);   // try to release, if final release, toast our app data too
      }
   }

   Free(mh->texptr);
   mh->texptr = NULL;
   mh->numtex = 0;
   mh->used = FALSE;
}

// lock textures

// unlock textures

/////// initialization //////////

static void InitModelHandle(ModelHandle *mh)
{
   mh->type = OM_TYPE_DEFAULT;
   mh->mhandle = NULL;
   mh->texptr = NULL;
   mh->numtex = 0;
   mh->refs = 0;
   mh->used = FALSE;
   mh->replace_idx= 0;
}

static BOOL FreeModel(int idx, BOOL drop)
{
   ModelHandle *mh;
   BOOL dropped=drop;
   int i;

   VALIDATE_MH_INDEX(idx, "objmodelFreeModel");

   mh = &g_aModelHandles[idx];

   if (mh->used)
   {  // Are we looking at a real, loaded model?
      if (mh->mhandle != NULL)
      {  // @HACKHACKHACK
         // sparks have an mh->texptr = mh->mhandle
         // but it has only been loaded once, so, here, only free it once...
         // would be cleaner for spark texture load to reload it, i think
         // but im not sure the is a ResNameLoadHandle, really

         if (mh->type!=OM_TYPE_SPARK)
         {
            IRes_Unlock(mh->mhandle);
            if (drop)
               if (!IRes_Drop(mh->mhandle))
                  dropped=FALSE;
            SafeRelease(mh->mhandle);
         }
         FreeTextures(mh);
      }
      else
      {  // Well, then, is it an effect without anything in ResName?
         for (i = 0; i < MAX_FX_TYPES; ++i)
            if ((g_aFXHandle[i].idx == idx) && (g_aFXHandle[i].set))
            {
               g_aFXHandle[i].set = FALSE;
               g_aModelHandles[idx].used = FALSE;
            }
      }
      _clear_replace_idx(mh->replace_idx);
      InitModelHandle(mh);
   }
   return dropped;
}

// Set up the search paths for the various objects and textures
void objmodelSetupRes()
{
   IResMan * pResMan = AppGetObj(IResMan);
   ISearchPath *pDummyTexturesPath;

   // @TBD (justin 8-17-98): This should, quite likely become a Variants
   // path instead. The issue, though, is a subtle one. If we put the
   // textures as, say, variants on MDModelPath, then any of those files
   // will be looked for in any of those directories. (., TXT, or TXT16)
   // Do we want that? Also, be careful -- variants don't show up in
   // canonical paths...
   pDummyTexturesPath = IResMan_NewSearchPath(pResMan, NULL);
   if (allow_16bit_obj_textures)
      ISearchPath_AddPath(pDummyTexturesPath, TXT_PATH_16);
   ISearchPath_AddPath(pDummyTexturesPath, TXT_PATH_8);

   MDModelPath = IResMan_NewSearchPath(pResMan, MD_MODEL_PATH);
   MDModelTexturesPath = ISearchPath_Copy(pDummyTexturesPath);
   ISearchPath_SetContext(MDModelTexturesPath, MDModelPath);

   MeshModelPath = IResMan_NewSearchPath(pResMan, MESH_MODEL_PATH);
   MeshModelTexturesPath = ISearchPath_Copy(pDummyTexturesPath);
   ISearchPath_SetContext(MeshModelTexturesPath, MeshModelPath);

   FXPath = IResMan_NewSearchPath(pResMan, FX_PATH);
   FXTexturesPath = ISearchPath_Copy(pDummyTexturesPath);
   ISearchPath_SetContext(FXTexturesPath, FXPath);

   BitmapPath = IResMan_NewSearchPath(pResMan, BITMAP_PATH);
   BitmapTexturesPath = ISearchPath_Copy(pDummyTexturesPath);
   ISearchPath_SetContext(BitmapTexturesPath, BitmapPath);

   SafeRelease(pDummyTexturesPath);
   SafeRelease(pResMan);
}

/////////////////
// horrible highpoly switch stuff

// or do we just set a global
void objmodelRefreshResPaths(BOOL which)
{
   IResMan * pResMan = AppGetObj(IResMan);

   // get rid of old paths - should i be doing this
   SafeRelease(MDModelPath);
   SafeRelease(MeshModelPath);
   
   if (which)
   {
      MDModelPath = IResMan_NewSearchPath(pResMan, MD_MODEL_HIGH_PATH);
      ISearchPath_AddPath(MDModelPath, MD_MODEL_PATH);
   }
   else
      MDModelPath = IResMan_NewSearchPath(pResMan, MD_MODEL_PATH);
   // ISearchPath_SetContext(MDModelTexturesPath, MDModelPath);

   if (which)
   {
      MeshModelPath = IResMan_NewSearchPath(pResMan, MESH_MODEL_HIGH_PATH);
      ISearchPath_AddPath(MeshModelPath, MESH_MODEL_PATH);
   }
   else
      MeshModelPath = IResMan_NewSearchPath(pResMan, MESH_MODEL_PATH);
   //   ISearchPath_SetContext(MeshModelTexturesPath, MeshModelPath);

   SafeRelease(pResMan);   
}

#define MAX_SWITCH 256
// 
void objmodelSwitchHighPolyDetail(BOOL target_high, BOOL mesh_only)
{
   char *modelNames[MAX_SWITCH];
   int   modelHandles[MAX_SWITCH];
   int   modelRefs[MAX_SWITCH];
   int   i, switch_cnt=0;
   
   objmodelRefreshResPaths(target_high);
   // now go through, look at every model, unload it, reload it with a new path?
   for (i = 0; i < MAX_OBJMODELS; ++i)
      if (g_aModelHandles[i].used)
      {
         ModelHandle *mh=&g_aModelHandles[i];
         if (mh->type==OM_TYPE_MESH || (!mesh_only && mh->type==OM_TYPE_MD))
         {
            char *tmp=(char *)IRes_GetName(mh->mhandle);
            modelHandles[switch_cnt]=i;
            modelNames[switch_cnt]=Malloc(strlen(tmp)+2);
            modelRefs[switch_cnt]=mh->refs;
            strcpy(modelNames[switch_cnt],tmp);
            switch_cnt++;
            FreeModel(i,TRUE);  // need the name, though, to get it back
         }
      }
   for (i=0; i<switch_cnt; i++)
   {
      ModelHandle *mh;
      objmodelLoadModelIntoHandle(modelNames[i],modelHandles[i]);
      mh=&g_aModelHandles[modelHandles[i]];
      mh->refs=modelRefs[i];
      Free(modelNames[i]);
   }
   //   mprintf("Tried to switch %d models\n",switch_cnt);
}

void objmodelDetailSetCommand(BOOL new_mode)
{
   objmodelSwitchHighPolyDetail(new_mode,TRUE);
   config_set_int("character_detail",g_highpoly_characters);
}

//////////////
// horrible thing
#if defined(REPORT) || !defined(SHIP)
void objmodelGetSize(int idx, int *model_mem, int *text_mem)
{
   *model_mem=*text_mem=0;   
   if (g_aModelHandles[idx].used)
   {
      int j, frames=0;
      if (g_aModelHandles[idx].mhandle)
         *model_mem=IRes_GetSize(g_aModelHandles[idx].mhandle);
      // problem is what if other people are using this too
      for (j=0; j<g_aModelHandles[idx].numtex; j++)
         *text_mem+=ectsAnimTxtGetRawSize(g_aModelHandles[idx].texptr[j],&frames);
   }
}

// @TODO: SORT BY SIZE
#define ALLOWED_STRLEN 32768
#define ALLOWED_TMAPS   1024
#define MAX_PALS         256
void objmodelFindUniqueTextures(char **rep_str, BOOL loud)
{
   IRes *used[ALLOWED_TMAPS];  // if we use > 1024, kill me
   int t_mem=0, used_idx=0;
   int i, j, k, used_cnt[ALLOWED_TMAPS];
   int pal_list[MAX_PALS];
   char *buf, *p;

   p=buf=(char *)Malloc(ALLOWED_STRLEN);
   for (i = 0; i < MAX_OBJMODELS; ++i)
      if (g_aModelHandles[i].used)
      {
         for (j=0; j<g_aModelHandles[i].numtex; j++)
         {
            for (k=0; k<used_idx; k++)
               if (used[k]==g_aModelHandles[i].texptr[j])
                  break;
            if (k==used_idx)
            {
               used[used_idx]=g_aModelHandles[i].texptr[j];
               used_cnt[used_idx]=0;
               if (used_idx++>ALLOWED_TMAPS)
                  Warning(("Out of space\n"));
            }
            used_cnt[k]++;
         }
      }
   memset(pal_list,0,sizeof(int)*MAX_PALS);
   // can we sort by size here?
   for (i=0; i<used_idx; i++)
   {
      int frames=0, size=0, pal;
      if (used[i])
      {
         grs_bitmap *bm=IRes_Lock(used[i]);
         size=ectsAnimTxtGetRawSize(used[i],&frames);
         pal=bm->align;
         IRes_Unlock(used[i]);
         pal_list[pal]++;
      }
      t_mem+=size;
      if (loud&&size)
      {
         rsprintf(&p,"Texture %s used %d times, size %d, palette %d",IRes_GetName(used[i]),used_cnt[i],size,pal);
         if (frames>1)
            rsprintf(&p," (%d frames)",frames);
         rsprintf(&p,"\n");
      }
   }
   if (loud)
      for (i=0; i<MAX_PALS; i++)
         if (pal_list[i])
            rsprintf(&p,"%3d use%s for palette %d\n",pal_list[i],pal_list[i]>1?"s":"",i);
   rsprintf(&p,"Real texture memory used is %dK by %d unique textures\n",t_mem/1024,used_idx);
   if (rep_str)
      rsprintf(rep_str,buf);
   else
      mprint(buf);
   Free(buf);
}

void objmodelListMemory(char **rep_str, BOOL loud)
{
   char *buf, *p;
   int free=0, max=0, i;
   int uses[7];
   int mem[2]={0,0}; // model itself, textures for it

   p=buf=(char *)Malloc(ALLOWED_STRLEN+2048);
   for (i = 0; i < MAX_OBJMODELS; ++i)
      if (g_aModelHandles[i].used)
         max++;
      else
         free++;
   rsprintf(&p,"Objmodel using %d of %d model handles, cur max %d real max %d\n",MAX_OBJMODELS-free,MAX_OBJMODELS,max,g_ModelHandleMax);
   memset(uses,0,sizeof(int)*7);
   for (i = 0; i < MAX_OBJMODELS; ++i)
      if (g_aModelHandles[i].used)      
      {
         int md, txt;
         uses[g_aModelHandles[i].type]++;
         objmodelGetSize(i,&md,&txt);
         mem[0]+=md;
         mem[1]+=txt;
      }
   rsprintf(&p,"Using %dKb for models, %dKb for textures\n",mem[0]/1024,mem[1]/1024);
   rsprintf(&p,"By type %d %d %d %d %d %d %d\n",uses[0],uses[1],uses[2],uses[3],uses[4],uses[5],uses[6]);
   objmodelFindUniqueTextures(&p,loud);
   if (rep_str)
      rsprintf(rep_str,buf);
   else
      mprint(buf);
   Free(buf);
}
#endif

void objmodelInit(void)
{
   int i;

   allow_16bit_obj_textures=config_is_defined("ObjTextures16");

   objmodelSetupRes();
   config_get_int("character_detail",&g_highpoly_characters);
   objmodelRefreshResPaths(g_highpoly_characters);

   for (i = 0; i < MAX_OBJMODELS; ++i)
      InitModelHandle(&g_aModelHandles[i]);

   for (i = 0; i < MAX_FX_TYPES; ++i)
      g_aFXHandle[i].set = FALSE;

   g_pDummyObjTmap = texmemBuildCheckerboardTmap(4, 0, FALSE);

   ObjTxtReplaceInit();
   init_replace_list();

#ifdef DBG_ON   // check all model types supported in function table
   AssertMsg2(sizeof(g_afnTextureLoadFuncs) \
            / sizeof(TextureLoadFunc) >= OM_NUM_TYPES,\
              "objmodel.c: %d model types supported, need %d\n",\
              sizeof(g_afnTextureLoadFuncs) \
            / sizeof(TextureLoadFunc),OM_NUM_TYPES);
#endif
}

void objmodelFreePaths()
{
   SafeRelease(MDModelPath);
   SafeRelease(MDModelTexturesPath);
   SafeRelease(MeshModelPath);
   SafeRelease(MeshModelTexturesPath);
   SafeRelease(FXPath);
   SafeRelease(FXTexturesPath);
   SafeRelease(BitmapPath);
   SafeRelease(BitmapTexturesPath);
}

void objmodelShutdown(void)
{
#ifndef SHIP
   if (config_is_defined("objmodel_exit_spew"))
      objmodelListMemory(NULL,FALSE);
#endif   
   
   ObjTxtReplaceTerm();
   
   objmodelFreeAllModels();

   objmodelFreePaths();

   if (g_pDummyObjTmap)
   {
      Free(g_pDummyObjTmap->bits);
      Free(g_pDummyObjTmap);
   }
}

void objmodelReset(void)
{
#ifndef SHIP
   if (config_is_defined("objmodel_reset_spew"))
      objmodelListMemory(NULL,FALSE);
#endif   
   objmodelFreeAllModels();  // try and reduce memory usage a bit
   ObjTxtReplaceDropAll();   // and they should stay in res cache, so hey
}

void objmodelFreeAllModels()
{
   int i;
   for(i = 0; i < MAX_OBJMODELS; ++i)
      FreeModel(i,TRUE);
}

////////// model loading //////////////

static BOOL LoadFXOrModel(char *objName, int idx)
{
   IRes *pRes;
   int i;
   IResMan *pResMan;

   i = FindFXHandleByName(objName);

   if (i != -1) {
      FXSetIdx(i, idx);
      return TRUE;
   }

   pResMan = AppGetObj(IResMan);

   if (has_prefix(objName, "fx_s")) {
      pRes = IResMan_Bind(pResMan,
                          objName + strlen("fx_s_"),
                          RESTYPE_IMAGE,
                          FXPath,
                          NULL, 0);
      if (pRes) {
         g_aModelHandles[idx].type = OM_TYPE_SPARK;
         goto found_model_type;
      }
   }

   pRes = IResMan_Bind(pResMan, objName, RESTYPE_MODEL, MDModelPath, NULL, 0);
   if (pRes) {
      g_aModelHandles[idx].type = OM_TYPE_MD;
      goto found_model_type;
   }

   pRes = IResMan_Bind(pResMan, objName, RESTYPE_IMAGE, BitmapPath, NULL, 0);
   if (pRes) {
      g_aModelHandles[idx].type = OM_TYPE_BITMAP;
      goto found_model_type;
   }

   pRes = IResMan_Bind(pResMan, objName, RESTYPE_MODEL, MeshModelPath, NULL, 0);
   if (pRes) {
      g_aModelHandles[idx].type = OM_TYPE_MESH;
      goto found_model_type;
   }

   Warning(("LoadFXOrModel: couldn't load %s.\n", objName));

   SafeRelease(pResMan);
   return FALSE;

found_model_type:
   SafeRelease(pResMan);

   IRes_SetAppData(pRes, idx);

   g_aModelHandles[idx].mhandle = pRes;
   // Icky; we are putting a permanent lock on the data, so we can access
   // it with DataPeek(). When this code grows up, the callers should instead
   // always do an Unlock(), so we can do away with this nastiness...
   IRes_Lock(pRes);
   g_aModelHandles[idx].used = TRUE;

   // ref count gets inc-ed by model num property manager
   g_afnTextureLoadFuncs[g_aModelHandles[idx].type](&g_aModelHandles[idx],
                                                    objName);

   return TRUE;
}

// load and free an object <-- WHAT DOES THIS MEAN?
//     (i think the old code had LoadModel then freeModel
//      and this comment was for both functions)
int objmodelLoadModel(char *objName)
{
   int idx;

   // Have we already handled this model?
   idx = objmodelGetIdx(objName);
   if (idx != MIDX_INVALID)
      return idx;

   idx = GetFreeModelIndex();

   if (idx == MIDX_INVALID)
      return MIDX_INVALID;

   if (LoadFXOrModel(objName, idx))
      return idx;

   return MIDX_INVALID;
}

// really, you should only call this one if you really know what you are doing
int objmodelLoadModelIntoHandle(char *objName, int idx)
{
   if(g_aModelHandles[idx].used)
   {
      Warning(("objmodelLoadModelIntoHandle: "
               "loading over existing model %d\n", idx));
#ifndef SHIP      
      if (!FreeModel(idx,TRUE))
         mprintf("Hey, loadmodelintohanle %s into %d, couldnt drop old\n",objName,idx);
#else
      FreeModel(idx,TRUE);
#endif      
   }

   if (LoadFXOrModel(objName, idx))
      return idx;
   else
      return MIDX_INVALID;
}

#ifdef EDITOR

// for command system
// XXX should this really be here?
void objmodel_command(char *objName)
{
   char buf[40];
   char *s, *p = NULL;
   int idx;

   s = strrchr(objName,' ');
   if (s!=NULL)
   {
      do { --s; } while (*s==' ');  // get rid of all the spaces
      p=++s;
      *p='\0';  // nullterm objName string, go to start of next
      do { ++s; } while (*s==' ');  // now skip to next thing
      idx=atoi(s);
      if ((idx==0)&&(s[0]!='0'))
      {
         Warning(("Invalid ObjTypeID <%s> after model name\n",s));
         idx=-1;
      }
      else
         idx=objmodelLoadModelIntoHandle(objName,idx);
   }
   else
      idx=objmodelLoadModel(objName);
   if (idx==-1)
      sprintf(buf,"Cannot load obj <%s>",objName);
   else
      sprintf(buf,"Object %s as %d",objName,idx);
   Status(buf);
   if (p!=NULL)
      *p=' ';
}

typedef uchar (*tpixfunc)(int, int, int);

// debugging, for command system
static tpixfunc f_table[3] = \
{
   texmemUGradient,
   texmemVGradient,
   texmemUVGradient
};

// if type < 0, stop using debug textures
void objmodel_set_debug_tex(int type)
{
   extern BOOL g_bAllObjsUnlit; // from objlight.c

   if (type>=0&&type<(sizeof(f_table)/sizeof(*f_table))) {
      if (g_pDummyObjTmap) {
         Free(g_pDummyObjTmap->bits);
         Free(g_pDummyObjTmap);
      }
      g_pDummyObjTmap=texmemBuildGradientTmap(8,f_table[type]);
      g_bTextureDebug=TRUE;
      g_bAllObjsUnlit=TRUE;
   } else {
      g_bTextureDebug=FALSE;
      g_bAllObjsUnlit=FALSE;
   }
}

#endif

/////////// name <-> handle swizzling //////////////////

// Have we already loaded a model with this name?  If so, return the
// idx.
int objmodelGetIdx(char *objName)
{
   IResMan *pResMan;
   IRes *pRes;
   int idx, i;

   // Is this some strange rendering type with no resources?
   i = FindFXHandleByName(objName);
   if (i != -1 && g_aFXHandle[i].set)
      return g_aFXHandle[i].idx;

   // First, try checking whether we already have this resource loaded.
   pResMan = AppGetObj(IResMan);
   pRes = IResMan_Lookup(pResMan, objName, RESTYPE_MODEL, NULL);
   if (!pRes) {
      pRes = IResMan_Lookup(pResMan, objName, RESTYPE_IMAGE, NULL);
   }
   SafeRelease(pResMan);
   if (pRes) {
      idx = IRes_GetAppData(pRes);
      if (g_aModelHandles[idx].used == TRUE
          && g_aModelHandles[idx].mhandle == pRes)
      {
         SafeRelease(pRes);
         return idx;
      }
      SafeRelease(pRes);
   }

   // All right, then, is this some strange rendering type with
   // extra stuff attached to its name?  The constant here is the
   // length of the string "fx_n_".
   if (objName[0] == 'f'
    && objName[1] == 'x'
    && objName[2] == '_'
    && objName[3] != 0
    && objName[4] != 0) 
   {
      pResMan = AppGetObj(IResMan);
      pRes = IResMan_Lookup(pResMan, objName + 5, RESTYPE_IMAGE, NULL);
      SafeRelease(pResMan);
      if (pRes) {
         idx = IRes_GetAppData(pRes);
         if (g_aModelHandles[idx].used == TRUE
             && g_aModelHandles[idx].mhandle == pRes)
         {
            SafeRelease(pRes);
            return idx;
         }
         SafeRelease(pRes);
      }
   }

   return MIDX_INVALID;    
}


char *objmodelGetName(int idx)
{
   IRes *pRes;
   char *name;

   if (!IS_VALID_MH_INDEX(idx)||!g_aModelHandles[idx].used)
      return NULL;

   name = FindFXNameByIdx(idx);

   if (name == NULL) {
      pRes = g_aModelHandles[idx].mhandle;
      name = (char *) IRes_GetName(pRes);
   }

   return name;
}


// get maximum model index
int objmodelMaxModel(void)
{
   return MAX_OBJMODELS;
}

int objmodelGetModelType(int idx)
{
   if (!IS_VALID_MH_INDEX(idx) || !g_aModelHandles[idx].used)
      return OM_TYPE_DEFAULT;
   return g_aModelHandles[idx].type;
}


////////////// rendering setup //////////////////////

// I Totally HATE the fact that we cant unify this between the md and mesh case!!!!!!
// HATE HATE HATE HATE HATE
//   it is _idiotic_ that mesh is an array of structs full of data
//   and md is a set of parallel arrays for each data type
// ACK

// returns a pointer to the model.  user needs to know how to cast it
void *objmodelGetModel(int idx)
{
   if (!IS_VALID_MH_INDEX(idx) || !g_aModelHandles[idx].used)
      return NULL;

   if(g_aModelHandles[idx].mhandle == NULL)
      return NULL;

   // We are keeping the mhandle permanently Locked, so DataPeek works. This
   // is fugly, though; in the long run, callers to objmodelGetModel should
   // also call something like "objmodelReleaseModel"; then this could be a
   // proper Lock, with ReleaseModel doing an Unlock, and we could do away
   // with the permanent Lock...
   return IRes_DataPeek(g_aModelHandles[idx].mhandle);
}

static grs_bitmap *_objmodelGetTexture(int idx, int offset)
{
   IRes *pRes = g_aModelHandles[idx].texptr[offset];
#ifdef EDITOR
   if (!pRes || g_bTextureDebug)
#else
   if (!pRes)
#endif      
      return g_pDummyObjTmap;
   else
      return ectsAnimTxtGetFrame(pRes);
}

// I HATE ALL OF THIS - HATE HATE HATE HATE HATE HATE HATE HATE
#define MAX_TEX_PER_OBJ 64

void objmodelSetupMdTextures(ObjID obj, void *model,int idx)
{
   int rep_idx=g_aModelHandles[idx].replace_idx;
   char hateful_back_mapping[MAX_TEX_PER_OBJ];
   mds_model *m=(mds_model *)model;
   mds_mat   *mat_list;
   int i,j;

   if (!m)
      return;

   mat_list = md_mat_list(m);
   for (i = 0, j = 0; i < m->mats; ++i)   // get the textures in the table
      if (mat_list[i].type==0)
      {
         hateful_back_mapping[j]=mat_list[i].num;  // this is too fucking horrible - dc
         mdd_vtext_tab[mat_list[i].num] = _objmodelGetTexture(idx,j++);
      }
   if (rep_idx)
   {  // @HATEFUL: stay in synch with identical lines below
      IRes **RepTextures=ObjTxtReplaceForObj(obj);
      if (RepTextures)
         for (i=0; i<MAX_REPL_TXT; i++)  // make sure it is replaced, and we have one
            if ((g_aReplaceList[rep_idx][i]!=REPL_LIST_UNUSED)&&RepTextures[i])
               mdd_vtext_tab[hateful_back_mapping[g_aReplaceList[rep_idx][i]]]=IRes_DataPeek(RepTextures[i]);
   }            
}




void objmodelSetupMeshTextures(ObjID obj, void *model, int idx)
{
   int rep_idx=g_aModelHandles[idx].replace_idx;
   char hateful_back_mapping[MAX_TEX_PER_OBJ];
   mms_model *m = (mms_model *)model;
   mms_smatr *mats;
   int i, j;
   uint version;
   
   if (!m)
      return;

   version = m->version;
   mats    = mm_smatr_list(m);
   
   if( version == MM_CUR_VER )//zb
   {
       for (i = 0, j = 0; i < m->smatrs; ++i)   // get the textures in the table
           if (mats[i].type == MM_MAT_TMAP)
           {
               hateful_back_mapping[j]=i;  // this is too fucking horrible - dc // he, he you love it        
               mats[i].handle=(ulong)_objmodelGetTexture(idx,j++);
           }
           if (rep_idx)
           {  // @HATEFUL: stay in synch with identical lines below      
               IRes **RepTextures=ObjTxtReplaceForObj(obj);
               if (RepTextures)
                   for (i=0; i<MAX_REPL_TXT; i++)  // make sure it is replaced, and we have one
                       if ((g_aReplaceList[rep_idx][i]!=REPL_LIST_UNUSED)&&RepTextures[i])
                           mats[hateful_back_mapping[g_aReplaceList[rep_idx][i]]].handle=(ulong)IRes_DataPeek(RepTextures[i]);
           }  
           
   }
   else if( version == MM_ACCLD_VER )//zb
   {
       mms_ssmatr* pOlMat = (mms_ssmatr*) mats;

       for( i = 0, j = 0; i < m->smatrs; ++i )
       {
           if( pOlMat[i].type == MM_MAT_TMAP )
           {
               hateful_back_mapping[j] = i;  
               pOlMat[i].handle        = (ulong)_objmodelGetTexture( idx, j++ );
           }
       }
       
       if (rep_idx)
       {  
           IRes**   RepTextures = ObjTxtReplaceForObj( obj );
           if( RepTextures )
           {
               for( i = 0; i < MAX_REPL_TXT; i++ ) 
               {
                   if( (g_aReplaceList[rep_idx][i] != REPL_LIST_UNUSED ) && RepTextures[i] )
                       mats[ hateful_back_mapping[ g_aReplaceList[rep_idx][i] ] ].handle 
                       = (ulong) IRes_DataPeek( RepTextures[i] );
               }
           }
       }  
       
   }
   
}




//////////////////
// special bitmap texture support

r3s_texture objmodelSetupBitmapTexture(int idx, int frame)
{
   r3s_texture tex;
   IRes *pRes;

   pRes = g_aModelHandles[idx].texptr[frame];
   tex = (r3s_texture) IRes_Lock(pRes);

   return tex;
}

void objmodelReleaseBitmapTexture(int idx, int frame)
{
   IRes *pRes = g_aModelHandles[idx].texptr[frame];
   IRes_Unlock(pRes);
}

int objmodelBitmapNumFrames(int idx)
{
   return g_aModelHandles[idx].numtex;
}
