/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/editsave.c,v 1.35 2000/02/19 13:10:37 toml Exp $
// editorlevel file format and save/load code

// the editor level part .COW file contains the following tags and info
//  Tag: Brushes       (BRLIST)
//    contains all actual brushes in the level, w/o any interpretation data
//  Tag: Obj Models    (OBJ MD)
//    contains a header with count info, then names of objects
//  Tag: Brush Header  (BRHEAD)
//    contains various info on username, skies used, and so on
//  Tag: Flow Texture Indices  (FLOW_TEX)
//    RGB and entry and exit texture indices for each flow group

#include <lg.h>
#include <mprintf.h>
#include <timer.h>

#include <lresdisk.h>

#include <tagfile.h>
#include <vernum.h>

#include <family.h>
#include <csg.h>

#include <editbr.h>
#include <editbr_.h>
#include <brinfo.h>
#include <brrend.h>
#include <brlist.h>
#include <saveload.h>
#include <vbrush.h>
#include <gridsnap.h>
#include <gedit.h>
#include <edmedmo.h>
#include <areapnp.h>
#include <status.h>
#include <texprop.h>

#include <appagg.h>
#include <iobjsys.h>

#include <objtype.h>
#include <objdef.h>
#include <objnotif.h>
#include <objpos.h>
#include <objquery.h>
#include <editobj.h>
#include <dbfile.h>

#include <sdesc.h>
#include <sdesbase.h>
#include <propface.h>
#include <property.h>
#include <propbase.h>
#include <missrend.h>

//#include <propfac_.h>
//#include <property.h>
//#include <dbtagfil.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// controls whether we load texture data or not
bool _editor_loadNoTextures=FALSE;

// holds current tagfile for use in all single element callbacks
// thus used by brushes, objmodels
static ITagFile  *_brTagFile=NULL;

IBoolProperty *gFromBriefcaseProp;

/////////////////////////
// Brushes:
static TagVersion BrushVer={1,0};
static TagFileTag BrushTag={"BRLIST"};

// SAVE
// callback for brush save
static bool _saveSingleBrush(editBrush *us, int sz)
{
   return ITagFile_Write(_brTagFile,(char*)us,sz) == sz;
}

// saves all brushes out, by setting curlocaltagfile then calling saveload sys
static BOOL _saveAllBrushes(ITagFile *file, int groupFilter)
{
   HRESULT result=ITagFile_OpenBlock(file,&BrushTag,&BrushVer);
   BOOL rv=FALSE;
   if (result==S_OK)
   {
      _brTagFile=file;
      rv=saveloadWriteBrushes(_saveSingleBrush,groupFilter);
      ITagFile_CloseBlock(_brTagFile);
      _brTagFile=NULL;
   }
   return rv;
}

// LOAD
static loadBrushNewGroup=0;   // another hack for save load mini brush list

// callback for brush load, fills in newbr or returns FALSE if it cant
static bool _loadSingleBrush(editBrush *newbr)
{
   int base_sz = sizeof(editBrush) - EDITBR_MAX_FACES * sizeof(TexInfo);
   
   if (ITagFile_Read(_brTagFile,(char *)newbr,base_sz)==base_sz)
   {
      if (brushGetType(newbr)==brType_TERRAIN)
      {
         int xtra_sz=sizeof(TexInfo)*newbr->num_faces;
         if (ITagFile_Read(_brTagFile,(char *)&newbr->txs,xtra_sz)!=xtra_sz)
            return FALSE;
      }
      if (loadBrushNewGroup)
         brSelect_Group(newbr)=loadBrushNewGroup;
      return TRUE;
   }
   else
      return FALSE;
}

static BOOL _loadAllBrushes(ITagFile *file, BOOL preserve_id)
{
   TagVersion LocalVer=BrushVer;
   HRESULT result=ITagFile_OpenBlock(file,&BrushTag,&LocalVer);
   BOOL rv=FALSE;

   if (result==S_OK)
   {
      _brTagFile=file;
      rv=saveloadReadBrushes(_loadSingleBrush,SAVELOAD_NO_COUNT,preserve_id);
      ITagFile_CloseBlock(_brTagFile);
      _brTagFile=NULL;
   }
   return rv;
}

/////////////////////////
// multibrush-objs

int vbr_filter_group;
static BOOL LGAPI vbr_partition_func(ObjID obj, const sObjPartitionFilter *f )
{
   // okay, do we have an editor brush that corresponds to us as an object?
   editBrush *us;
   
   us = editObjGetBrushFromObj(obj);

   // if so, does it match our group?
   if (us != NULL)
   {
      if (brSelect_Group(us) == vbr_filter_group)
      {
         return(TRUE);
      }
   }
   return(FALSE);
}

