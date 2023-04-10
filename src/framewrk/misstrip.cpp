/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/misstrip.cpp,v 1.4 2000/02/19 13:16:27 toml Exp $
#include <config.h>
#include <cfg.h>
#include <stdio.h>
#include <string.h>
#include <tagfile.h>
#include <vernum.h>
#include <stdlib.h>
#include <datapath.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


////////////////////////////////////////////////////////////
// STAND-ALONE MISSION FILE STRIPPER 
//

//
// Tags we exclude 
//

static const TagFileTag exclude_tags[] = 
{
   { "BRLIST" },
   { "BRHEAD" },
   { "HotRegions" },

#ifdef NON_BRUSH_STUFF
   { "FAMILY" },
   { "TXLIST" },
#endif // 
}; 

#define NUM_EXCLUDED (sizeof(exclude_tags)/sizeof(exclude_tags[0]))

//
// Check for excluded tag
//

BOOL IsExcluded(const TagFileTag& tag)
{
   for (int i = 0; i < NUM_EXCLUDED; i++)
   {
      const TagFileTag& exclude = exclude_tags[i]; 

      if (strcmp(exclude.label,tag.label) == 0)
         return TRUE; 
   }

   char buf[64]; 
   sprintf(buf,"exclude_%s",tag.label); 
   if (config_is_defined(buf))
      return TRUE; 

   return FALSE; 
}

//
// Utterly cribbed from cfg tool 
// 


BOOL find_file_in_config_path(char* targ, const char* filename, const char* path_var)
{
   Datapath path;
   char pathbuf[80] = "";
   BOOL result;
   DatapathClear(&path);
   config_get_raw(path_var,pathbuf,sizeof(pathbuf));
   DatapathAdd(&path,pathbuf);
   result = DatapathFind(&path,filename,targ,80);
   DatapathFree(&path);
   return result;
}


//
// Main.
//

void main(int argc, const char* argv[])
{

   // usage
   if (argc <= 2)
   {
      printf("Usage: %s <in> <out>",argv[0]); 
      printf("Strips editor tags\n"); 
      exit(1); 
   }

   config_startup("misstrip.cfg"); 
   config_parse_commandline(argc,argv,NULL); 

   char srcname[256]; 

   if (!find_file_in_config_path(srcname,argv[1],"path"))
   {
      printf("Couldn't find file %s\n",argv[1]); 
      exit(1); 
   }
   
   ITagFile* srcfile = TagFileOpen(srcname,kTagOpenRead); 
   ITagFile* dstfile = TagFileOpen(argv[2],kTagOpenWrite); 

   ITagFileIter* iter = srcfile->Iterate(); 

   for (iter->Start(); !iter->Done(); iter->Next())
   {
      const TagFileTag* tag = iter->Tag(); 

      if (config_is_defined("spew_blocks"))
         printf("Tag block %s is %d bytes\n",tag->label,srcfile->BlockSize(tag));
      if (IsExcluded(*tag))
         continue; 

      // the oldest possible version 
      TagVersion v = { 0xFFFFFFF, 0xFFFFFFFF };  

      Verify(SUCCEEDED(srcfile->OpenBlock(tag,&v))); 
      Verify(SUCCEEDED(dstfile->OpenBlock(tag,&v))); 

      // read and write 16k blocks
      static char buf[16384]; 

      while(srcfile->TellFromEnd() > 0)
      {
         int len = srcfile->Read(buf,sizeof(buf)); 
         Verify(dstfile->Write(buf,len) == len); 
      }

      Verify(SUCCEEDED(srcfile->CloseBlock())); 
      Verify(SUCCEEDED(dstfile->CloseBlock())); 
      
      
      
   }

   SafeRelease(iter);
   SafeRelease(srcfile);
   SafeRelease(dstfile); 
   
   config_shutdown(); 
      
}


