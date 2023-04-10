/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <dpccmobj.h>

#include <appagg.h>
#include <aggmemb.h>
#include <memall.h>

#include <matrix.h>

#include <propbase.h>

#include <camera.h>
#include <objpos.h>
#include <playrobj.h>

#include <dpccobpr.h>

// Include these absolutely last
#include <dbmem.h>
#include <initguid.h>

DEFINE_LG_GUID(IID_ICameraObjects, 0x243);

class cCameraObjects:
   public cCTDelegating<ICameraObjects>, 
   public cCTAggregateMemberControl<kCTU_Default>
{
public:
   cCameraObjects(IUnknown* pOuter)
   {
      MI_INIT_AGGREGATION_1(pOuter, ICameraObjects, kPriorityNormal, NULL);
   }
   
   STDMETHOD(Frame)(tSimTime deltaTime);

private:
   void UpdatePosition(ObjID objID);
};

////////////////////////////////////////

void cCameraObjects::UpdatePosition(ObjID objID)
{
   cCameraObj *pCameraObj;
   mxs_vector camPos;
   mxs_angvec camFacing;
   mxs_vector loc;
   mxs_matrix m1;
   mxs_vector offset;
   mxs_angvec facing;

   g_CameraObjProperty->Get(objID, &pCameraObj);
   Assert_(pCameraObj != NULL);

   // Get base location/facing
   CameraGetLocation(PlayerCamera(), &camPos, &camFacing);
   // turn player facing into matrix
   mx_ang2mat(&m1, &camFacing);

   // add offset
   mx_mat_mul_vec(&offset, &m1, &pCameraObj->m_offset);
   mx_add_vec(&loc, &camPos, &offset);

   if ((!pCameraObj->m_lockang[1]) && (pCameraObj->m_lockang[2]))
   {
      mxs_matrix m2;
      mx_rot_z_mat(&m2, &m1, -camFacing.tz+pCameraObj->m_angle.tz);
      mx_mat2ang(&facing, &m2);
   }
   else
   {
      facing.tx = pCameraObj->m_angle.tx;
      if (!pCameraObj->m_lockang[0])
         facing.tx += camFacing.tx;

      facing.ty = pCameraObj->m_angle.ty;
      if (!pCameraObj->m_lockang[1])
         facing.ty += camFacing.ty;
      
      facing.tz = pCameraObj->m_angle.tz;
      if (!pCameraObj->m_lockang[2])
         facing.tz += camFacing.tz;
   }


   // update actual obj pos
   ObjPosUpdate(objID, &loc, &facing);
}

////////////////////////////////////////

STDMETHODIMP cCameraObjects::Frame(tSimTime timeDelta)
{
   sPropertyObjIter iter;
   ObjID objID;

   g_CameraObjProperty->IterStart(&iter);
   while (g_CameraObjProperty->IterNext(&iter, &objID))
      UpdatePosition(objID);
   g_CameraObjProperty->IterStop(&iter);
   return S_OK;
}

////////////////////////////////////////

void DPCCameraObjectsCreate(void)
{
   AutoAppIPtr(Unknown); 
   new cCameraObjects(pUnknown); 
}


