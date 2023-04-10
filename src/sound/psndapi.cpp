/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sound/psndapi.cpp,v 1.26 1999/08/05 18:20:56 Justin Exp $
//
// Sound propagation functions
//

#include <lg.h>

#include <timer.h>
#include <playrobj.h>
#include <command.h>
#include <ged_line.h>
#include <gedit.h>
#include <ghostmvr.h>

#include <rooms.h>
#include <psnd.h>
#include <psndapi.h>

#include <schema.h>
#include <schbase.h>

#include <mprintf.h>

// Must be last header
#include <dbmem.h>


#ifndef SHIP
////////////////////////////////////////////////////////////////////////////////

#define SOUND_SCHEMA  -372

void MakeSound(mxs_real volume)
{
   mxs_vector zero;

   mx_zero_vec(&zero);

#if 0
   SchemaIDPlayLoc(SOUND_SCHEMA, &zero);
#endif

   sfx_parm parms = {0, 0, 0, 0, 0, 100, 0, 0, 0, NULL, NULL};
   g_pPropSnd->GenerateSound(5, OBJ_NULL, "ai_fire1", 1.0, &parms, TRUE);
}

void MeMakeSound(mxs_real volume)
{
   SchemaIDPlayObj(SOUND_SCHEMA, PlayerObject());
}

////////////////////////////////////////////////////////////////////////////////

void SpewRoomAI()
{
   const cRoom *room;
   const ObjID *obj_list;
   int   list_size;

   for (int i=0; i<g_pRooms->GetNumRooms(); i++)
   {
      room = g_pRooms->GetRoom(i);
      room->GetObjList(&obj_list, &list_size, g_SndWatchHandle);
      while (list_size--)
         mprintf(" [room %d (obj %d)]  AI obj %d\n", room->GetRoomID(), room->GetObjID(), obj_list[list_size]);
   }
}

////////////////////////////////////////////////////////////////////////////////

void DrawSoundPath(char *args)
{
#ifdef EDITOR
   ObjID src_obj;
   Label schema_name;
   int radius;

   gedDeleteChannelLines(1<<LINE_CH_SOUND);
   ged_line_view_channels |= (1<<LINE_CH_SOUND);

   if (strstr(args, "goat"))
      sscanf(args, "%dgoat%dgoat%s", &src_obj, &radius, &schema_name);
   else
      sscanf(args, "%d,%d,%s", &src_obj, &radius, &schema_name);

   mprintf("playing %s on %d and drawing path (radius %d)\n", &schema_name, src_obj, radius);

   sSchemaCallParams callParams = g_sDefaultSchemaCallParams;
   callParams.flags |= SCH_SET_OBJ | SCH_RADIUS_VOLUME;
   callParams.volume = radius;
   callParams.sourceID = src_obj;

   g_pPropSnd->SetDrawSoundPath(TRUE);
   SchemaPlay(&schema_name, &callParams);
   g_pPropSnd->SetDrawSoundPath(FALSE);

   gedit_full_redraw();
#else
   mprintf("This command does not work without an editor build!\n");
#endif
}

void ClearSoundPath()
{
#ifdef EDITOR
   gedDeleteChannelLines(1<<LINE_CH_SOUND);
   ged_line_view_channels &= ~(1<<LINE_CH_SOUND);

   gedit_full_redraw();
#endif
}

void DrawSounds()
{
   g_pPropSnd->DrawActiveSounds();
}   

void SpewSounds()
{
   g_pPropSnd->SpewActiveSounds();
}

////////////////////////////////////////////////////////////////////////////////


BOOL g_show_sounds = FALSE;

