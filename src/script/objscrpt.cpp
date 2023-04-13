/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <types.h>
#include <refsys.h>

// $Header: r:/t2repos/thief2/src/script/objscrpt.cpp,v 1.18 2000/01/17 19:44:02 adurant Exp $

#include <comtools.h>
#include <appagg.h>

#include <creature.h>

#include <scrptsrv.h>

#include <objscrpt.h>

#include <iobjsys.h>
#include <osysbase.h>
#include <objlpars.h>

#include <gametool.h>
#include <objpos.h>
#include <wrtype.h>
#include <rendobj.h>

#include <netmsg.h>

// Used by FindClosestObjectNamed
#include <aiutils.h>
#include <bspsphr.h>
#include <objshape.h>
#include <traitman.h>
#include <objtype.h>
#include <wrfunc.h>
#include <wrdb.h>
#include <traitbas.h>
#include <objquery.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// OBJECT SCRIPTING SERVICE IMPLEMENTATION
//

static IObjectSystem* gpObjSys = NULL; 

static IObjectSystem* ObjSys()
{
   if (gpObjSys == NULL)
   {
      gpObjSys = AppGetObj(IObjectSystem); 
   }
   return gpObjSys; 
}

static ITraitManager* gpTraitMan = NULL; 

static ITraitManager* TraitMan()
{
   if (gpTraitMan == NULL)
   {
      gpTraitMan = AppGetObj(ITraitManager); 
   }
   return gpTraitMan; 
}

static IGameTools* gpGameTools = NULL; 

static IGameTools* GameTools()
{
   if (gpGameTools == NULL)
   {
      gpGameTools = AppGetObj(IGameTools); 
   }
   return gpGameTools; 
}

////////////////////////////////////////////////////////////
//
// Networking code
//

static void handleChangeMetaProp(ObjID obj, ObjID metaprop, BOOL bAdd)
{
   if (bAdd) {
      // Avoid adding duplicate metaproperties, because they give networking
      // indigestion:
      if (!TraitMan()->ObjHasDonor(obj, metaprop)) {
         TraitMan()->AddObjMetaProperty(obj, metaprop);
      }
   } else {
      TraitMan()->RemoveObjMetaProperty(obj, metaprop);
   }
}

static sNetMsgDesc g_ChangeMetaPropDesc =
{
   kNMF_SendToObjOwner,
   "ChangeMeta",
   "Change MetaProperty",
   NULL,
   handleChangeMetaProp,
   {{kNMPT_ReceiverObjID, kNMPF_None, "Obj"},
    // We assume that metaprop is an abstract, and same on all machines:
    {kNMPT_ObjID, kNMPF_None, "Metaproperty"},
    // TRUE == add; FALSE == remove
    {kNMPT_BOOL, kNMPF_None, "Add/Remove"},
    {kNMPT_End}}
};


