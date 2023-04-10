/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/family.c,v 1.68 2000/02/14 20:35:19 bfarquha Exp $
// hack for family loading/palette switch until real dataflow

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <lg.h>
#include <2d.h>
#include <mprintf.h>
#include <config.h>
#include <appagg.h>

#include <portal.h>
#include <wrdbrend.h>

#include <command.h>
#include <status.h>

#include <storeapi.h>
#include <resapi.h>
#include <palrstyp.h>
#include <lresdisk.h>

#include <family.h>
#include <family_.h>
#include <famedit.h>
#include <filetool.h>
#include <palette.h>
#include <mipmap.h>
#include <texmem.h>
#include <palmgr.h>
#include <brushgfh.h>
#include <textarch.h>

#include <scrnmode.h>
#include <gamescrn.h>

#include <dbmem.h>

/////////////////////////
// horrifying horror

#define kDirWater 0
#define kDirSky   1
#define kDirSW    0
#define kDirHW    1

static int cur_render_type=kDirSW;

static char *_fam_dir_names[2][2]=
{
  { "fam\\water\\", "fam\\waterhw\\" },
  { "fam\\sky\\",   "fam\\skyhw\\" },
};

#define get_water_base() _fam_dir_names[kDirWater][cur_render_type]
#define get_sky_base()   _fam_dir_names[kDirSky][cur_render_type]

/////////////////////////
// utility functions

// remap a single cell texture through the swap table
static BOOL cell_tx_id_remap(uchar *cur_val, int *tex_swap, int cnt)
{
   int val=(int)*cur_val;
   int newval=0;
   if ((val>=0)&&(val<cnt))                 // protect against mahks broken cosmic ray level
      newval=tex_swap[val];
   else if ((val>=WATERIN_IDX)&&(val<=255)) // hateful initial check to save sky_tex
      return FALSE;                         // "special"'s just stay themselves
   if (newval==-1) newval=0;
   if (newval!=val)
   {
      *cur_val=(uchar)newval;
      return TRUE;
   }
   return FALSE;
}

// remap all portal cells
static BOOL _familyRemapCells(int *tex_swap, int cnt)
{
   PortalCell *p;
   int ci, i;
   BOOL chg=FALSE;

   for (ci=0; ci<wr_num_cells; ci++)
      if ((p=wr_cell[ci])!=NULL)
         for (i=0; i<p->num_render_polys; i++)
            chg|=cell_tx_id_remap(&p->render_list[i].texture_id,tex_swap,cnt);
   return chg;
}

// remap all brushes based on a list of RESNAME handles! not texmem handles
// ie. given the resname mapping from resnameParseBlock, find the texmem handles
//   and perform that mapping to all brushes in memory
BOOL _familyRemapFromResName(IRes **tex_rnhnd, int cnt)
{
   int handles[TEXMEM_MAX];

   texmemGetHandlesFromRes(tex_rnhnd, handles, cnt);

   if (_familyRemapBrushes(handles, cnt))
      if (_familyRemapCells(handles, cnt))
         return TRUE;

   return FALSE;
}

extern bool level_valid;

/////////////////////
// disk block interface to the family system

// actually go create the disk writeable block
void *familyDiskTexBlockBuild(void)
{
   return BuildResBlock(texmem_hnd, texmem_cur_max);
}

// for water hack hell - return FALSE to say "skip this!!"
// @HACK: @TODO: @ARGH: OH MY GOD this is HIDEOUS
// it scans a name from a disk block
// realizes that is a "water" or "waterhw" or "sky" or "skyhw" sort of thing
// then hacks the name so that it maps to the right sort of thing
//    (hw or sw) for your current setup
// HATE HATE HATE
BOOL fam_name_examine_dir(char *name, int dir)
{
   char tmp_name[256], *p, *s;
   int j;

   //   mprintf("Examining '%s'\n",name);
   p=strrchr(name,'\\');
   if (!p) return FALSE; // got me, just let it go
   // this is really really horrifying
   for (j=0; j<2; j++)
   {
      int len=strlen(_fam_dir_names[dir][j]);
      s=p-len+1;
      if (strnicmp(_fam_dir_names[dir][j],s,len)==0)
      {
         strcpy(tmp_name,_fam_dir_names[dir][cur_render_type]);
         strcat(tmp_name,p+1);
         strcpy(name,tmp_name);
         return TRUE;
      }  // KILL ME NOW
   }
   return FALSE;
}

