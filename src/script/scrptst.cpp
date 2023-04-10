/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/scrptst.cpp,v 1.6 1998/10/05 17:27:51 mahk Exp $

/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\

   scrptst.cpp

   script state

   Here.  Have a few wrappers for the script library SaveLoad method.

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */


#include <appagg.h>
#include <scrptapi.h>
#include <scrptst.h>
#include <vernum.h>


// Must be last header 
#include <dbmem.h>


/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\

   tag file static structures

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */

static TagFileTag g_TagFileTag = { "SCRIPTSTATE" };
static TagVersion g_TagVersion = { 1, 2 };


/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\

   Our loading and saving pass a simple wrapper into the script system.

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */

static int SaveLoadTagFile(void *pContext, void *pBuff, size_t size)
{
   ITagFile *pTag = (ITagFile *) pContext;
   return ITagFile_Move(pTag, (char*) pBuff, size);
}


void ScriptStateSave(ITagFile *pFile)
{
   HRESULT result = ITagFile_OpenBlock(pFile, &g_TagFileTag, &g_TagVersion);

   if (result == S_OK) {
      AutoAppIPtr(ScriptMan);
      pScriptMan->SaveLoad(SaveLoadTagFile, pFile, FALSE);

      ITagFile_CloseBlock(pFile);
   }
}


void ScriptStateLoad(ITagFile *pFile)
{
   TagVersion found_version = g_TagVersion;
   HRESULT result = ITagFile_OpenBlock(pFile, &g_TagFileTag, &found_version);

   if (result == S_OK
    && found_version.major == g_TagVersion.major
    && found_version.minor == g_TagVersion.minor) {
      AutoAppIPtr(ScriptMan);
      pScriptMan->SaveLoad(SaveLoadTagFile, pFile, TRUE);

      ITagFile_CloseBlock(pFile);
   }
}
