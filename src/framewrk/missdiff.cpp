/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/missdiff.cpp,v 1.2 2000/02/19 13:16:26 toml Exp $
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
// STAND-ALONE MISSION FILE DIFF TOOL 
//

//
// Check for excluded tag
//

BOOL IsExcluded(const TagFileTag& tag)
{
   char buf[64]; 
   sprintf(buf,"exclude_%s",tag.label); 
   if (config_is_defined(buf))
      return TRUE; 

   return FALSE; 
}

#define ConfigSpew(x,y) if (config_is_defined(x)) printf y

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

// the oldest possible version 
const TagVersion oldest_version = { 0x7FFFFFFF, 0x7FFFFFFF }; 

// Buffer for tagfile ops
static char io_buf[16384]; 


BOOL block_equal(ITagFile* file0, ITagFile* file1, const TagFileTag* tag)
{
   if (file0->BlockSize(tag) != file1->BlockSize(tag))
   {
      ConfigSpew("spew_diff",("Different sizes\n")); 
      return FALSE; 
   }

   TagVersion v = oldest_version; 

   HRESULT retval0 = file0->OpenBlock(tag,&v); 
   HRESULT retval1 = file1->OpenBlock(tag,&v); 

   if (FAILED(retval0) && FAILED(retval1))  // No such block?  
      return TRUE; 

   BOOL retval = TRUE; 

   Assert_(SUCCEEDED(retval0) && SUCCEEDED(retval1)); 

   // chop the buffer in half 
   char* buf0 = io_buf; 
   char* buf1 = io_buf+sizeof(io_buf)/2; 
   int bufsize = sizeof(io_buf)/2; 

   while(file1->TellFromEnd() > 0)
   {
      int len0 = file0->Read(buf0,bufsize); 
      int len1 = file1->Read(buf1,bufsize); 
      Assert_(len0 == len1);
      if (memcmp(buf0,buf1,len0) != 0)
      {
         ConfigSpew("spew_diff",("Memcmp failed\n")); 
         retval = FALSE; 
         break; 
      }



   }

   Verify(SUCCEEDED(file0->CloseBlock())); 
   Verify(SUCCEEDED(file1->CloseBlock())); 

   return retval; 
}

void copy_block(ITagFile* dstfile, ITagFile* srcfile, const TagFileTag* tag)
{
   TagVersion v = oldest_version;

   Verify(SUCCEEDED(srcfile->OpenBlock(tag,&v))); 
   Verify(SUCCEEDED(dstfile->OpenBlock(tag,&v))); 

   char* buf = io_buf; 
   int bufsize = sizeof(io_buf); 

   while(srcfile->TellFromEnd() > 0)
   {
      int len = srcfile->Read(buf,bufsize); 
      Verify(dstfile->Write(buf,len) == len); 
   }

   Verify(SUCCEEDED(srcfile->CloseBlock())); 
   Verify(SUCCEEDED(dstfile->CloseBlock())); 
}


//
// Main.
//

void main(int argc, const char* argv[])
{

   // usage
   if (argc <= 2)
   {
      printf("Usage: %s <file1> <file2> <outfile>",argv[0]); 
      printf("outfile will have only the tags in file2 that are different from file1\n"); 
      exit(1); 
   }

   config_startup("missdiff.cfg"); 
   config_parse_commandline(argc,argv,NULL); 

   char fullname[2][256]; 

   for (int i = 0; i < 2; i++)
      if (!find_file_in_config_path(fullname[i],argv[i+1],"path"))
      {
         printf("Couldn't find file %s\n",argv[i+1]); 
         exit(1); 
      }
   
   ITagFile* file0 = TagFileOpen(fullname[0],kTagOpenRead); 
   ITagFile* file1 = TagFileOpen(fullname[1],kTagOpenRead); 

   printf("diffing %s from %s\n",fullname[0],fullname[1]);

   ITagFile* outfile = TagFileOpen(argv[3],kTagOpenWrite); 

   Assert_(file0 && file1 && outfile); 

   ITagFileIter* iter = file1->Iterate(); 

   for (iter->Start(); !iter->Done(); iter->Next())
   {
      const TagFileTag* tag = iter->Tag(); 


      if (config_is_defined("spew_blocks"))
         printf("Tag block %s is %d bytes\n",tag->label,file1->BlockSize(tag));

      if (IsExcluded(*tag))
         continue; 

      if (!block_equal(file0,file1,tag))
      {
         if (config_is_defined("spew_diff_blocks"))
            printf("Saving tag %s\n",tag->label); 
         copy_block(outfile,file1,tag); 
      }

      
      
   }

   SafeRelease(iter);
   SafeRelease(file0); 
   SafeRelease(file1);
   SafeRelease(outfile);
   
   config_shutdown(); 
      
}


