/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/roomsys.h,v 1.7 2000/02/18 13:03:14 ccarollo Exp $
//
// Room System main header
//
#pragma once

#ifndef __ROOMSYS_H
#define __ROOMSYS_H


typedef int ObjID;

////////////////////////
//
// Epsilons
//
#define ON_PLANE_EPSILON      0.0001
#define SAME_POINT_EPSILON_2  0.0001

////////////////////////
//
// Functions
//
typedef void (*RoomReadWrite) (void *buf, size_t elsize, size_t nelem);

////////////////////////
//
// Forward-declarations
//
typedef struct tOBB tOBB;
typedef struct tPlane tPlane;
typedef struct tEdge tEdge;

#ifdef __cplusplus
class cRoomPortal;
class cRoom;
class cRooms;
#endif

////////////////////////
//
// Public functions
//
EXTERN void InitRooms(void);
EXTERN void TermRooms(void);
EXTERN void ClearRooms(void);

EXTERN void LoadRooms(RoomReadWrite movefunc);
EXTERN void SaveRooms(RoomReadWrite movefunc);
EXTERN void PostLoadRooms(void);
EXTERN void InitObjRooms(void);

EXTERN BOOL ConvertRoomBrushes(void);
EXTERN void DrawRoom(ObjID objID);
EXTERN void DoRoomTest(ObjID objID);

#ifdef __cplusplus
EXTERN cRooms *g_pRooms;
#endif

#endif