DECLARE_SCRIPT_SERVICE_IMPL(cObjectSrv, Object)
{
public:
   STDMETHOD_(void, Init)()
   {
      m_pChangeMetaPropMsg = new cNetMsg(&g_ChangeMetaPropDesc);
   }

   STDMETHOD_(void, End)()
   {
      delete m_pChangeMetaPropMsg;
   }

   STDMETHOD_(object,BeginCreate)(object sobj)
   {
      ObjID obj = ScriptObjID(sobj); 
      AssertMsg(ObjSys()->Exists(obj),"Cloning a non-existent object!");
      return ObjSys()->BeginCreate(obj,kObjectConcrete); 
   }

   STDMETHOD(EndCreate)(object obj)
   {
      return ObjSys()->EndCreate(ScriptObjID(obj)); 
   }

   STDMETHOD_(object,Create)(object sobj)
   {
      ObjID obj = ScriptObjID(sobj);
      AssertMsg(ObjSys()->Exists(obj),"Cloning a non-existent object!");
      return ObjSys()->Create(obj,kObjectConcrete); 
   }

   STDMETHOD(Destroy)(object obj)
   {
      return ObjSys()->Destroy(ScriptObjID(obj)); 
   }

   STDMETHOD_(boolean,Exists)(object obj)
   {
      return ObjSys()->Exists(ScriptObjID(obj)); 
   }

   STDMETHOD(SetName)(object obj, const char* name)
   {

      return ObjSys()->NameObject(ScriptObjID(obj),name);
   }

   STDMETHOD_(string,GetName)(object obj)
   {
      return ObjSys()->GetName(ScriptObjID(obj)); 
   }

   STDMETHOD_(object,Named)(const char* name)
   {
      return ObjSys()->GetObjectNamed(name);
   }

   STDMETHOD(SetTransience)(object obj, boolean is_transient)
   {
      return ObjSys()->SetObjTransience(ScriptObjID(obj),is_transient); 
   }

   STDMETHOD_(boolean,IsTransient)(object obj)
   {
      return ObjSys()->ObjIsTransient(ScriptObjID(obj)); 
   }

   STDMETHOD(AddMetaProperty)(object obj, object metaprop) 
   {
#ifdef NEW_NETWORK_ENABLED
      m_pChangeMetaPropMsg->Send(OBJ_NULL, 
                                 ScriptObjID(obj),
                                 ScriptObjID(metaprop),
                                 TRUE);
      return S_OK;
#else
      return TraitMan()->AddObjMetaProperty(ScriptObjID(obj),ScriptObjID(metaprop)); 
#endif
   }

   STDMETHOD(RemoveMetaProperty)(object obj, object metaprop) 
   {
#ifdef NEW_NETWORK_ENABLED
      m_pChangeMetaPropMsg->Send(OBJ_NULL, 
                                 ScriptObjID(obj),
                                 ScriptObjID(metaprop),
                                 FALSE);
      return S_OK;
#else
      return TraitMan()->RemoveObjMetaProperty(ScriptObjID(obj),ScriptObjID(metaprop)); 
#endif
   }

   STDMETHOD_(boolean,HasMetaProperty)(object obj, object metaprop) 
   {
      return TraitMan()->ObjHasDonorIntrinsically(ScriptObjID(obj),ScriptObjID(metaprop)); 
   }

   STDMETHOD_(boolean,InheritsFrom)(object obj, object donor) 
   {
      return TraitMan()->ObjHasDonor(ScriptObjID(obj),ScriptObjID(donor));
   }

#define DEGREES2ANG(x) (mxs_ang)(((long)((x) * MX_ANG_PI))/180)
#define ANG2DEGREES(x) (((mxs_real)(x) * 180)/MX_ANG_PI)

   STDMETHOD(Teleport)(object obj, const vector& position, const vector& facing, object ref_frame) 
   {
      mxs_angvec fac = { DEGREES2ANG(facing.x), DEGREES2ANG(facing.y), DEGREES2ANG(facing.z) }; 
      return GameTools()->TeleportObject(obj,&position,&fac,ref_frame); 
   }

   STDMETHOD_(vector,Position)(object obj)
   {
      ObjPos* pos = ObjPosGet(obj);
      Assert_(pos!=NULL);
      return pos->loc.vec; 
   }

   STDMETHOD_(vector,Facing)(object obj)
   {
      ObjPos* pos = ObjPosGet(obj);
      Assert_(pos!=NULL);
      mxs_angvec& ang = pos->fac; 
      return vector(ANG2DEGREES(ang.tx),ANG2DEGREES(ang.ty),ANG2DEGREES(ang.tz)); 
   }

   STDMETHOD_(boolean,IsPositionValid)(object obj)
   {
      return (boolean)IsCreaturePositionValid(obj);
   }

   // Object Searching
   STDMETHOD_(object,FindClosestObjectNamed)(ObjID objId, const char* name)
   {
      // Basis of this stuff is from Mahk's radiag8r.cpp code.
      return object(FindTheClosestObjectNamed(objId, name));
   }

   STDMETHOD_(integer, AddMetaPropertyToMany)(object metaprop, const string& ToSet)
   {
      cObjListParser   parser;
      cDynArray<ObjID> objs;
      ObjID            metaPropID = ScriptObjID(metaprop);
      
      if (!metaPropID)
         return 0;
      
      parser.Parse(ToSet, &objs);

      for (int i = 0; i < objs.Size(); i++) {
#ifdef NEW_NETWORK_ENABLED
         m_pChangeMetaPropMsg->Send(OBJ_NULL, 
                                    ScriptObjID(objs[i]),
                                    ScriptObjID(metaPropID),
                                    TRUE);
#else
         TraitMan()->AddObjMetaProperty(objs[i], metaPropID);
#endif
      }
      
      return objs.Size();
   }
   
   STDMETHOD_(integer, RemoveMetaPropertyFromMany)(object metaprop, const string& FromSet)
   {
      cObjListParser   parser;
      cDynArray<ObjID> objs;
      ObjID            metaPropID = ScriptObjID(metaprop);
      
      if (!metaPropID)
         return 0;

      parser.Parse(FromSet, &objs);

      for (int i = 0; i < objs.Size(); i++) {
#ifdef NEW_NETWORK_ENABLED
         m_pChangeMetaPropMsg->Send(OBJ_NULL, 
                                    ScriptObjID(objs[i]),
                                    ScriptObjID(metaPropID),
                                    FALSE);
#else
         TraitMan()->RemoveObjMetaProperty(objs[i], metaPropID);
#endif
      }
      
      return objs.Size();
   }

   // misc useful thing to be able to ask about objects
   STDMETHOD_(boolean, RenderedThisFrame)(object scr_obj)
   {
      ObjID obj = ScriptObjID(scr_obj);
      return rendobj_object_visible(obj);
   }

protected:
   cNetMsg *m_pChangeMetaPropMsg;
};

