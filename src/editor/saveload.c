/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/saveload.c,v 1.22 2000/02/19 13:13:50 toml Exp $
// abstracted save/load for our burgeoning area of save/load formats

// for now, so we can make sure we dont have old broken data in vbrush saveoffs
#define PROTECT_DELETE

#include <stdlib.h>     // getenv

#include <lg.h>
#include <config.h>

#include <editbr.h>
#include <editbr_.h>
#include <brlist.h>
#include <brinfo.h>
#include <editobj.h>
#include <objmodel.h>
#include <vbrush.h>
#include <primal.h>
#include <mprintf.h>
#include <objremap.h>

#include <saveload.h>
#include <ged_rmap.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// global which gets set and looked at for knowing name/user of current level
levelNameInfo saveloadLevelInfo;

// NOTE: this assume brush field update (for obj's) does
//  1) if obj exists, attach brush and sync brush to obj. 
//  2) if no obj, create it from the brush

// read in the brushes with the readfunc, insert into brush list
// if num_brush is != SAVELOAD_NO_COUNT, it will read that many
// returns TRUE if SAVELOAD_NO_COUNT, else returns whether it read num_brush brushes
bool saveloadReadBrushes(bool (*readnext)(editBrush *newbr), int num_brush, BOOL preserve_id)
{
   editBrush us, *tmp, *old_curs=blistGet();

   while ((*readnext)(&us))
   {
      // ignore object brushes - MAHK
      if (brushGetType(&us) == brType_OBJECT)
         continue;

      tmp=brushCopy(&us);
#ifdef PROTECT_DELETE
      if ((tmp->flags&kEBF_BeingDeleted)!=0)
      {
         Warning(("Hey! Brush %d is mid delete, it thinks\n",us.br_id));
         tmp->flags&=~kEBF_BeingDeleted;
      }
#endif
      if (preserve_id)
         tmp->br_id=us.br_id;   // here we copy the brush ids across for csg_brush
      blistInsert(tmp);         //   remapping compatibility
      blistSetPostoBrush(tmp);  // hack to try and fix the reversal of time on insert in middle
      //      brush_field_update(tmp,NULL,brField_Old);      // see NOTE above about brush_field_update
      if (num_brush!=SAVELOAD_NO_COUNT)  // if we have been given a count
         if (--num_brush==0)    // see if we have reached it
            break;              // done with real thing
   }
   if (preserve_id)
      ged_remap_brushes();
   blistSetPostoBrush(old_curs);
   return !(num_brush>0);     // if >0, we thought we had count, but failed somehow
}

BOOL saveloadPostLoadBrushes(void)
{
   int hIter;
   editBrush* us=blistIterStart(&hIter);
   while (us!=NULL)
   {
     //to fix room brush number remapping.
     if (brushGetType(us) == brType_ROOM)
       {
	 us->primal_id = ObjRemapOnLoad(us->primal_id);
       }
      brush_field_update(us,NULL,brField_Old);      // see NOTE above about brush_field_update
      us=blistIterNext(hIter);
   }
   return TRUE;
}

// writes out the brushes, by computing the correct size and passing it and
// the brush to the write function, which is supposed to store it out
bool saveloadWriteBrushes(bool (*writebr)(editBrush *br, int size), int groupFilter)
{
   int base_sz = sizeof(editBrush) - EDITBR_MAX_FACES * sizeof(TexInfo);
   editBrush *us;
   int hIter;
   
   us=blistIterStart(&hIter);
   while (us!=NULL)
   {
      int xtra_sz=0;
      if ((groupFilter==0)||(brSelect_Group(us)==groupFilter))
      {
         int type = brushGetType(us);
#ifdef PROTECT_DELETE
         if ((us->flags&kEBF_BeingDeleted)!=0)
         {
            Warning(("Hey! Brush %d is mid delete, it thinks\n",us->br_id));
            us->flags&=~kEBF_BeingDeleted;
         }
#endif
         if (type==brType_TERRAIN)
            xtra_sz+=us->num_faces * sizeof(TexInfo);
         if (!(*writebr)(us,base_sz+xtra_sz))
            return FALSE;
      }
      us=blistIterNext(hIter);
   }
   return TRUE;
}

// reads contiguous model strings in from the file,
// md_cnt is number to read, name_len is strlen of each name
// readnext should return FALSE if it cant read the model
bool saveloadReadModels(bool (*readnext)(char *model, int len), int md_cnt, int name_len)
{
   int i;
   char *name=(char *)Malloc(name_len+1);

   for (i=0; i<md_cnt; i++)
   {
      if (!(*readnext)(name,name_len))
         break;
      if (name[0]!='\0')
         objmodelLoadModelIntoHandle(name,i);
   }
   Free(name);
   return i==md_cnt;
}