extern edbFiletype dbSaveTagFile(ITagFile* file, edbFiletype filetype);
extern edbFiletype dbMergeLoadTagFile(ITagFile* file, edbFiletype filetype);

static BOOL _saveAllObjs(ITagFile *file, int group)
{
   //ObjPos *opos;
   IObjectQuery* query;
   IObjectSystem* pObjSys = AppGetObj(IObjectSystem); 
   ObjID obj;

   // register the filter that describes the "briefcase" sub-partition 
   sObjPartitionFilter filter = { vbr_partition_func}; 
   IObjectSystem_SetSubPartitionFilter(pObjSys,kObjPartBriefcase,&filter); 
   vbr_filter_group = group;

   query = IObjectSystem_Iter(pObjSys,kObjectConcrete); 
   for (; !IObjectQuery_Done(query); IObjectQuery_Next(query))
   {
      obj = IObjectQuery_Object(query);
      if (vbr_partition_func(obj, NULL))
      {
         /*
         opos = ObjPosGet(obj);
         UpdateChangedLocation(&opos->loc);
         */

         // flag it as being in the briefcase so that we can go 
         // know that later when we retrieve from briefcase
         PROPERTY_SET(gFromBriefcaseProp, obj, TRUE);
      }
   }

   // save the "briefcase" sub-partition 
   dbSaveTagFile(file,kObjPartConcrete|kObjPartBriefcase); 

   query = IObjectSystem_Iter(pObjSys,kObjectConcrete); 
   for (; !IObjectQuery_Done(query); IObjectQuery_Next(query))
   {
      obj = IObjectQuery_Object(query);
      if (vbr_partition_func(obj, NULL))
      {
         IProperty_Delete(gFromBriefcaseProp, obj);
      }
   }
   return TRUE;
}

static BOOL _loadAllObjs(ITagFile *file)
{
   IObjectQuery* query;
   IObjectSystem* pObjSys = AppGetObj(IObjectSystem); 
   ObjID obj;
   editBrush *newbr;
   BOOL val;

   dbMergeLoadTagFile(file,kObjPartConcrete|kObjPartBriefcase); 

   query = IObjectSystem_Iter(pObjSys,kObjectConcrete); 

   for (; !IObjectQuery_Done(query); IObjectQuery_Next(query))
   {
      obj = IObjectQuery_Object(query);
      newbr = editObjGetBrushFromObj(obj);
      if (IProperty_IsRelevant(gFromBriefcaseProp, obj))
      {
         PROPERTY_GET(gFromBriefcaseProp, obj, &val);

         if (val)
         {
            if (loadBrushNewGroup)
               brSelect_Group(newbr)=loadBrushNewGroup;
         }
         IProperty_Delete(gFromBriefcaseProp, obj);
      }
   }
   return TRUE;
}

/////////////////////////
// generic block header stuff for lists of same size records

// put at the top of each string block
typedef struct {
   int size_per;
   int elem_cnt;
} blockHeader;

// SAVE
static BOOL _saveGenericBlocked(ITagFile *file, blockHeader *hdr, void *mem, TagFileTag *tag, TagVersion *ver)
{
   int write_sz;
   BOOL rv=FALSE;
   
   if (ITagFile_OpenBlock(file,tag,ver)!=S_OK) return FALSE;
   write_sz=hdr->elem_cnt*hdr->size_per;
   if (ITagFile_Write(file,(char*)hdr,sizeof(blockHeader))==sizeof(blockHeader))
      if (ITagFile_Write(file,mem,write_sz)==write_sz)
         rv=TRUE;
   ITagFile_CloseBlock(file);
   return rv;
}

// LOAD
static void *_loadGenericBlocked(ITagFile *file, TagFileTag *tag, TagVersion *ver, TagVersion *newver, blockHeader *hdr)
{
   void *mem=NULL;

   if (ITagFile_OpenBlock(file,tag,newver)!=S_OK) return NULL;
   if (ITagFile_Read(file,(char*)hdr,sizeof(blockHeader))==sizeof(blockHeader))
   {
      int read_sz=hdr->elem_cnt*hdr->size_per;
      mem=Malloc(read_sz);
      if (mem!=NULL)
         if (ITagFile_Read(file,mem,read_sz)!=read_sz)
         {
            Free(mem);    // we are gonna return failure
            mem=NULL;     // but lets free our memory block first
         }
   }
   ITagFile_CloseBlock(file);
   return mem;
}

/////////////////////////
// Family List:
static TagVersion FamilyVer={1,0};
static TagFileTag FamilyTag={"FAMILY"};

