/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/gametool.cpp,v 1.9 1999/11/02 15:57:32 BODISAFA Exp $

#include <gametool.h>
#include <appagg.h>
#include <aggmemb.h>

#include <wrtype.h>
#include <osystype.h>
#include <objpos.h>
#include <matrix.h>
#include <physapi.h>
#include <rooms.h>
#include <roomsys.h>

#include <creatext.h>

#include <phnet.h>

#ifdef EDITOR
#include <editobj.h>
#endif 

// Include these absolutely last
#include <dbmem.h>
#include <initguid.h>
#include <gtooliid.h>

#include <linkman.h>
#include <linkbase.h>
#include <relation.h>
#include <lnkquery.h>

static void CleanLinks(ObjID obj, RelationID relid)
{
   ILinkQuery *pQuery;
   AutoAppIPtr(LinkManager);
   sLink l;

   pQuery = pLinkManager->Query(obj, LINKOBJ_WILDCARD, relid);

   while (!pQuery->Done())
   {
      pQuery->Link(&l);
      pLinkManager->Remove(pQuery->ID()); 
      pQuery->Next();
   }
   SafeRelease(pQuery);

   pQuery = pLinkManager->Query(LINKOBJ_WILDCARD, obj, relid);

   while (!pQuery->Done())
   {
      pQuery->Link(&l);
      pLinkManager->Remove(pQuery->ID()); 
      pQuery->Next();
   }
   SafeRelease(pQuery);
}

class cGameTools : public cCTDelegating<IGameTools>,
                   public cCTAggregateMemberControl<kCTU_Default>
{
public:
   cGameTools(IUnknown* pOuter)
   {
      MI_INIT_AGGREGATION_1(pOuter, IGameTools, kPriorityNormal, NULL);
      m_fIsToGameGUIDCallback = NULL;
   }
   
   STDMETHOD_(BOOL, IsToGameModeGUID)(REFGUID rguid)
   {
      if (m_fIsToGameGUIDCallback)
      {
         return (m_fIsToGameGUIDCallback)(rguid);
      }
      return FALSE;
   }
   
   STDMETHOD(SetIsToGameModeGUIDCallback)(fIsToGameGUIDCallback inCallback)
   {
      m_fIsToGameGUIDCallback = inCallback;
      return S_OK;
   }

   STDMETHOD(TeleportObject)(THIS_ ObjID obj, const mxs_vector* ppos, const mxs_angvec* pfac, ObjID ref_frame)
   {
      mxs_vector pos = *ppos; 
      mxs_angvec ang = *pfac; 

      if (ref_frame != OBJ_NULL)
      {
         ObjPos* ref_pos = ObjPosGet(ref_frame);
      
         // Compute orientation matrix
         mxs_matrix mat;
         mx_ang2mat(&mat,&ref_pos->fac);

         // transform pos into ref_frame
         mxs_vector oldpos = pos;
         mx_zero_vec(&pos);
      
         for (int i = 0; i < 3; i++)  
         {
            // Transform pos
            mx_scale_addeq_vec(&pos, &mat.vec[i], oldpos.el[i]);
         
            // transform ang
            ang.el[i] += ref_pos->fac.el[i];
         }

         // Now add in ref_frame origin
         mx_addeq_vec(&pos,&ref_pos->loc.vec);
      }

      // check for physics
      if (PhysObjHasPhysics(obj))
      {
         // TODO: figure out if this fits in the world

         PhysSetModLocation(obj,&pos);
         PhysSetModRotation(obj,&ang);
      }

      ObjPosUpdate(obj,&pos,&ang);

      // update the room brush system as well
      cRoom *pPreRoom = g_pRooms->GetObjRoom(obj);
      cRoom *pPostRoom = g_pRooms->RoomFromPoint(pos);
      if ((pPreRoom != NULL) && (pPostRoom != NULL))
         g_pRooms->UpdateObjRoom(obj, pPreRoom, pPostRoom);

      // snap all AI Awareness links to it
      AutoAppIPtr(LinkManager);
      IRelation *pRel;
      pRel = pLinkManager->GetRelationNamed("AIAwareness");
      CleanLinks(obj,pRel->GetID());

      pRel = pLinkManager->GetRelationNamed("AIAttack");
      CleanLinks(obj,pRel->GetID());

#ifdef NEW_NETWORK_ENABLED
      // Tell the other players where obj now is:
      PhysNetBroadcastObjPos(obj, &pos, &ang);
#endif

#ifdef EDITOR
      editobjUpdateBrushfromObj(obj); 
#endif 

      return S_OK; 
   }

}; 

BOOL GameToolIsToGameModeGUID(REFGUID rguid)
{
   IGameTools* pGameTools = AppGetObj(IGameTools);
   if (pGameTools)
   {
	  BOOL bResult = pGameTools->IsToGameModeGUID(rguid);
      SafeRelease(pGameTools);
	  return bResult;
   }
   return FALSE;
}

void GameToolsCreate()
{
   AutoAppIPtr(Unknown); 
   new cGameTools(pUnknown); 
}