// @NOTE:  Bodisafa 12/9/1999
// About DO_SPHERE_TEST:
// I'm not sure if we'll want/need the sphere test as part of this.
// For small matches it should be fine to use Trait Manager.
// However, I put the sphere stuff in since Mahk and I talked about it.

ObjID FindTheClosestObjectNamed(ObjID objId, const char* name)
{
   // Basis of this stuff is from Mahk's radiag8r.cpp code.
   
   ObjPos* pos = ObjPosGet(objId);
   if (!pos)
   {
      return OBJ_NULL;
   }
   
   mxs_vector objLocation = pos->loc.vec;
   
#ifdef DO_SPHERE_TEST
   int cells[BSPSPHR_OUTPUT_LIMIT]; 
   const int n = portal_cells_intersecting_sphere(&pos->loc, ObjGetRadius(objId), cells); 

   // Mahk says, "if you thought that array was decadent, check out this one" 
   Assert_(gMaxObjID < 16384); 
   bool bVisited[16384]; 
   memset(bVisited, 0, sizeof(bVisited));
#endif // DO_SPHERE_TEST
   
   ObjRef*  r          = NULL;
   ObjID    curObj     = OBJ_NULL;
   ObjID    returnObj  = OBJ_NULL;
   float    minDist    = 4294967295.0f;
   float    curMinDist = 0.0f;

#ifdef DO_SPHERE_TEST
   // Find closest object within this object's sphere.
   for (int c = 0; c < n; c++)
   {
      PortalCell* cell = WR_CELL(cells[c]); 
      ObjRefID    id = *(int*)&cell->refs; 
      while (id)
      {
         r = (ObjRefInfos[id].ref); //OBJREFID_TO_PTR(id); 
         curObj = r->obj;
         if (OBJ_IS_CONCRETE(curObj) && (!bVisited[curObj]) && !stricmp(GetName(curObj), name))
         {
            curMinDist = AIDistanceSq(objLocation, *(GetObjLocation(curObj)));
            if (curMinDist < minDist)
            {
               returnObj = curObj;
               minDist   = curMinDist;
            }
            bVisited[curObj] = TRUE; 
         }
         id = r->next_in_bin; 
      }
   }
#else

   // Query the trait manager for all objects in the database with the given name.
   // Then, find the closest object.
   IObjectQuery* query = TraitMan()->Query(ObjSys()->GetObjectNamed(name), kTraitQueryAllDescendents);
   for (; !query->Done(); query->Next())
   {
      curObj = query->Object();
      if (OBJ_IS_CONCRETE(curObj))
      {
         curMinDist = AIDistanceSq(objLocation, *(GetObjLocation(curObj)));
         if (curMinDist < minDist)
         {
            returnObj = curObj;
            minDist   = curMinDist;
         }

      }
   }

#endif // DO_SPHERE_TEST
   return returnObj;
}

IMPLEMENT_SCRIPT_SERVICE_IMPL(cObjectSrv, Object);
