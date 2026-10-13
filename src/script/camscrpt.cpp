/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <lg.h>
#include <scrptapi.h>
#include <scrptsrv.h>
#include <scrptbas.h>
#include <scrptmsg.h>
#include <objscrt.h>
#include <osystype.h>
#include <camera.h>
#include <camscrs.h>
#include <playrobj.h>

#include <rendprop.h> //to unrender playerarm
#include <plycbllm.h> //to get playerarm
#include <command.h> //to execute clear_weapon command
#include <matrix.h>

#define CAMERA_ANG_TO_DEGREES(x) (((mxs_real)(x) * 180) / MX_ANG_PI)

// must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// Camera script service 
//


DECLARE_SCRIPT_SERVICE_IMPL(cCameraSrv,Camera)
{
public:

   STDMETHOD(StaticAttach)(object attachee)
      {
	 ObjID plyrobj = PlayerObject();
	 ObjID plyrarm = PlayerArm();
#ifdef THIEF
//other games don't have the "clear_weapon" command, and making
//it not render without unequipping would be very bad.
	 if (plyrarm)
	   ObjSetRenderType(plyrarm,kRenderNotAtAll);	 
	 CommandExecute("clear_weapon");
#endif
	 Camera *playercam = PlayerCamera();
	 CameraView(playercam, attachee);
         return S_OK; 
      }

   STDMETHOD(DynamicAttach)(object attachee)
      {
	 ObjID plyrobj = PlayerObject();
	 ObjID plyrarm = PlayerArm();
#ifdef THIEF
//other games don't have the "clear_weapon" command, and making
//it not render without unequipping would be very bad.
	 if (plyrarm)
	   ObjSetRenderType(plyrarm,kRenderNotAtAll);	 
	 CommandExecute("clear_weapon");
#endif
	 Camera *playercam = PlayerCamera();
	 CameraRemote(playercam, attachee);
         return S_OK; 
      }

   STDMETHOD(CameraReturn)(object attachee)
      {
	 Camera *playercam = PlayerCamera();
	 object actualattach = CameraGetObjID(playercam);
	 if (attachee == actualattach)
	   {
	     CameraAttach(playercam, PlayerObject());
             return S_OK; 
	   }
	 else
	   return S_FALSE;
      }

   STDMETHOD(ForceCameraReturn)()
     {
       Camera *playercam = PlayerCamera();
       CameraAttach(playercam,PlayerObject());
       return S_OK;
     }

   STDMETHOD_(object, GetCameraParent)()
     {
       return object(CameraGetObjID(PlayerCamera()));
     }

   STDMETHOD_(BOOL, IsRemote)()
     {
       return CameraGetObjID(PlayerCamera()) != PlayerObject();
     }

   STDMETHOD_(vector, GetPosition)()
     {
       mxs_vector position;
       CameraGetLocation(PlayerCamera(), &position, NULL);
       return vector(position);
     }

   STDMETHOD_(vector, GetFacing)()
     {
       mxs_angvec facing;
       CameraGetLocation(PlayerCamera(), NULL, &facing);
       return vector(CAMERA_ANG_TO_DEGREES(facing.tx),
                     CAMERA_ANG_TO_DEGREES(facing.ty),
                     CAMERA_ANG_TO_DEGREES(facing.tz));
     }

   STDMETHOD_(vector, CameraToWorld)(const vector &local_pos)
     {
       mxs_vector position, transformed;
       mxs_angvec facing;
       mxs_matrix rotation;
       CameraGetLocation(PlayerCamera(), &position, &facing);
       mx_ang2mat(&rotation, &facing);
       mx_mat_mul_vec(&transformed, &rotation, &local_pos);
       mx_addeq_vec(&transformed, &position);
       return vector(transformed);
     }

   STDMETHOD_(vector, WorldToCamera)(const vector &world_pos)
     {
       mxs_vector position, relative, transformed;
       mxs_angvec facing;
       mxs_matrix rotation;
       CameraGetLocation(PlayerCamera(), &position, &facing);
       mx_sub_vec(&relative, &world_pos, &position);
       mx_ang2mat(&rotation, &facing);
       mx_mat_tmul_vec(&transformed, &rotation, &relative);
       return vector(transformed);
     }

   STDMETHOD(LockMovement)(BOOL)
     {
       // The original engine has no separate camera-input lock.  Preserve the
       // call ABI; callers can still attach the camera to obtain a fixed view.
       return S_OK;
     }

};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cCameraSrv, Camera); 

