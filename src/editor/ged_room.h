/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/ged_room.h,v 1.8 2000/01/29 13:12:15 adurant Exp $
#pragma once

#ifndef __GED_ROOM_H
#define __GED_ROOM_H


#define ROOT_ROOM_NAME     "Base Room"
#define DEFAULT_ROOM_NAME  "Default Room"


// is room database up-to-date?
EXTERN BOOL g_RoomsOK;

// Use to compile rooms and set status flags
EXTERN void ged_compile_rooms(void);

// Get the object id of a room named
EXTERN ObjID ged_get_room_named(const char *name);

// Create a new room object
EXTERN ObjID ged_create_room_obj(const char *name, ObjID archetype, BOOL concrete);

// Make an object a room object
EXTERN void ged_make_room_obj(ObjID objID);

// Get the root and default room ids (creates if necessary)
EXTERN ObjID ged_room_get_root(void);
EXTERN ObjID ged_room_get_default(void);

// Is the object a room?
EXTERN BOOL ged_is_room(ObjID objID);

EXTERN void  ged_room_set_default_arch(ObjID objID);
EXTERN ObjID ged_room_get_default_arch(void);

// Get the object's parent's id
EXTERN ObjID ged_room_get_parent(ObjID objID);

// Get the depth of the object in the room subtree (-1 if fails)
EXTERN int ged_room_get_depth(ObjID objID);

// Draw room-portal connection lines for specified room
EXTERN void ged_room_draw_connections(short roomID, BOOL clear);

// Draw room-portal connection lines for all rooms (or turn off)
EXTERN void ged_room_draw_all_connections(BOOL state);

EXTERN void ged_room_init(void); 

// post-load rooms (also do this on build default) 
EXTERN void ged_room_postload(void);


#endif  // __GED_ROOM_H