// same as ReadModel, basically
bool saveloadWriteModels(bool (*writenxt)(char *model, int len), int md_cnt, int name_len)
{
   char noname[]="";
   int i;
   
   for (i=0; i<md_cnt; i++)
   {
      char *name=objmodelGetName(i);
      if (name==NULL) name=noname;
      if (!(*writenxt)(name,name_len))
         break;
   }
   return i==md_cnt;
}

// get the current username from the environment
char *saveloadGetUser(void)
{
   static char buf[32];
   if (config_get_raw("user",buf,sizeof(buf)))
   {
      buf[sizeof(buf)-1]='\0';
      return buf;
   }
   else
   {
      char* val = getenv("USER");
      if (val != NULL) return val;
   }
   return "UNKNOWN";
}

///////////////
// various format fixup code

// to deal with levels from before group fields
static void _zeroGroup(editBrush *us)
 { brSelect_Group(us)=0; }

void saveloadZeroAllGroups(void)
{
   vBrush_GroupOp(TRUE,_zeroGroup);
}

// for old 0xDD filled fields
static void _clearOldFields(editBrush *us)
 { brSelect_Flag(us)=0; us->edge=us->point=0; us->group_id=0; }

void saveloadCleanVer1File(void)
{
   vBrush_GroupOp(TRUE,_clearOldFields);
}

// to fix texture alignment fields
static void _fixAlign(editBrush *us)
{
   int i;
   if (brushGetType(us)==brType_TERRAIN)
      for (i=0; i<us->num_faces; i++)
      {
         us->txs[i].tx_x/=4;
         us->txs[i].tx_y/=4;
      }
}

void saveloadFixAlignmentFields(void)
{
   vBrush_GroupOp(TRUE,_fixAlign);
}

// to fix old idiotic primal id space stuff
// in case you need to just go do something in code with a particular primal
#define OLD_PRIMAL_CUBE_IDX  (0)
#define OLD_PRIMAL_NUM_NGONS (16-3)
#define OLD_PRIMAL_CYL_BASE  (3)
#define OLD_PRIMAL_PYR_BASE  (3+OLD_PRIMAL_NUM_NGONS)
static void _fixPrimFields(editBrush *us)
{
   if ((brushGetType(us)==brType_TERRAIN)&&(us->primal_id!=OLD_PRIMAL_CUBE_IDX))
   {  // fix the old primal data
      BOOL odd=FALSE;
      if (us->primal_id<OLD_PRIMAL_CYL_BASE)
         odd=TRUE;
      if ((us->primal_id>=OLD_PRIMAL_CYL_BASE)&&(us->primal_id<OLD_PRIMAL_PYR_BASE))
         us->primal_id=primalID_Make(PRIMAL_TYPE_CYLINDER,us->primal_id-OLD_PRIMAL_CYL_BASE+3);
      else if (us->primal_id<OLD_PRIMAL_PYR_BASE+OLD_PRIMAL_NUM_NGONS)
         us->primal_id=primalID_Make(PRIMAL_TYPE_PYRAMID,us->primal_id-OLD_PRIMAL_PYR_BASE+3);
      else
         odd=TRUE;
      if (odd)
      {
         Warning(("Odd old level primal_id %d (on br %d)\n",us->primal_id,us->br_id));
         us->primal_id=PRIMAL_CUBE_IDX;
      }
   }
}

void saveloadFixIntPrimalIDs(void)
{
   vBrush_GroupOp(TRUE,_fixPrimFields);
}

////////////////
// for old misbuilt bad primals on non-terr brushes
static void _fixNonTerrPrimFields(editBrush *us)
{
   if ((brushGetType(us)!=brType_TERRAIN)&&(us->primal_id!=0))
      if (brushGetType(us)!=brType_ROOM)
      {
         us->num_faces=6;
         us->primal_id=0;
      }  // should we play with txs? probably not...
}

void saveloadFixNonTerrPrimalIDs(void)
{
   vBrush_GroupOp(TRUE,_fixNonTerrPrimFields);
}

////////////////
// for clearing the flags field

BOOL loud_flag_clear=FALSE;

static void _fixFlagFields(editBrush *us)
{
   if (loud_flag_clear)
      if (us->flags)
         mprintf("Brush %d had flags %x\n",(int)us->br_id,(int)us->flags);
   us->flags = 0;
}

void saveloadFixFlagField(void)
{
   vBrush_GroupOp(TRUE,_fixFlagFields);
}
