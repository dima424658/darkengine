/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/camera.c,v 1.35 2000/02/19 12:35:41 toml Exp $

// camera stuff

#include <stdlib.h>

#include <matrix.h>
#include <objshape.h>
#include <wrtype.h>
#include <objpos.h>
#include <playrobj.h>
#include <physapi.h>
#include <phmoapi.h>
#include <camera.h>
#include <config.h>
#include <plyrmode.h>
#include <rendprop.h>
#include <mnamprop.h> //for playermodelset

#ifdef THIEF 
#include <drkpwups.h> //for flash effect
#endif 

#include <mprintf.h>

//shock hack (gen_bind.cpp)  I'm using it here because it's useful.
extern BOOL gNoMoveKeys;

mxs_vector CameraPosOffsets[MAX_CAMERA_MODE] = { { 0,0,0 }, { -5,0,3 }, { 0,0,0 }, { 0,0,0 }, { 0,0,0 }, {0,0,0} , {0,0,0} };
mxs_angvec CameraAngOffsets[MAX_CAMERA_MODE] = { { 0,0,0 }, { 0,2048,0 }, { 0,0,0 }, { 0,0,0 }, { 0,0,0 }, {0,0,0}, {0,0,0} };

// call this when changing camera mode and you need to do things like
// block/unblock movement etc (remote eye goodness)
static void CameraSwitchProcess(Camera *cam, BOOL DoFlash, ObjID newobj)
{
	  
  char buf[80];

  if (cam == PlayerCamera())
    {
#ifdef THIEF
      if (DoFlash) FlashOnlyPlayer(1.0);
#endif 
      gNoMoveKeys = (cam->mode != FIRST_PERSON);
      if (gNoMoveKeys)
	{
	  //set player model
	  if (config_get_raw("player_model",buf,sizeof(buf)))
	    {
	      buf[sizeof(buf)-1] = '\0';
	      ObjSetModelName(PlayerObject(),buf);
	    }
	  if ((GetPlayerMode()==kPM_Crouch) && (config_get_raw("player_crouch_model",buf,sizeof(buf))))
	    {
	      buf[sizeof(buf)-1] = '\0';
	      ObjSetModelName(PlayerObject(),buf);
	    }
	  ObjSetHasRefs(newobj,FALSE);
	  SetForwardState(0);
	  UpdatePlayerSpeed();
	}
    }
}

//
// allocate a camera and set it to a position, angle, and optional objid
//
Camera* CameraInit(mxs_vector *pos, mxs_angvec *ang, ObjID objid)
{
   Camera *cam;
   int cnt, cfg_vals[6];

   if ((cam = (Camera *)Malloc(sizeof(Camera))) != NULL)
   {
      cam->mode = FIRST_PERSON;
      cam->zoom = 1.0;
      cam->pos = *pos;
      cam->ang = *ang;
      cam->ang_off.tx = 0;
      cam->ang_off.ty = 0;
      cam->ang_off.tz = 0;
      cam->objid = objid;
   }

   if ((cam->mode == FIRST_PERSON) && PhysObjHasPhysics(cam->objid))
      PhysSetSubModRotation(cam->objid, PLAYER_HEAD, &cam->ang_off);

   cnt = 6;
   config_get_value("user_camera_offset",CONFIG_INT_TYPE,(config_valptr)cfg_vals,&cnt);
   switch (cnt)
   {
      case 6: CameraAngOffsets[USER_DEFINED].tz = cfg_vals[5];
      case 5: CameraAngOffsets[USER_DEFINED].ty = cfg_vals[4];
      case 4: CameraAngOffsets[USER_DEFINED].tx = cfg_vals[3];
      case 3: CameraPosOffsets[USER_DEFINED].z = cfg_vals[2];
      case 2: CameraPosOffsets[USER_DEFINED].y = cfg_vals[1];
      case 1: CameraPosOffsets[USER_DEFINED].x = cfg_vals[0];
   }

   return cam;
}

//
// free the camera
//
void CameraDestroy(Camera *cam)
{
   if (cam != NULL)
   {
      Free(cam);
      cam = NULL;
   }
}

