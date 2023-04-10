/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// camera stuff
#pragma once

#ifndef __CAMERA_H
#define __CAMERA_H

#include <matrixs.h>
#include <objtype.h>

typedef enum eCameraMode
{
   FIRST_PERSON,
   THIRD_PERSON,
   OBJ_ATTACH,
   REMOTE_CAM, //like obj_attach, but w/ freelook, and player can't move.
   VIEW_CAM, //like remote_cam, but w/o freelook.  Player can't move.
   DETACHED,
   TRACKING,
   USER_DEFINED,
   MAX_CAMERA_MODE
} eCameraMode;

typedef struct Camera 
{
   eCameraMode mode;
   mxs_real    zoom;
   mxs_vector  pos;
   mxs_angvec  ang;
   mxs_angvec  ang_off;    // head offset from ang
   ObjID       objid;      // 0 for using vector/angle, not 0 for following object
} Camera;

EXTERN Camera* CameraInit(mxs_vector *pos, mxs_angvec *ang, ObjID objid);
EXTERN void    CameraDestroy(Camera *cam);
EXTERN void    CameraDetach(Camera *cam);
EXTERN void    CameraReAttach(Camera *cam);
EXTERN void    CameraAttach(Camera *cam, ObjID objid);
EXTERN void    CameraView(Camera *cam, ObjID objid);
EXTERN void    CameraRemote(Camera *cam, ObjID objid);
EXTERN void    CameraSetLocation(Camera *cam, mxs_vector *pos, mxs_angvec *ang);
EXTERN void    CameraGetLocation(Camera *cam, mxs_vector *pos, mxs_angvec *ang);
EXTERN void    CameraSetAngOffset(Camera *cam, mxs_angvec *ang);
EXTERN void    CameraGetAngOffset(Camera *cam, mxs_angvec *ang);
EXTERN void    CameraUpdate(Camera *cam);
EXTERN void    CameraNextMode(Camera *cam);
EXTERN void    CameraSetMode(Camera *cam, eCameraMode mode);
EXTERN eCameraMode CameraGetMode(Camera *cam); 
EXTERN ObjID   CameraGetObjID(Camera *cam);
EXTERN BOOL    CameraIsRemote(Camera *cam);

/////////// attempted disk save/load setup
EXTERN void    CameraSave(Camera *cam, char *str);
EXTERN void    CameraLoad(Camera *cam, char *str);

#endif
