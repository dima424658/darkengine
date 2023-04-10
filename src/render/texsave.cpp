/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/texsave.cpp,v 1.4 1998/10/05 17:28:29 mahk Exp $
// editorlevel file format and save/load code

//  Tag: Families      (FAMILY)
//    contains a header and then a list of the current families
//  Tag: Textures      (TXLIST)
//    resname memory block for the textures in memory

#include <lg.h>
#include <mprintf.h>
#include <timer.h>

#include <lresdisk.h>

#include <tagfile.h>
#include <vernum.h>

#include <family.h>
#include <csg.h>

#include <status.h>
#include <texsave.h>
#include <texprop.h>

// Must be last header 
#include <dbmem.h>


static BOOL texsave_enabled = TRUE; 

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
   
   if (file->OpenBlock(tag,ver)!=S_OK) return FALSE;
   write_sz=hdr->elem_cnt*hdr->size_per;
   if (file->Write((char*)hdr,sizeof(blockHeader))==sizeof(blockHeader))
      if (file->Write((char*)mem,write_sz)==write_sz)
         rv=TRUE;
   file->CloseBlock();
   return rv;
}

// LOAD
static void *_loadGenericBlocked(ITagFile *file, TagFileTag *tag, TagVersion *ver, TagVersion *newver, blockHeader *hdr)
{
   char *mem=NULL;

   if (file->OpenBlock(tag,newver)!=S_OK) return NULL;
   if (file->Read((char*)hdr,sizeof(blockHeader))==sizeof(blockHeader))
   {
      int read_sz=hdr->elem_cnt*hdr->size_per;
      mem=(char*)Malloc(read_sz); 
      if (mem!=NULL)
         if (file->Read(mem,read_sz)!=read_sz)
         {
            Free(mem);    // we are gonna return failure
            mem=NULL;     // but lets free our memory block first
         }
   }
   file->CloseBlock();
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
      if (file->OpenBlock(&TextureTag,&TextureVer)!=S_OK) return FALSE;
      rv=(file->Write((char*)mem,size)==size);
      file->CloseBlock();
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
   
   if (file->OpenBlock(&TextureTag,&LocalVer)!=S_OK) return FALSE;
   size=file->BlockSize(file->CurrentBlock());
   mem=(void *)Malloc(size);
   rv=(file->Read((char *)mem,size)==size);
   Assrt(size==ResBlockSize(mem));
   file->CloseBlock();
   if (rv)           // successfully read the data, now lets try to load
      rv=familyDiskTexBlockLoad(mem);  // the resnameblock in for real...
   Free(mem);  // since we malloced it and dont need it any more
   return rv;
}



////////////////////////
// MAIN STUFF
// these are the actual calls made by the outside world

BOOL texture_Save(ITagFile *file)
{
   if (!texsave_enabled)
      return FALSE; 

   BOOL rv = FALSE;
   rv |= _saveAllFamilies(file);
   rv |= _saveAllTextures(file);
   if (!rv)
      Warning(("Some texture component of the file saved incorrectly"));
   return rv;
}

BOOL texture_Load(ITagFile *file)
{
   if (!texsave_enabled)
      return FALSE; 

   BOOL rv = FALSE;
   
   rv |= _loadAllFamilies(file);   
   rv |= _loadAllTextures(file);
#ifdef COMPARE_RES
   // The theory is that we don't need this any more...
   txtprop_load();
#endif
   if (texmemSpaceOverrun())
   {
      mprintf("HEY! YOU! Out of Texture Slots!\n");
      texmemSpaceOverrun()=0; // well, reset it, at least
   }  // hateful syntax, i know, sorry, will fix someday - dc

   if (!rv)
      Warning(("Some texture component of the file loaded incorrectly"));

   return rv;
}


BOOL texture_EnableSaveLoad(BOOL enabled)
{
   BOOL old = texsave_enabled;
   texsave_enabled= enabled;
   return old; 
}