// SAVE
static BOOL _saveAllFamilies(ITagFile *file)
{
   blockHeader theHdr;
   void *mem=family_name_block_build(&theHdr.elem_cnt,&theHdr.size_per);
   if (mem)
   {
      BOOL rv=_saveGenericBlocked(file,&theHdr,mem,&FamilyTag,&FamilyVer);
      Free(mem);  // allocated by block build
      return rv;
   }
   return FALSE;
}

// LOAD
static BOOL _loadAllFamilies(ITagFile *file)
{
   TagVersion LocalVer=FamilyVer;
   blockHeader theHdr;
   void *mem=NULL;

   mem=_loadGenericBlocked(file,&FamilyTag,&FamilyVer,&LocalVer,&theHdr);
   if (mem!=NULL)
   {
      BOOL rv=family_name_block_parse(theHdr.elem_cnt,theHdr.size_per,mem);
      Free(mem);
      return rv;
   }
   return FALSE;
}

/////////////////////////
// Textures:
static TagVersion TextureVer={1,0};
static TagFileTag TextureTag={"TXLIST"};

// SAVE
static BOOL _saveAllTextures(ITagFile *file)
{
   void *mem=familyDiskTexBlockBuild();
   if (mem!=NULL)
   {
      BOOL rv;
      int size=ResBlockSize(mem);
      if (ITagFile_OpenBlock(file,&TextureTag,&TextureVer)!=S_OK) return FALSE;
      rv=(ITagFile_Write(file,(char*)mem,size)==size);
      ITagFile_CloseBlock(file);
      Free(mem);
      return rv;
   }
   return FALSE;
}

// LOAD
static BOOL _loadAllTextures(ITagFile *file)
{
   TagVersion LocalVer=TextureVer;
   BOOL rv=FALSE;
   void *mem;
   int size;
   
   if (ITagFile_OpenBlock(file,&TextureTag,&LocalVer)!=S_OK) return FALSE;
   size=ITagFile_BlockSize(file,ITagFile_CurrentBlock(file));
   mem=(void *)Malloc(size);
   rv=(ITagFile_Read(file,(char *)mem,size)==size);
   Assrt(size==ResBlockSize(mem));
   ITagFile_CloseBlock(file);
   if (rv)           // successfully read the data, now lets try to load
      rv=familyDiskTexBlockLoad(mem);  // the resnameblock in for real...
   Free(mem);  // since we malloced it and dont need it any more
   return rv;
}

/////////////////////////
// Header:

typedef struct {
   levelNameInfo names;                // nice-o name 
   int   ambient;                      // the rest are editor state fields
   int   grid_pow2;
} old_brHeader;

typedef struct {
   levelNameInfo names;                // nice-o name 
   int   ambient;                      // the rest are editor state fields
   int   grid_pow2;
   int   brush_pos;
   int   color_state;
   int   vBrush_info;
   bool  use_grid;
   bool  hots_state;
   short pad;
   int   edit_time;
} brHeader;

static TagVersion HeaderVer={9,sizeof(brHeader)};
static TagFileTag HeaderTag={"BRHEAD"};

// for timing silliness - ie. length of time level has been being run
static int last_edit_time=0, cur_total_time=0;

static brHeader theHdr;

// for now, should really fix at some point, since it is such a stupid hack
extern void grid_pow2_scale(int pow2);
extern int  get_pow2_scale(void);

#define ms_to_sec(ms_val) ((ms_val/1000)%60)
#define ms_to_min(ms_val) (((ms_val/1000)/60)%60)
#define ms_to_hr(ms_val)  ((((ms_val/1000)/60)/60)%24)
#define ms_to_day(ms_val)  ((((ms_val/1000)/60)/60)/24)

void _editsave_text_info(char *buf)
{
   int cur_time=(last_edit_time)?tm_get_millisec_unrecorded()-last_edit_time:0;
   int ms_cur;

   strcat(buf,"\nCreated by ");
   strcat(buf,theHdr.names.creator);
   strcat(buf," last save ");
   strcat(buf,theHdr.names.user);
   strcat(buf,"\n Total time ");
   ms_cur=cur_total_time+cur_time;
   sprintf(buf+strlen(buf),"%d:%2.2d:%2.2d:%2.2d",ms_to_day(ms_cur),ms_to_hr(ms_cur),ms_to_min(ms_cur),ms_to_sec(ms_cur));
   strcat(buf," this session ");
   ms_cur=cur_time;
   sprintf(buf+strlen(buf),"%d:%2.2d:%2.2d:%2.2d",ms_to_day(ms_cur),ms_to_hr(ms_cur),ms_to_min(ms_cur),ms_to_sec(ms_cur));   
}

