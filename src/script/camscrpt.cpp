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

};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cCameraSrv, Camera); 