BOOL fam_name_examine(char *name)
{
   return fam_name_examine_dir(name,kDirWater) || fam_name_examine_dir(name,kDirSky);
}

// The actual call we pass to texblockparse.  always returns true.
// I don't know why.  - MAHK
BOOL fam_name_cb(char* name)
{
   // replace front slashes with back slashes
   char* s;
   for (s = name; *s != '\0'; s++)
      if (*s == '/')
         *s = '\\';
   fam_name_examine(name);
   return TRUE;
}

static BOOL no_remap_on_load = FALSE;

// messy integrated load for use with lresdisk block swizzle stuff
BOOL familyDiskTexBlockLoad(void *mem)
{
   IRes *handles[TEXMEM_MAX];
   BOOL rv, old_find_align=texmem_find_align;

   if (TEXMEM_MAX<=ResBlockHndCnt(mem))
      return FALSE;

   if (no_remap_on_load)
      return FALSE;

   texmem_callback_idx=0;             texmem_find_align=TRUE;
   rv = ParseResBlock(mem, handles, texmem_interpret_new_res, fam_name_cb);
   texmem_callback_idx=TEXMEM_NO_HND; texmem_find_align=old_find_align;

   if (rv)
   {
      int i;
      rv = _familyRemapFromResName(handles,ResBlockHndCnt(mem));
      for (i = 0; i < ResBlockHndCnt(mem); i++)
         if (handles[i])
            SafeRelease(handles[i]);
   }
   return rv;  // lets return in both the true and false cases, now, not just one
}

//////////////////////////////////////
// new family stuff

// we keep a list of each family loaded, and its load_id
// then we have the following "family primitives"

// load_family <famname>
//  - for backwards compatibility, clears other families, loads this one
// add_family <famname>
//  - adds all textures in this family to the in memory texture list
//    if this is done for an existing family it reloads it and remaps
// remove_family <famname|all>
//  - with a name goes and removes all textures from that family
//    with "all" punts all textures in memory and resets to no families
// load_water <prefixnm>
//  - the "new" way for water is to load the water on its own
//    all water textures should go in "fam/water/<prefixnm>[in/out].pcx"
//    so assuming there is "sewerin" and "sewerout", "pondin", "pondout",
//      and so on, you just do "load_water pond" to make that your water
// load_a_texture <FamName> <TexName>
//  - given a texture (with the family name) load it as a single slot
// compress_family <famname|all>
//  - gets rid of all unused textures from a given family, or all families
//
// clearly this cries out for a visual tool of some sort, but oh well

// @TODO: test reload/readd same family code, fam_synch timing in that case
// @TODO: counts on texmemLoadDir - counts are critical, real fails on load???

extern void clear_surface_cache(void);

char       _fam_water_name[FAM_NAME_LEN];
char       _fam_sky_name[FAM_NAME_LEN];

fam_record cur_fams[MAX_FAMILIES];
int        num_fams;

IRes *     _fam_pal_hnd=NULL;

// @TODO: this is named wrong, or something, or the code right below in synch is bogus
static bool       _fam_no_compress=FALSE;

//////////////////
// internal utilities

// the idea is that in _fam_sync, at the end of any successful texture
//   operation we recompress, clear surface cache, then remap brushes/cells
void _fam_synch(BOOL remap)
{
   if (remap&&(!_fam_no_compress))
   {  // compress out the empties
      int handle_map[TEXMEM_MAX];
      texmemCompressHandles(handle_map);
      _familyRemapBrushes(handle_map,TEXMEM_MAX);
      _familyRemapCells(handle_map,TEXMEM_MAX);
   }
   clear_surface_cache();
   // refresh editor gadget
   // @TODO: replace this with some anonymous control flow

#ifdef EDITOR
   GFHRedrawPnPnSuch();
#endif
   RebuildTextureArchetypes();
}

static void _fam_palette_free(void)
{
   if (_fam_pal_hnd)
   {
      IRes_Unlock(_fam_pal_hnd);
      SafeRelease(_fam_pal_hnd);
   }
}