// SAVE
static BOOL _saveBrushHeader(ITagFile *file)
{
   int delta_time=last_edit_time?tm_get_millisec_unrecorded()-last_edit_time:0;
   BOOL rv;

   if (ITagFile_OpenBlock(file,&HeaderTag,&HeaderVer)!=S_OK) return FALSE;
   strcpy(saveloadLevelInfo.user,saveloadGetUser());
   if (saveloadLevelInfo.creator[0]==0)  // no creator string, so it must be us
      strcpy(saveloadLevelInfo.creator,saveloadGetUser());
   memcpy(&theHdr.names,&saveloadLevelInfo,sizeof(levelNameInfo));
   theHdr.grid_pow2=get_pow2_scale();
   theHdr.vBrush_info=vBrushGetSaveData();
   theHdr.brush_pos=blistSeek(0,SEEK_CUR);
   theHdr.hots_state=renderHotsOnly;
   theHdr.color_state=editbr_color_mode;
   theHdr.use_grid=cur_global_grid.grid_enabled;

   cur_total_time+=delta_time;      // timing hacks
   theHdr.edit_time=cur_total_time;
//   mprintf("Total time now %d from %d\n",theHdr.edit_time,delta_time);
   last_edit_time=tm_get_millisec_unrecorded();
   
   rv=(ITagFile_Write(file,(char *)&theHdr,sizeof(brHeader))==sizeof(brHeader));
   ITagFile_CloseBlock(file);
   return rv;
}

// LOAD
static BOOL _loadBrushHeader(ITagFile *file)
{
   TagVersion LocalVer=HeaderVer;
   BOOL rv=FALSE;                   // hack as i didnt start with the
   int hdrSz=sizeof(old_brHeader);  //    minor version is size thing till version 2

   if (ITagFile_OpenBlock(file,&HeaderTag,&LocalVer)!=S_OK) return FALSE;
   if (LocalVer.major>1) hdrSz=LocalVer.minor;
   memset(&theHdr,0,sizeof(brHeader));
   if (ITagFile_Read(file,(char *)&theHdr,hdrSz)>=sizeof(old_brHeader))
   {
      memcpy(&saveloadLevelInfo,&theHdr.names,sizeof(levelNameInfo));
      grid_pow2_scale(theHdr.grid_pow2);
      // Cleanup from old versions of the header to brush list
      if (LocalVer.major>1)
      {
         cur_global_grid.grid_enabled=theHdr.use_grid;
         editbr_color_mode=theHdr.color_state;
      }
      else
      {
         saveloadZeroAllGroups();
         saveloadCleanVer1File();
      }
      if (LocalVer.major<3)
      {
         saveloadFixAlignmentFields();
         theHdr.edit_time=0;
      }
      if (LocalVer.major<4)
         saveloadFixIntPrimalIDs();
      if (LocalVer.major<5)
         saveloadFixNonTerrPrimalIDs();
      if (LocalVer.major<6)
         AreaPnP_Names_Fixup();
      if (LocalVer.major<8)
         saveloadFixFlagField();
      if (LocalVer.major<9)
      {
         sMissionRenderParams parms;
         parms = *GetMissionRenderParams();
         parms.ambient_light.x = theHdr.ambient/256.0;
         parms.ambient_light.y = theHdr.ambient/256.0;
         parms.ambient_light.z = theHdr.ambient/256.0;
         SetMissionRenderParams(&parms);
      }

      vBrushParseLoadData(theHdr.vBrush_info);
      blistSeek(theHdr.brush_pos,SEEK_SET);
      renderHotsOnly=theHdr.hots_state;
      rv=TRUE;
   }
   vBrush_SelectBrush(blistGet());
   ITagFile_CloseBlock(file);
   last_edit_time=tm_get_millisec_unrecorded();
   cur_total_time=theHdr.edit_time;
   return rv;
}

// for minibrushes (i.e. multibrush files)

typedef struct {
   int   flags;
} miniHeader;

static TagVersion MiniHeaderVer={1,sizeof(miniHeader)};
static TagFileTag MiniHeaderTag={"MBHEAD"};

static BOOL _saveMiniBrushHeader(ITagFile *file)
{
   static miniHeader mHeader;
   BOOL rv;

   if (ITagFile_OpenBlock(file,&MiniHeaderTag,&MiniHeaderVer)!=S_OK) return FALSE;
   mHeader.flags=0;
   rv=(ITagFile_Write(file,(char *)&mHeader,sizeof(miniHeader))==sizeof(miniHeader));
   ITagFile_CloseBlock(file);
   return rv;
}

