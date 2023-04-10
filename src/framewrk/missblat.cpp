/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/missblat.cpp,v 1.2 2000/02/19 13:16:25 toml Exp $
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
// STAND-ALONE MISSION FILE BLATTER (Brushes)
//

//
// Tags we exclude 
//

static const TagFileTag blatin_tags[] = 
{
   { "BRLIST" },
   { "BRHEAD" },
}; 

#define NUM_BLATTED (sizeof(blatin_tags)/sizeof(blatin_tags[0]))

//
// Check for excluded tag
//

BOOL IsBlatted(const TagFileTag& tag)
{
   for (int i = 0; i < NUM_BLATTED; i++)
   {
      const TagFileTag& blattarget = blatin_tags[i]; 

      if (strcmp(blattarget.label,tag.label) == 0)
         return TRUE; 
   }

   char buf[64]; 
   sprintf(buf,"blat_%s",tag.label); 
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
   if (argc <= 3)
   {
      printf("Usage: %s <file1> <file2> <file3>\n",argv[0]); 
      printf("Takes Brushes from File1 and everything else from File2\n");
      printf("and puts them in file3\n"); 
      exit(1); 
   }

   config_startup("missblat.cfg"); 
   config_parse_commandline(argc,argv,NULL); 

   char srcname[256]; 

   if (!find_file_in_config_path(srcname,argv[1],"path"))
   {
      printf("Couldn't find file %s\n",argv[1]); 
      exit(1); 
   }

   if (!find_file_in_config_path(srcname,argv[2],"path"))
   {
      printf("Couldn't find file %s\n",argv[2]); 
      exit(1); 
   }
   
   ITagFile* brushfile = TagFileOpen(argv[1],kTagOpenRead); 
   ITagFile* restfile = TagFileOpen(argv[2],kTagOpenRead); 
   ITagFile* dstfile = TagFileOpen(argv[3],kTagOpenWrite);

   ITagFileIter* iter = brushfile->Iterate(); 
   ITagFileIter* iterb = restfile->Iterate();


//put the terrain in
   for (iter->Start(); !iter->Done(); iter->Next())
   {
      const TagFileTag* tag = iter->Tag(); 

      if (config_is_defined("spew_blocks"))
         printf("Tag block %s is %d bytes\n",tag->label,brushfile->BlockSize(tag));
      //if we aren't blatting, then don't overwrite it
      if (!IsBlatted(*tag))
         continue; 

      // the oldest possible version 
      TagVersion v = { 0xFFFFFFF, 0xFFFFFFFF };  

      Verify(SUCCEEDED(brushfile->OpenBlock(tag,&v))); 
      Verify(SUCCEEDED(dstfile->OpenBlock(tag,&v))); 

      // read and write 16k blocks
      static char buf[16384]; 

      while(brushfile->TellFromEnd() > 0)
      {
         int len = brushfile->Read(buf,sizeof(buf)); 
         Verify(dstfile->Write(buf,len) == len); 
      }

      Verify(SUCCEEDED(brushfile->CloseBlock())); 
      Verify(SUCCEEDED(dstfile->CloseBlock())); 
      
      
      
   }
   for (iterb->Start(); !iterb->Done(); iterb->Next())
   {
      const TagFileTag* tag = iterb->Tag(); 

      if (config_is_defined("spew_blocks"))
         printf("Tag block %s is %d bytes\n",tag->label,restfile->BlockSize(tag));
      //if it IS blatted, it was already taken care of.
      if (IsBlatted(*tag))
         continue; 

      // the oldest possible version 
      TagVersion v = { 0xFFFFFFF, 0xFFFFFFFF };  

      Verify(SUCCEEDED(restfile->OpenBlock(tag,&v))); 
      Verify(SUCCEEDED(dstfile->OpenBlock(tag,&v))); 

      // read and write 16k blocks
      static char buf[16384]; 

      while(restfile->TellFromEnd() > 0)
      {
         int len = restfile->Read(buf,sizeof(buf)); 
         Verify(dstfile->Write(buf,len) == len); 
      }

      Verify(SUCCEEDED(restfile->CloseBlock())); 
      Verify(SUCCEEDED(dstfile->CloseBlock())); 
      
      
      
   }

   SafeRelease(iter);
   SafeRelease(iterb);
   SafeRelease(restfile);
   SafeRelease(brushfile);
   SafeRelease(dstfile); 
   
   config_shutdown(); 
      
}