// check to make sure this palette matches
static BOOL _fam_palette_check(char *dir)
{
   BOOL rv=FALSE;
   IRes *loc_hnd = NULL;
   IResMan *pResMan;

   if (config_is_defined("TerrTextures16"))
      return TRUE;

   pResMan = AppGetObj(IResMan);
   // The palette is the file "full" in the given directory:
   loc_hnd = IResMan_Bind(pResMan, "full", RESTYPE_PALETTE, NULL, dir, 0);
   SafeRelease(pResMan);
   if (!loc_hnd)
   {
      return FALSE;
   }

   rv=(memcmp(IRes_Lock(loc_hnd),palmgr_get_pal(0),256*3)==0);

   IRes_Unlock(loc_hnd);
   SafeRelease(loc_hnd);
   return rv;
}

// clear out our internal data structure
// should clear sky and water as well
void _fam_clear_records(void)
{
   int i;
   for (i=0; i<MAX_FAMILIES; i++)
      cur_fams[i].load_id=TEXMEM_LOADID_NULL;
}

//  punt all textures
void _fam_remove_all(void)
{
   // must deal with sky and water and such!
   texmemClearAll();
   _fam_clear_records();
   _fam_palette_free();
   _fam_synch(FALSE);
}

// find a free record
static fam_record *_fam_get_free_record()
{
   int i;
   for (i=0; i<MAX_FAMILIES; i++)
      if (cur_fams[i].load_id==TEXMEM_LOADID_NULL)
         return &cur_fams[i];
   return NULL;
}

// check for existance of this record
fam_record *_fam_lookup_record(char *str)
{
   int i;
   for (i=0; i<MAX_FAMILIES; i++)
      if (cur_fams[i].load_id!=TEXMEM_LOADID_NULL)
         if (stricmp(cur_fams[i].name,str)==0)
            return &cur_fams[i];
   return NULL;
}

/////////////////
// actual api - internal calls and exposed stuff

// remove this family from the list, synch is TRUE if you want this to
//  do a fam_synch, false if this is part of a larger operation
BOOL family_remove(char *str, BOOL synch)
{
   fam_record *toast_rec=_fam_lookup_record(str);
   if (toast_rec)
   {
      texmemFreeLoadId(toast_rec->load_id);
      toast_rec->load_id=TEXMEM_LOADID_NULL;   // deallocate
      num_fams--;
   }
   if (synch)
      _fam_synch(TRUE);       // let user rezap textures by hand, for now
   return toast_rec!=NULL;
}

// add this family to the list
BOOL family_add(char *str)
{  // see if this family is already in memory
   fam_record *our_rec=_fam_lookup_record(str);
   BOOL new_pal_loaded=FALSE;
   void *mem=NULL;
   char buf[256];

   if (our_rec==NULL)
      our_rec=_fam_get_free_record();
   else  // look, we are replacing and old record
   {     // so save off old data on texture usage and set up
      mem=familyDiskTexBlockBuild();
      family_remove(str,FALSE);
   }

   diskPathMunge(buf,"fam",str,NULL);   // full path for family

   if (!_fam_palette_check(buf))
      Warning(("Fam %s not in correct palette\n",str));

   texmemSetPalette(buf,"full");
   our_rec->load_id=texmemLoadDir(buf);
   if (new_pal_loaded)
      texmemRebuildDefaultMaps();
   texmemSetPalette(NULL,NULL);

   strncpy(our_rec->name,str,FAM_NAME_LEN);
   num_fams++;
   if (mem)
   {
      familyDiskTexBlockLoad(mem);
      Free(mem);
   }
   _fam_synch(mem!=NULL); // if mem, we might have deleted some, since we swapped

   if (texmemSpaceOverrun())
      Status("Out of Texture Space\n");
   return TRUE;           // if !mem, we added to end, so no need to remap fancy
}

// load a set of water textures
BOOL family_load_water(char *water_prefix)
{
   char buf[256], *base=get_water_base();
   BOOL rv=TRUE;

   texmemSetPalette(base,"full");
   strcpy(buf,water_prefix); strcat(buf,"in");
   if (texmemLoadSingleAtIdx(base,buf,WATERIN_IDX) != TEXMEM_NO_HND)
      BuildOneTextureArchetype(WATERIN_IDX);
   else
      rv = FALSE;

   strcpy(buf,water_prefix); strcat(buf,"out");
   if (texmemLoadSingleAtIdx(base,buf,WATEROUT_IDX) !=TEXMEM_NO_HND)
      BuildOneTextureArchetype(WATEROUT_IDX);
   else
      rv = FALSE;

   _fam_synch(FALSE);    // we are adding at a special idx, so no remap
   if (rv) strcpy(_fam_water_name,water_prefix);
   if (texmemSpaceOverrun())
      Status(("Out of Texture Space\n"));

   texmemSetPalette(NULL,NULL);
   return rv;
}