static BOOL _loadMiniBrushHeader(ITagFile *file)
{
   static miniHeader mHeader;
   TagVersion LocalVer=MiniHeaderVer;
   BOOL rv=FALSE;                  
   int hdrSz;

   memset(&mHeader,0,sizeof(mHeader));
   if (ITagFile_OpenBlock(file,&MiniHeaderTag,&LocalVer)!=S_OK)
      saveloadFixFlagField();  // non-existant
   else
   {
      hdrSz=LocalVer.minor;
      if (ITagFile_Read(file,(char *)&mHeader,hdrSz)>=sizeof(miniHeader))
         rv=TRUE;   // it worked
   }

   // parse any flags or anything

   ITagFile_CloseBlock(file);
   return rv;
}

/////////////////////////
// Flow Texture Indices:
static TagVersion FlowTexVersion = {1, 3};
static TagFileTag FlowTexTag = {"FLOW_TEX"};

static BOOL _saveFlowTextureIndices(ITagFile *file)
{
   HRESULT result = ITagFile_OpenBlock(file, &FlowTexTag, &FlowTexVersion);
   BOOL rv = FALSE;

   if (result == S_OK) {

      ITagFile_Write(file, (char *) &g_aEdMedMoSurface[0],
                     sizeof(g_aEdMedMoSurface[0]) * MAX_CELL_MOTION);
      ITagFile_CloseBlock(file);

   }
   return rv;
}

static BOOL _loadFlowTextureIndices(ITagFile *file)
{
   TagVersion found_version = FlowTexVersion;
   HRESULT result = ITagFile_OpenBlock(file, &FlowTexTag, &found_version);
   BOOL rv = FALSE;

   if (result == S_OK
    && found_version.major == FlowTexVersion.major
    && found_version.minor == FlowTexVersion.minor) {

      ITagFile_Read(file, (char *) &g_aEdMedMoSurface[0],
                    sizeof(g_aEdMedMoSurface[0]) * MAX_CELL_MOTION);
      ITagFile_CloseBlock(file);

   }
   return rv;
}


////////////////////////
// MAIN STUFF
// these are the actual calls made by the outside world

BOOL editor_SaveCow(ITagFile *file)
{
   BOOL rv = TRUE;
   rv |= AreaPnP_Names_Save(file);
   rv |= vBrush_NameSave(file);
   rv |= _saveAllBrushes(file, 0);
   rv |= _saveBrushHeader(file);
   rv |= _saveFlowTextureIndices(file);
   if (!rv)
      Warning(("Some component of the COW file saved incorrectly"));
   return rv;
}

BOOL editor_LoadCow(ITagFile *file)
{
   BOOL rv = TRUE;
   rv |= AreaPnP_Names_Load(file);
   rv |= vBrush_NameLoad(file);
   rv |= _loadAllBrushes(file, TRUE);
   rv |= _loadBrushHeader(file);
   rv |= _loadFlowTextureIndices(file);
   if (!rv)
      Warning(("Some component of the COW file loaded incorrectly"));
   return rv;
}

BOOL editor_PostLoadCow(void)
{
   saveloadPostLoadBrushes();
   return TRUE; 
}

BOOL editor_SaveMiniBrush(char *fname, int group)
{
   ITagFile *file;
   BOOL rv=TRUE;

   if ((file=TagFileOpen(fname,kTagOpenWrite))==NULL) return FALSE;
   rv|=_saveAllBrushes(file,group);
   rv|=_saveMiniBrushHeader(file);
   // now save out any objects as a briefcase within the file
   rv|=_saveAllObjs(file,group);
   SafeRelease(file);   
   return rv;
}

BOOL editor_LoadMiniBrush(char *fname, int new_group)
{
   ITagFile *file;   
   BOOL rv=TRUE;

   if ((file=TagFileOpen(fname,kTagOpenRead))==NULL) return FALSE;
   loadBrushNewGroup=new_group;
   rv|=_loadAllBrushes(file,FALSE);
   rv|=_loadMiniBrushHeader(file);
   rv|=_loadAllObjs(file);
   loadBrushNewGroup=0;
   SafeRelease(file);
   return rv;
}

static sPropertyDesc FromBriefcaseDesc =
{
   "FromBriefcase", 0, 
   NULL, 0, 0, // constraints, versions
   { "Engine Features", "From Briefcase?" },
};

void VbrushSaveLoadInit()
{
   gFromBriefcaseProp = CreateBoolProperty(&FromBriefcaseDesc, kPropertyImplDense);
}