//
// detach the camera from the current object and use the current loc and ang
//
void CameraDetach(Camera *cam)
{
   if (cam != NULL)
   {
      cam->objid = 0;
      cam->mode = DETACHED;
   }
}

//
// reattach camera to current object.  Verifies that player can/can't
// move, but doesn't do flash.
//

void CameraReAttach(Camera *cam)
{
  if (cam->objid == OBJ_NULL)
    return;
  CameraSwitchProcess(cam,FALSE,cam->objid);
}

//
// attach the camera to an object
//
void CameraAttach(Camera *cam, ObjID objid)
{
   BOOL DoFlash = FALSE;
   if (cam != NULL)
   {
      if (cam->objid != OBJ_NULL)
	ObjSetHasRefs(cam->objid, TRUE);
      cam->objid = objid;
      if (objid == PlayerObject())
	{
	  if ((cam->mode == REMOTE_CAM) || (cam->mode == VIEW_CAM))
	    DoFlash=TRUE;
	  //this flash helps keep player from seeing efficiency mode changes.
	  cam->mode = FIRST_PERSON;
	  CameraSwitchProcess(cam,DoFlash,objid);
	}
      else
         cam->mode = OBJ_ATTACH; 


   }
}


//
// attach the camera to an object as a "remote viewing camera"
// (no freelook, but player can't move either)
//
void CameraView(Camera *cam, ObjID objid)
{
  if (cam != NULL)
    {
      if (cam->objid != OBJ_NULL)
	ObjSetHasRefs(cam->objid, TRUE);
      cam->objid = objid;
      if (objid == PlayerObject())
	{
	  Warning(("Remote attaching view camera to player object.  Should use normal CameraAttach for this.  Setting mode to FIRST_PERSON.\n"));
	  cam->mode = FIRST_PERSON;
	  CameraSwitchProcess(cam,TRUE,objid);
	}
      else
      if (PhysObjHasPhysics(objid))
	{
	  cam->mode = VIEW_CAM;
	  CameraSwitchProcess(cam,TRUE,objid);
	}
      else
	{
	  Warning(("Remote attach should only attach to physical objects.  Making it OBJ_ATTACH instead.\n"));
	  cam->mode=OBJ_ATTACH;
	}
    }
}

//
// attach the camera to an object as a "remote camera" 
// (angle tracks player freelook, player can't move)
//
void CameraRemote(Camera *cam, ObjID objid)
{
  if (cam != NULL)
    {
      if (cam->objid != OBJ_NULL)
	ObjSetHasRefs(cam->objid, TRUE);
      cam->objid = objid;
      if (objid == PlayerObject())
	{
	  Warning(("Remote attaching camera to player object.  Should use normal CameraAttach for this.  Setting mode to FIRST_PERSON.\n"));
	  cam->mode = FIRST_PERSON;
	  CameraSwitchProcess(cam,TRUE,objid);
	}
      else
      if (PhysObjHasPhysics(objid))
	{
	  cam->mode = REMOTE_CAM;
	  CameraSwitchProcess(cam,TRUE,objid);
	}
      else
	{
	  Warning(("Remote attach should only attach to physical objects.  Making it OBJ_ATTACH instead.\n"));
	  cam->mode=OBJ_ATTACH;
	}      

    }
}

//
// set the camera location (you should not be attached to an object or this function will
// have not effect)
// pass a NULL if you don't want to set a parameter
//
void CameraSetLocation(Camera *cam, mxs_vector *pos, mxs_angvec *ang)
{
   if (cam != NULL)
   {
      if (pos != NULL)
         cam->pos = *pos;
      if (ang != NULL)
         cam->ang = *ang;
   }
}

//
// get the camera location (NULL if you don't want a parm)
//
void CameraGetLocation(Camera *cam, mxs_vector *pos, mxs_angvec *ang)
{
   if (cam != NULL)
   {
      if (pos != NULL)
         *pos = cam->pos;
      if (ang != NULL)
         *ang = cam->ang;
   }
}