BOOL family_add_water(const char* prefix)
{
   char buf[256], *base=get_water_base();
   BOOL rv=TRUE;

   texmemSetPalette(base,"full");
   strcpy(buf,prefix); strcat(buf,"in");
   rv&=(texmemLoadSingle(base,buf)!=TEXMEM_NO_HND);
   strcpy(buf,prefix); strcat(buf,"out");
   rv&=(texmemLoadSingle(base,buf)!=TEXMEM_NO_HND);
   _fam_synch(FALSE);
   texmemSetPalette(NULL,NULL);
   return rv;
}

static BOOL _get_sky_palette(char *path_base, char *sky_name)
{
   char sky_pal_name[32];
   sprintf(sky_pal_name,"%sp",sky_name);
   if (!texmemSetPalette(path_base,sky_pal_name))
      if (!texmemSetPalette(path_base,"full"))
         return FALSE;
   return TRUE;
}

// Works on skies, too !- well, not really
static void reload_single_water(int i)
{
   char buf[256];
   IRes* hnd = texmemGetHandle(i);
   char* name;

   if (hnd == NULL)
      return;

   IRes_GetCanonPathName(hnd,&name);
   strncpy(buf,name,sizeof(buf));
   Free(name);

   buf[sizeof(buf)-1] = '\0';
   if (fam_name_examine(buf))
   {
      char* path = NULL;
      char* name = buf;
      char* slash = strrchr(buf,'\\');

      // break texture up into path and name
      if (slash)
      {
         *slash = '\0';
         name = slash+1;
         path = buf;
      }

      // reload the texture
      if (i>BACKHACK_IDX)      // try to get the sky correct, hate hate
         _get_sky_palette(path,_fam_sky_name);
      else
         texmemSetPalette(path,"full");
      texmemLoadSingleAtIdx(path,name,i);
      texmemSetPalette(NULL,NULL);

      ForciblyRebuildOneTextureArchetype(i);
   }
}

void family_reload_water(void)
{
   BOOL hw = (GetGameScreenMode()->flags & kScrnMode3dDriver) != 0;
   int new_type = hw ? kDirHW : kDirSW;
   int i;

   portal_cleanup_water_hack();

   // no need to reload
   if (new_type == cur_render_type)
      return ;
   cur_render_type = new_type;

   for (i = 0; i < texmemGetMax(); i++)
      reload_single_water(i);

   reload_single_water(WATERIN_IDX);
   reload_single_water(WATEROUT_IDX);
   for (i = BACKHACK_IDX+1; i < 256; i++)
      reload_single_water(i);
}

BOOL family_texture_is_water(int i)
{
   char buf[256];
   IRes* hnd = texmemGetHandle(i);
   char* name;

   if (hnd == NULL)
      return FALSE;

   IRes_GetCanonPathName(hnd,&name);
   strncpy(buf,name,sizeof(buf));
   Free(name);

   buf[sizeof(buf)-1] = '\0';
   return fam_name_examine_dir(buf,kDirWater);
}

static int find_single(const char* base, const char* name)
{
   int hnd = texmemGetSinglebyName((char*)base,(char*)name);
   if (hnd == TEXMEM_NO_HND)
   {  // should i do palette stuff here, or not... tough to tell
      texmemSetPalette((char *)base,"full");
      hnd = texmemLoadSingle((char*)base,(char*)name);
      texmemSetPalette(NULL,NULL);
   }
   return hnd;
}

