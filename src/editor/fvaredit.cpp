/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/fvaredit.cpp,v 1.3 2000/02/19 13:10:40 toml Exp $

#include <appagg.h>
#include <filevar.h>
#include <fvaredit.h>
#include <command.h>
#include <simpwrap.h>
#include <dynarray.h>

#include <sdestool.h>
#include <edittool.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

typedef cDynArray<cFileVarBase*> cVarArray; 
typedef cDynArray<const char*> cStringArray; 

static const char* titles[] = { "Misson Variables", "GameSys Variables", "Campaign Variables" }; 

static void edit_filevar_type(int file)
{
   cVarArray vars;
   cStringArray names; 
   int n = 0; 

   AutoAppIPtr_(StructDescTools,pSdescTools);
   AutoAppIPtr(EditTools); 

   cFileVarList* list = cFileVarBase::AllFileVars(); 

   if (list != NULL)
      for (cFileVarList::cIter iter = list->Iter(); !iter.Done(); iter.Next())
      {
         cFileVarBase* fv = iter.Value(); 
         if (fv->mDesc->file == file && pSdescTools->Lookup(fv->mDesc->type))
         {
            n++;
            vars.Append(fv);
            names.Append(fv->mDesc->friendly_name); 
         }
      }

   if (n == 0)
   {
      n++; 
      names.Append("<No vars>"); 
   }

   int i = PickFromStringList((char*)titles[file],names,n); 

   if (i >= 0 && i < vars.Size())
   {
      cFileVarBase* fv = vars[i]; 
      if (pEditTools->EditTypedData(names[i],fv->mDesc->type,fv->mBlock) == S_OK)
         fv->Update(); 
   }
}


//
// Command list 
//

static Command commands[] = 
{
   { "edit_file_vars", FUNC_INT, edit_filevar_type, "0 = mission vars, 1 = gamesys vars, 2 = cmpaign vars" }, 
};

void FileVarEditInit(void)
{
   COMMANDS(commands,HK_EDITOR); 
}

void FileVarEditTerm(void)
{
}
