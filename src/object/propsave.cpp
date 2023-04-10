/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propsave.cpp,v 1.16 1998/10/05 17:27:23 mahk Exp $
#include <propbase.h>
#include <propert_.h>
#include <dataops_.h>
#include <tagfile.h>
#include <vernum.h>
#include <objdef.h>
#include <iobjsys.h>
#include <config.h>
#include <cfgdbg.h>
#include <objdb.h>

#include <lazyagg.h>

#include <stdlib.h>
#include <string.h>
#include <mprintf.h>

// Must be last header 
#include <dbmem.h>


static LazyAggMember(IObjectSystem) gpObjSys; 

////////////////////////////////////////////////////////////
// DEFAULT PROPERTY SAVE/LOAD IMPLEMENTATION
//

typedef void PropFile;

//----------------------------------------
// Object Header structure 
//

struct ObjHeader 
{
   ObjID obj;  
   ulong len; 
};


//----------------------------------------
// TAG FILE SETUP/CLEANUP
//

#define PROPERTY_TAG_PREFIX "P$"
#define PROPERTY_MAJOR_VERSION 2 

// Here's how we pack the value func version and the property version into the minor version
#define MAKE_MINOR_VERSION(desc,vfunc) (((desc) << 16) | (vfunc)) 
#define MINOR_2_VFUNC(minor)     ((minor) & 0xFFFF)
#define MINOR_2_PROP(minor)      ((minor) >> 16)

static BOOL setup_file(const sPropertyDesc* desc, ITagFile* file, uint& version)
{
   TagFileTag tag = { PROPERTY_TAG_PREFIX } ;
   
   // build the tag
   strncat(tag.label,desc->name,sizeof(tag.label)-strlen(tag.label)-1);
   
   tag.label[sizeof(tag.label)-1] = '\0';

   TagVersion v = { PROPERTY_MAJOR_VERSION, MAKE_MINOR_VERSION(desc->version,version)};
   TagVersion thisver = v;
   BOOL retval = file->OpenBlock(&tag,&v) == S_OK;

   version = MINOR_2_VFUNC(v.minor);

   if (!VersionNumsEqual(&v,&thisver))
   {
      // Previous major versions may not have the prop version
      ulong propver = (v.major < thisver.major) ? 0 : MINOR_2_PROP(v.minor);
      ulong oldest = (desc->oldest != 0) ? desc->oldest : desc->version;

      // Too old.  Nuke it.
      if (propver < oldest)
      {
         ConfigSpew("prop_save_spew",("Old property version.  Purging %s\n",desc->name));
         file->CloseBlock();
         return FALSE;
      }
      
   }

   return retval;
}

static void cleanup_file(ITagFile* file)
{
   file->CloseBlock();
}

#define WRITE(obj)  Write((char*)&obj,sizeof(obj))
#define READ(obj)   Read((char*)&obj,sizeof(obj))

//----------------------------------------
// Write function 
//

struct write_data
{
   ITagFile* file;
   ulong written;

   write_data(ITagFile* f) :file(f),written(0) {}; 
};

static int write_func(PropFile* file, void* buf, int len)
{
   write_data* dat = (write_data*)file;
   len = dat->file->Write((char*)buf,len);
   dat->written += len;
   return len;
}

void cStoredProperty::write_obj(ObjID obj, IDataOpsFile* file, eObjPartition )
{
   ulong cursor = file->Tell();
   ObjHeader header = { obj};

   // make room for header
   file->WRITE(header);

   ulong start = file->Tell(); 
   // write the value
   mpStore->WriteObj(obj,file); 
   ulong end = file->Tell();

   // back patch header 
   file->Seek(kDataOpSeekFromStart,cursor);  
   header.len = end-start; 
   file->WRITE(header); 

   file->Seek(kDataOpSeekFromStart,end);
}


////////////////////////////////////////////////////////////
// SAVE!
//

void cStoredProperty::Save(ITagFile* file, eObjPartition partition)
{
   // Grab the property descriptor 

   // Don't save transient properties
   if (mDesc.flags & kPropertyTransient)
      return;

   uint version = mpStore->WriteVersion();
   setup_file(&mDesc,file,version);

   cDataOpsTagFile opfile(file); 

   ObjID obj;
   sDatum value;
   sPropertyObjIter iter;
   

   mpStore->IterStart(&iter);
   while (mpStore->IterNext(&iter,&obj,&value))
   {  
      if (!gpObjSys->Exists(obj))
      {
         ConfigSpew("prop_save_spew",("Trying to save %s property for nonexistent object %d\n",mDesc.name,obj));
         continue;
      }
      else 
         ConfigSpew("prop_save_spew",("Saving %s property for object %d\n",mDesc.name,obj));

      if (gpObjSys->IsObjSavePartition(obj,partition))
         write_obj(obj,&opfile,partition);
   }
   mpStore->IterStop(&iter);

   cleanup_file(file);
}

//----------------------------------------
// Read function
//

static int read_func(PropFile* file, void* buf, int len)
{
   ITagFile* f = (ITagFile*)file;
   len = f->Read((char*)buf,len);
   return len;
}

void cStoredProperty::read_obj(IDataOpsFile* file, eObjPartition partition, uint version)
{
   ObjHeader header;

   // read in header
   file->READ(header);
   ulong target = file->Tell() + header.len;

   if (gpObjSys->IsObjLoadPartition(header.obj,partition))
   {
      header.obj = gpObjSys->RemapOnLoad(header.obj); 
      if (gpObjSys->Exists(header.obj))
      {
         ConfigSpew("prop_save_spew",("Loading %s property for object %d\n",mDesc.name,header.obj));
         mpStore->ReadObj(header.obj,file,version); 
      }
      else 
      {
         Warning(("Trying to load property %s for nonexistent object %d\n",mDesc.name,header.obj));
      }
   }

   // seek to the end of the value
   file->Seek(kDataOpSeekFromStart,target);
}

void cStoredProperty::Load(ITagFile* file, eObjPartition partition)
{
   // Don't load transient properties
   if (mDesc.flags & kPropertyTransient)
      return;

   uint version = mpStore->WriteVersion(); 
   if (!setup_file(&mDesc,file,version))
      return;

   cDataOpsTagFile opfile(file); 

   while (file->TellFromEnd() > 0)
   {
      read_obj(&opfile,partition,version);
   }
   cleanup_file(file);
}