BOOL family_find_water(const char* prefix, int* handles, int nhandles)
{
   char buf[256], *base=get_water_base();
   BOOL rv=TRUE;
   int i = 0;
   int hnd[2];

   strcpy(buf,prefix); strcat(buf,"in");
   hnd[i]=find_single(base,buf);
   rv &= hnd[i++] != TEXMEM_NO_HND;

   strcpy(buf,prefix); strcat(buf,"out");
   hnd[i]=find_single(base,buf);
   rv &= hnd[i++] != TEXMEM_NO_HND;

   if (nhandles > i) nhandles = i;
   memcpy(handles,hnd,i*sizeof(int));

   _fam_synch(FALSE);
   return rv;
}

#ifdef EDITOR
void family_cmd_add_water(char *water_prefix)
{
   if (!family_add_water(water_prefix))
      Status("Couldnt load water");
   else
      Status("Water loaded");
}

static void family_cmd_free_water(char *water_prefix)
{
   char buf[256], *base=get_water_base();
   BOOL rv=TRUE;

   strcpy(buf,water_prefix); strcat(buf,"in");
   rv&=texmemFreeSinglebyName(base,buf);
   strcpy(buf,water_prefix); strcat(buf,"out");
   rv&=texmemFreeSinglebyName(base,buf);
   _fam_synch(FALSE);
   if (!rv)
      Status("Problem unloading water");
   else
      Status("Water unloaded");
}
#endif

// @HATEFUL:  SHOULD NOT NEED THIS

// add a "which type" to the end
static char dir_char[6]={'t','b','e','n','w','s'};
#define SKY_BASE (BACKHACK_IDX+1)

// specific sky loading stuff
BOOL family_load_sky(char *sky_name)
{
   char *path_base=get_sky_base();
   BOOL rv=TRUE;
   int i;

   // first remove any existing one?
   for (i=0; i<6; i++)
      texmemFreeSingle(i+SKY_BASE);  // get rid of old ones
   _get_sky_palette(path_base,sky_name);
   if (sky_name&&sky_name[0]!='\0')
   {
      mipmap_enable=FALSE;
      for (i=0; i<6; i++)
      {
         char which[32];
         sprintf(which,"%s%c",sky_name,dir_char[i]);
         rv|=(texmemLoadSingleAtIdx(path_base,which,i+SKY_BASE)!=-1);
      }
      mipmap_enable=TRUE;
      _fam_synch(FALSE);   // we are adding at specific idx, no need to remap
   }
   else
      rv=FALSE;
   texmemSetPalette(NULL,NULL);
   if (rv)
      strcpy(_fam_sky_name,sky_name);
   else
      strcpy(_fam_sky_name,"");
   return rv;
}

#ifdef EDITOR

#define TZAP_ALL     (-2)
#define TZAP_INVALID (-1)

// probably better handled by building a replacement array and calling the remappers
static void _fam_texture_zap(short oval, short nval)
{
   int i, mapping[TEXMEM_MAX];

   for (i=0; i<TEXMEM_MAX; i++)    // @TODO: get rid of this waterin_idx hack
      if ((i<WATERIN_IDX)&&
          ((oval==i)||(oval==TZAP_ALL)||((oval==TZAP_INVALID)&&(i>=texmemGetMax()))))
         mapping[i]=nval;
      else
         mapping[i]=i;
   _familyRemapBrushes(mapping,TEXMEM_MAX);
   _familyRemapCells(mapping,TEXMEM_MAX);
   _fam_synch(FALSE);
}

/////////////////
// commands for user in editor

// add this family to existing list
static void family_cmd_add(char *str)
{
   if (family_add(str))
      Status("Added Family");
   else
      Status("Couldnt Add Family");
}

// backwards compatible... clear all existing and load a new family
static void family_cmd_load(char *str)
{
   void *mem=NULL;
   if (_fam_lookup_record(str))       // have to do this explicitly here, since we
      mem=familyDiskTexBlockBuild();  // are gonna hard kill everything here on
   _fam_remove_all();                 // the next line
   family_cmd_add(str);
   if (mem)
   {
      familyDiskTexBlockLoad(mem);
      Free(mem);
   }
}

// remove this family
static void family_cmd_remove(char *str)
{
   if (stricmp("all",str)==0)
   {
      _fam_remove_all();
      Status("Removed all textures");
   }
   else    // just remove this family
      if (family_remove(str,TRUE))
         Status("Freed family");
      else
         Status("No such family to remove");
}