static Command g_RoomCommands[] =
{
   {"make_sound_z",    FUNC_FLOAT,  MakeSound,         "Generate a sound at 0,0,0",        HK_ALL},
   {"make_sound" ,     FUNC_FLOAT,  MeMakeSound,       "Generate a sound at the player",   HK_ALL},
   {"spew_room_ai",    FUNC_VOID,   SpewRoomAI,        "List each AI's current room",      HK_ALL},
   {"show_sounds",     TOGGLE_BOOL, &g_show_sounds,    "Toggles visible sound display",    HK_ALL},
   {"draw_sound_path", FUNC_STRING, DrawSoundPath,     "Draw the sound prop path",         HK_ALL},
   {"clear_sound_path",FUNC_VOID,   ClearSoundPath,    "Clears any drawn sound paths",     HK_ALL},
   {"spew_sounds",     FUNC_VOID,   SpewSounds,        "Spews all propagating sounds",     HK_ALL},
};
#endif

////////////////////////////////////////////////////////////////////////////////

static IProperty *_pAIProp = NULL;

BOOL IsSoundListener(ObjID objID)
{
   return ((PlayerObjectExists() && objID==PlayerObject()) || _pAIProp->IsRelevant(objID) ||
           ((GetGhostType(objID) & kGhostListenTypes) == kGhostListenTypes));
}

////////////////////////////////////////

void _SoundProp_Setup(void)
{
   g_pPropSnd = new cPropSnd;
   g_pRooms->AddWatch(IsSoundListener, &g_SndWatchHandle);
}

////////////////////////////////////////

void _SoundProp_Clear(void)
{
   if (g_pRooms && (g_SndWatchHandle != -1))
      g_pRooms->RemoveWatch(g_SndWatchHandle);

   delete g_pPropSnd;
   g_pPropSnd = NULL;

}

////////////////////////////////////////

void SoundPropagationInit(void)
{
   _SoundProp_Setup();
   _pAIProp = GetPropertyNamed("AI");

   #ifndef SHIP
   COMMANDS(g_RoomCommands, HK_ALL);
   #endif
}

void SoundPropagationTerm(void)
{
   _SoundProp_Clear();
   SafeRelease(_pAIProp);
}

void SoundPropagationReset(void)
{
   _SoundProp_Clear();
   _SoundProp_Setup();
}

////////////////////////////////////////////////////////////////////////////////

int GenerateSoundObj(ObjID objID, ObjID schemaID, const char *sampleName, float atten_factor, 
                     sfx_parm *parms, int flags, void *data)
{
   return g_pPropSnd->GenerateSound(objID, schemaID, sampleName, atten_factor, parms, flags, data);
}

////////////////////////////////////////

int  GenerateSoundVec(mxs_vector *vec, ObjID src_obj, ObjID schemaID, const char *sampleName, float atten_factor, 
                      sfx_parm *parms, int flags, void *data)
{
   return g_pPropSnd->GenerateSound(*vec, src_obj, schemaID, sampleName, atten_factor, parms, flags, data);
}

////////////////////////////////////////

int  GenerateSound(const char *sampleName, sfx_parm *parms)
{
   return g_pPropSnd->GenerateSound(sampleName, parms);
}

////////////////////////////////////////

void SoundHalt(int handle)
{
   g_pPropSnd->SoundHalt(handle);
}

////////////////////////////////////////

void SoundHaltObj(ObjID objID)
{
   g_pPropSnd->SoundHaltObj(objID);
}

////////////////////////////////////////////////////////////////////////////////

void ApplyPropSounds()
{
   AssertMsg(g_pPropSnd != NULL, "Attempt to apply sounds of unitialized propagation system");

   g_pPropSnd->ApplySoundInfo();
}

////////////////////////////////////////

void ClearPropSounds()
{
   AssertMsg(g_pPropSnd != NULL, "Attempt to clear sounds of unitialized propagation system");

   g_pPropSnd->ClearSoundInfo();
}

////////////////////////////////////////

void CleanupSoundObjects()
{
   AssertMsg(g_pPropSnd != NULL, "Attempt to clean up sound objects of unitialized propagation system");

   g_pPropSnd->Cleanup();
}
   
////////////////////////////////////////////////////////////////////////////////
