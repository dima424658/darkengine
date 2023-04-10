/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/playrobj.h,v 1.9 2000/01/31 10:00:16 adurant Exp $
#pragma once

#ifndef PLAYROBJ_H
#define PLAYROBJ_H
#include <objtype.h>
#include <comtools.h>

////////////////////////////////////////////////////////////
// THE PLAYER
//
// This is support for the single object that represents the 
// player playing at the console.  It may also support 
// some player-specific state data.
//
//


//
// Get the player's ObjID
//

EXTERN ObjID gPlayerObj;

#ifdef DBG_ON
EXTERN ObjID get_player_object(void);
#define PlayerObject() get_player_object()
#else
#define PlayerObject() gPlayerObj
#endif 


//
// Get the player's camera
//
typedef struct Camera PlayerObjCamera;

#ifdef DBG_ON
EXTERN PlayerObjCamera* get_player_camera(void);
#define PlayerCamera() get_player_camera()
#else
EXTERN PlayerObjCamera* gPlayerCam;
#define PlayerCamera() gPlayerCam
#endif 

//
// Is this obj either PlayerObject or any network player, if this is a networked game.
//
EXTERN BOOL IsAPlayer(ObjID obj);

//
// Get the symbolic name used by the player object
//
EXTERN const char* PlayerObjName(void);

//
// Create the player object explicitly
//

EXTERN void PlayerCreate(void); 
EXTERN void PlayerDestroy(void); 

//
// Test to see whether an object is a player
//
#define IsPlayerObj(obj) ((obj) != OBJ_NULL && (obj) == gPlayerObj)

//
// Test to see if the player object exists
//
#define PlayerObjectExists() (gPlayerObj != OBJ_NULL)


//
// Check to see whether this (readable) database file contains the player
// 
F_DECLARE_INTERFACE(ITagFile); 
EXTERN BOOL ContainsPlayerTag(ITagFile* file); 


//
// Install a callback on creation of the player, so that you can instantiate
// game-specific properties and stuff.
// 

enum ePlayerEvent
{
   kPlayerCreate,
   kPlayerDestroy, 
   kPlayerLoad, 
};

typedef enum ePlayerEvent ePlayerEvent;
typedef void (*PlayerCreateFunc)(ePlayerEvent event, ObjID player);


EXTERN void HookPlayerCreate(PlayerCreateFunc func);

//
// This callback is used to determine the "factory object" used to create
// the player.  If the "factory object" has an outgoing "playerfactory" link,
// the linked object is used as the archetype for creating the player. 
//

typedef ObjID (*PlayerFactoryObjFunc)(void); 
EXTERN void HookPlayerFactory(PlayerFactoryObjFunc func); 

#endif // PLAYROBJ_H