static BOOL _parse_str_fam_txt(char *str, char **fam, char **txt)
{
   char *p;
   while ((*str==' ')&&(*str!='\0')) str++;  // get past any initial space
   if (*str!='\0')
      if ((p=strchr(str,' '))!=NULL)
      {
         char *eos=p;
         while ((*p==' ')&&(*p!='\0')) p++;  // get past any initial space
         if (*p!='\0')
         {
            *eos='\0';
            *fam=str;
            *txt=p;
            return TRUE;
         }
      }
   return FALSE;
}

static int next_slot_to_use = -1;

// load a single texture - of form "load_single FamName TxtName"
static void family_cmd_load_single(char *str)
{  // parse the string out as a famname and txtname
   char *fam, *txt;
   bool loaded=FALSE;

   if (_parse_str_fam_txt(str,&fam,&txt))
   {
      char buf[256];
      diskPathMunge(buf,"fam",fam,NULL);   // full path for family
      texmemSetPalette(buf,"full");
      if (next_slot_to_use==-1)
         loaded=texmemLoadSingle(buf,txt);
      else
         loaded=texmemLoadSingleAtIdx(buf,txt,next_slot_to_use);
      texmemSetPalette(NULL,NULL);
   }
   if (loaded)
      Status("Loaded single texture");
   else
      Status("Could not find texture");
}

// delete a single texture
static void family_cmd_delete_single(char *str)
{
   char *fam, *txt;
   BOOL free_d=FALSE;

   if (_parse_str_fam_txt(str,&fam,&txt))
   {
      char buf[256];
      diskPathMunge(buf,"fam",fam,NULL);   // full path for family
      free_d=texmemFreeSinglebyName(buf,txt);
   }
   if (free_d)
   {
      Status("Freed single texture");
      _fam_synch(TRUE);   // need to go remap since we punted something
   }
   else
      Status("Could not find texture");
}

BOOL family_free_single(int handle)
{
   BOOL free_d;
   fam_record *toast_rec=NULL;
   if (texmem_loadid[handle] != TEXMEM_LOADID_NULL)
   {
      int i;
      for (i = 0; i < MAX_FAMILIES; i++)
         if (cur_fams[i].load_id==texmem_loadid[handle])
            toast_rec = &cur_fams[i];
   }

   free_d=texmemFreeSingle(handle);
   if (free_d)
   {
      if (toast_rec!=NULL)
         toast_rec->load_id=TEXMEM_LOADID_NULL;
      _fam_synch(TRUE);   // need to go remap since we punted something
   }
   return free_d;
}

// load a water set from fam\water
static void family_cmd_water(char *str)
{
   if (family_load_water(str))
      Status("Loaded water textures");
   else
      Status("Failed to load water");
}

// load a sky background thing
static void family_cmd_sky(char *sky_name)
{
   if (family_load_sky(sky_name))
      Status("Sky loaded");
   else
      Status("Could not load Sky");
}

// set all textures that are invalid to newval
static void family_cmd_zap(int newval)
{
   _fam_texture_zap(TZAP_INVALID,(short)newval);
}

static void family_cmd_zap_all(void)
{
   _fam_texture_zap(TZAP_ALL,0);
}

// swap all faces using texture oldval to use newval
static void family_cmd_swap(char *str)
{
   int oldval=0, newval=0;
   if (sscanf(str,"%d,%d",&oldval,&newval)==2)
      _fam_texture_zap((short)oldval,(short)newval);
   else   // maybe it is two names...
      Status("Couldnt interpret command");
}         // @TODO: add simple code here to decode as two strings and try that mapping

// show the current family state
static void family_dump_info(void)
{
   int i;

   mprintf("Sky   %s\n",_fam_sky_name);
   mprintf("Water %s\n",_fam_water_name);
   for (i=0; i<MAX_FAMILIES; i++)
      if (cur_fams[i].load_id!=TEXMEM_LOADID_NULL)
         mprintf("Fam %d %s\n",i,cur_fams[i].name);
}

static void secret_fam_rem_name(char *str)
{
   fam_record *toast_rec=_fam_lookup_record(str);
   toast_rec->load_id=TEXMEM_LOADID_NULL;
   num_fams--;
}

//////////////////
// editor