//
// set the camera angular offset
//
void CameraSetAngOffset(Camera *cam, mxs_angvec *ang)
{
   if ((cam != NULL) && (ang != NULL))
   {
      if ((cam->mode == FIRST_PERSON) && PhysObjHasPhysics(cam->objid))
         PhysSetSubModRotation(cam->objid, PLAYER_HEAD, ang);
      else
      if ((cam->mode == REMOTE_CAM) && (PhysObjHasPhysics(cam->objid)))
	{
	  PhysSetModRotation(cam->objid,ang);//fix targ obj.
	}
      else
	{
	  cam->ang_off = *ang;
	}
   }
}

//
// get the camera angular offset
//
void CameraGetAngOffset(Camera *cam, mxs_angvec *ang)
{
   if ((cam != NULL) && (ang != NULL))
   {
      if ((cam->mode == FIRST_PERSON) && PhysObjHasPhysics(cam->objid))
         PhysGetSubModRotation(cam->objid, PLAYER_HEAD, ang);
      else
      if ((cam->mode == REMOTE_CAM) && (PhysObjHasPhysics(cam->objid)))
	{
	  PhysGetModRotation(cam->objid, ang);
	}
      else
         *ang = cam->ang_off;
   }
}

//
// update the position to follow the current objid
//

void CameraUpdate(Camera *cam)
{
   mxs_matrix mat;
   mxs_angvec mouselook_offset;

   if (cam != NULL)
   {
      if (CameraGetObjID(cam) != 0)
      {
         if ((cam->mode == FIRST_PERSON) && PhysObjHasPhysics(cam->objid))
         {
            PhysGetSubModLocation(cam->objid, 0, &cam->pos);

            PhysGetModRotation(cam->objid, &cam->ang);
            PhysGetSubModRotation(cam->objid, PLAYER_HEAD, &mouselook_offset);

            PhysAdjustPlayerHead(&cam->pos, &cam->ang);
         }
	 else
	 if ((cam->mode == REMOTE_CAM) && (cam->objid != OBJ_NULL) && (PhysObjHasPhysics(cam->objid)))
	   {
	     ObjPos* pos = ObjPosGet(cam->objid); //get our attached obj pos.
	     if (pos) 
	       {
		 cam->pos = pos->loc.vec;
		 cam->ang = pos->fac;
	       }
	   }
	 else
	 if ((cam->mode == VIEW_CAM) && (cam->objid != OBJ_NULL) && (PhysObjHasPhysics(cam->objid)))
	   {
	     ObjPos* pos = ObjPosGet(cam->objid); //get our attached obj pos.
	     if (pos) 
	       {
		 cam->pos = pos->loc.vec;
		 cam->ang = pos->fac;
	       }
	   }
         else
         if ((cam->mode == TRACKING) && (cam->objid != OBJ_NULL))
         {
            mxs_vector x_axis;
            mxs_vector y_axis;
            mxs_vector z_axis;

            mx_sub_vec(&x_axis, &ObjPosGet(cam->objid)->loc.vec, &cam->pos);
            if (mx_mag2_vec(&x_axis) > 0)
               mx_normeq_vec(&x_axis);

            mx_unit_vec(&z_axis, 2);

            mx_cross_vec(&y_axis, &z_axis, &x_axis);
            mx_cross_vec(&z_axis, &x_axis, &y_axis);

            if ((mx_mag2_vec(&y_axis) > 0) && (mx_mag2_vec(&z_axis) > 0))
            {
               mxs_matrix rotation;
           
               rotation.vec[0] = x_axis;
               rotation.vec[1] = y_axis;
               rotation.vec[2] = z_axis;

               mx_mat2ang(&cam->ang, &rotation);
            }
         }
         else
         if (cam->mode != DETACHED)
         {
            ObjPos* pos = ObjPosGet(cam->objid); 
            if (pos)
            {
               cam->pos = pos->loc.vec;
               cam->ang = pos->fac;
            }
         }

         if ((cam->mode != FIRST_PERSON) && (cam->mode != REMOTE_CAM) && (cam->mode != VIEW_CAM))
         {
            // add in the camera offset
            mx_ang2mat(&mat, &cam->ang);
            mx_scale_addeq_vec(&cam->pos, &mat.vec[0], CameraPosOffsets[cam->mode].x);
            mx_scale_addeq_vec(&cam->pos, &mat.vec[1], CameraPosOffsets[cam->mode].y);
            mx_scale_addeq_vec(&cam->pos, &mat.vec[2], CameraPosOffsets[cam->mode].z);
            cam->ang.tx += CameraAngOffsets[cam->mode].tx;
            cam->ang.ty += CameraAngOffsets[cam->mode].ty;
            cam->ang.tz += CameraAngOffsets[cam->mode].tz;
         }

         // add in the mouselook offset
         CameraGetAngOffset(cam, &mouselook_offset);

         cam->ang.tx += mouselook_offset.tx;
         cam->ang.ty += mouselook_offset.ty;
         cam->ang.tz += mouselook_offset.tz;
      }
   }
}

