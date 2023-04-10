/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/scrnedit.cpp,v 1.2 2000/02/19 13:11:22 toml Exp $
#include <scrnedit.h>
#include <scrnmode.h>
#include <sdesbase.h>
#include <sdestool.h>
#include <isdesced.h>
#include <isdescst.h>
#include <string.h>

#include <gamescrn.h>
#include <command.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


//------------------------------------------------------------
// Screen mode sdesc 
//

static const char* valid_names[] = 
{
   "Dimensions",
   "Depth",
   "Flags", 
};

static const char* flag_names[] = 
{
   "Fullscreen",
   "Windowed",
   "Hardware",
   "DirectX",
};

static sFieldDesc mode_fields[] = 
{
   { "Valid Parameters", kFieldTypeBits, FieldLocation(sScrnMode,valid_fields), 0, 0, FieldNames(valid_names) }, 
   { "Width", kFieldTypeInt, FieldLocation(sScrnMode,w), kFieldFlagUnsigned }, 
   { "Height", kFieldTypeInt, FieldLocation(sScrnMode,h), kFieldFlagUnsigned }, 
   { "Bit Depth", kFieldTypeInt, FieldLocation(sScrnMode,bitdepth), kFieldFlagUnsigned }, 
   { "Flags", kFieldTypeBits, FieldLocation(sScrnMode,flags), 0, 0, FieldNames(flag_names) }, 
}; 

static sStructDesc mode_sdesc = StructDescBuild(sScrnMode,0,mode_fields); 

//------------------------------------------------------------

BOOL EditScreenMode(const char* title, struct sScrnMode* mode)
{
   sStructEditorDesc eddesc = { "", kStructEditNoApplyButton};
   strncpy(eddesc.title,title,sizeof(eddesc.title));
   eddesc.title[sizeof(eddesc.title)-1] = '\0'; 

   IStructEditor* ed = CreateStructEditor(&eddesc,(sStructDesc*)&mode_sdesc,mode);

   BOOL result = ed->Go(kStructEdModal);
   SafeRelease(ed);
   return result;
}

//------------------------------------------------------------
// Commands
//

static void game_mode_edit(void)
{
   sScrnMode mode = *GetGameScreenMode(); 
   if (EditScreenMode("Default Game Screen Mode",&mode))
      SetGameScreenMode(&mode); 
}

//----------------------------------------

static Command commands[] = 
{
   { "game_mode_edit", FUNC_VOID, game_mode_edit, "Edit screen mode params for game mode.", HK_EDITOR },
}; 

void ScrnEditInit(void)
{
   COMMANDS(commands,HK_ALL); 
}

void ScrnEditTerm(void)
{
}