static Command family_commands[]=
{
   { "load_family", FUNC_STRING, family_cmd_load, "old style single family load\n" },
   { "add_family", FUNC_STRING, family_cmd_add, "add another texture family" },
   { "remove_family", FUNC_STRING, family_cmd_remove, "remove family, use <all> to clear all" },
   { "load_sky", FUNC_STRING, family_cmd_sky, "load a sky bitmap set" },
   { "load_water", FUNC_STRING, family_cmd_water, "load a water texture" },
   { "add_water", FUNC_STRING, family_cmd_add_water, "add a water texture" },
   { "free_water", FUNC_STRING, family_cmd_free_water, "free a water texture" },
   { "load_a_texture", FUNC_STRING, family_cmd_load_single, "load a single texture, load <fam> <txtname>" },
   { "remove_a_texture", FUNC_STRING, family_cmd_delete_single, "delete single, <fam> <name>" },
   { "texture_zap", FUNC_INT, family_cmd_zap, "zap invalid textures to newval" },
   { "texture_zap_all", FUNC_VOID, family_cmd_zap_all, "zap all textures to newval" },
   { "texture_change", FUNC_STRING, family_cmd_swap, "swap oval,nval - modifies all faces" },
   { "no_fam_compress", TOGGLE_BOOL, &_fam_no_compress, "disallow texture space compression" },
   { "family_dump", FUNC_INT, family_dump_info, "dump all current family info" },
   { "secret_rem_family_name", FUNC_STRING, secret_fam_rem_name, "hi dorian" },
   { "sload_next_slot", VAR_INT, &next_slot_to_use, "set next load slot" },
   { "sload_no_remap", TOGGLE_BOOL, &no_remap_on_load, "no disk block on load" },
};
#endif

//////////////////
// startup/shutdown

void family_init(void)
{
   BOOL hw = (GetGameScreenMode()->flags & kScrnMode3dDriver) != 0;
   cur_render_type = hw ? kDirHW : kDirSW;
   _fam_remove_all();
#ifdef EDITOR
   family_edit_init();
   COMMANDS(family_commands,HK_EDITOR);
#endif
}

void family_term(void)
{
   _fam_remove_all();
   texmemShutdown();
}

//////////////////
// calls for editsave stuff

void *family_name_block_build(int *cnt, int *size_per)
{
   char *mem=Malloc((MAX_FAMILIES+2)*FAM_NAME_LEN), *famlist;
   int i;

   strncpy(mem+FAM_NAME_LEN*0,_fam_sky_name,FAM_NAME_LEN);
   strncpy(mem+FAM_NAME_LEN*1,_fam_water_name,FAM_NAME_LEN);
   famlist=mem+FAM_NAME_LEN*2;
   for (i=0; i<MAX_FAMILIES; i++, famlist+=FAM_NAME_LEN)
      if (cur_fams[i].load_id!=TEXMEM_LOADID_NULL)
         strncpy(famlist,cur_fams[i].name,FAM_NAME_LEN);
      else
         strncpy(famlist,"NULL",FAM_NAME_LEN);
   *cnt=i+2;
   *size_per=FAM_NAME_LEN;
   return mem;
}

BOOL family_name_block_parse(int cnt, int size_per, void *mem)
{
   char *famlist, *sky_name, *water_name;
   int i;

   if (cnt<2) return FALSE;   // bogus data, somehow
   _fam_remove_all();
   sky_name=(char *)mem;
   water_name=sky_name+size_per;   // skip to next
   famlist=water_name+size_per;   // skip to next
   for (i=0; i<cnt-2; i++, famlist+=size_per)
      if (strnicmp(famlist,"NULL",FAM_NAME_LEN)==0)
         cur_fams[i].load_id=TEXMEM_LOADID_NULL;
      else
         family_add(famlist);
   if (strlen(sky_name)) family_load_sky(sky_name);
   if (strlen(water_name)) family_load_water(water_name);
   return TRUE;
}

void family_clear_all(void)
{
   _fam_remove_all();
}

#ifdef OLD
//////////////////
// compatibility calls for blfile support
void family_load(char *str)
{
   _fam_remove_all();
   family_add(str);
}

char *family_which(void)
{
   int i;
   for (i=0; i<MAX_FAMILIES; i++)
      if (cur_fams[i].load_id!=TEXMEM_LOADID_NULL)
         return cur_fams[i].name;
   return NULL;
}

char *sky_which(void)
{
   return _fam_sky_name;
}
#endif