//
// cycle the camera mode
//
void CameraNextMode(Camera *cam)
{
   if (cam != NULL)
      if (++cam->mode >= MAX_CAMERA_MODE)
	cam->mode = FIRST_PERSON;

}

//
// set the camera mode
//
void CameraSetMode(Camera *cam, eCameraMode mode)
{
   if (mode >= MAX_CAMERA_MODE)
   {
      Warning(("Attempt to set invalid camera mode\n"));
      return;
   }

   cam->mode = mode;
}

//
// get the camera mode
//
eCameraMode CameraGetMode(Camera *cam)
{
   return cam->mode;
}

//
// get the objid that the camera is attached to
//
ObjID CameraGetObjID(Camera *cam)
{
   if (cam != NULL)
//      if (cam->mode==FIRST_PERSON)
         return cam->objid;
   return 0;
}

//////////////////////////////////////
// save/load system to get accurate camera positions saved to disk

#include <stdio.h>
#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <dump.h>

#include <status.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

void CameraSave(Camera *cam, char *str)
{
   char name_buf[30];
   int fh;
   
   if ((str==NULL)||(str[0]=='\0'))
   {
      fh=dmp_find_free_file(name_buf,"view","cam");
      str=name_buf;
   }
   else
   {
      // add .cam?
      fh=open(str,O_BINARY|O_WRONLY|O_CREAT,S_IREAD|S_IWRITE);
   }
   if (fh==-1)
      Status("Couldnt Open File");
   else
   {
      mxs_vector pos;
      mxs_angvec ang;
      char buf[256];

      CameraGetLocation(cam, &pos, &ang);
      sprintf(buf,"%x %x %x %x %x %x",
              *((int *)&pos.x),*((int *)&pos.y),*((int *)&pos.z),(int)ang.el[0],(int)ang.el[1],(int)ang.el[2]);
      write(fh,buf,strlen(buf));
      close(fh);
      sprintf(buf,"Wrote %s",str);
      Status(buf);
   }
}

// should learn about loading from null string, ie. findfirst/next through *.cam
void CameraLoad(Camera *cam, char *str)
{
   int fh;

   // add .cam?   
   fh=open(str,O_BINARY|O_RDONLY);
   if (fh==-1)
      Status("Cant open file");
   else
   {
      mxs_vector pos;
      mxs_angvec ang;
      char buf[256];
      int p,h,b;
      
      read(fh,buf,255); // bet it isnt this long, i hope
      sscanf(buf,"%x %x %x %x %x %x",               // im not sure these are really phb, but there are 3
                   (int *)&pos.x,(int *)&pos.y,(int *)&pos.z,&p,&h,&b);  
      ang.el[0]=p; ang.el[1]=h; ang.el[2]=b;
      close(fh);
      CameraDetach(cam);
      CameraSetLocation(cam, &pos, &ang);
      Status("Read in, Detached Camera");
   }
}

BOOL CameraIsRemote(Camera *cam)
{
  if ((cam->mode == REMOTE_CAM) || (cam->mode == VIEW_CAM))
    return TRUE;
  return FALSE;
}








