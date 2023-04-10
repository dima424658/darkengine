/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/meshatt.cpp,v 1.9 1999/10/21 17:40:44 adurant Exp $

/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\
   meshatt.cpp

   attaching things to mesh objects

   This property is the custodian of the custom data for objects with
   mesh models, though we may want to pull that out into its own
   system if it ever grows beyond attaching objects.

   The thing we attach is a regular object, identified by ObjID.

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */

#include <mprintf.h>
#include <matrix.h>

#include <mm.h>

#include <portal.h>
#include <propert_.h>
#include <proplist.h>
#include <propbase.h>
#include <sdesc.h>
#include <sdesbase.h>
#include <rendobj.h>
#include <objpos.h>
#include <objmodel.h>
#include <mnumprop.h>
#include <mprintf.h>

#include <meshatt.h>
#include <meshatts.h>

// last header
#include <dbmem.h>

//
// Mesh attach data ops
//
void DestroyCustomData(sMeshAttach* pAttach); 


struct sOpsMeshAttach : public sMeshAttach
{
   void Alloc()
   {
      m_pCustom = new mms_custom_data; 
      m_pCustom->attachment_list = new mms_attachment[kMeshAttachMax]; 
      m_pCustom->num_attachments = kMeshAttachMax;
      m_pAccessory = new sMeshAttachAccessory; 
      m_pAccessory->m_bAttachmentCurrent = FALSE;      
   }

   const sOpsMeshAttach& operator = (const sOpsMeshAttach& that)
   {
      memcpy(m_Attachment,that.m_Attachment,sizeof(m_Attachment)); 
      m_pAccessory->m_bAttachmentCurrent = FALSE; 

      return *this; 
   }

   sOpsMeshAttach(const sOpsMeshAttach& that)
   {
      Alloc(); 
      operator =(that); 
   }

   sOpsMeshAttach()
   {
      Alloc(); 
   }

   ~sOpsMeshAttach()
   {
      delete m_pCustom->attachment_list;  
      delete m_pCustom; 
      delete m_pAccessory; 
   }
};

class cMeshAttachOps : public cClassDataOps<sOpsMeshAttach>
{
public:
   cMeshAttachOps()
      : cClassDataOps<sOpsMeshAttach>(kNoFlags)
   {
   }

}; 

//
// Mesh attach property store
//
class cMeshAttachStore : public cListPropertyStore<cMeshAttachOps>
{
}; 

//
// Mesh attach property
//
typedef cSpecificProperty<IMeshAttachProperty, &IID_IMeshAttachProperty,
                          sMeshAttach*, cMeshAttachStore> cBaseMeshAttachProp;

class cMeshAttachProp : public cBaseMeshAttachProp
{
public:
   cMeshAttachProp(const sPropertyDesc* desc)
      : cBaseMeshAttachProp(desc)
   {
   }

   void OnListenMsg(ePropertyListenMsg msg, ObjID obj, uPropListenerValue val)
   {
      sMeshAttach* pAttach = (sMeshAttach*)val.ptrval; 

      if (msg & kListenPropModify) 
      {
         Get(obj,&pAttach); // get the identical value of the property
         pAttach->m_pAccessory->m_bAttachmentCurrent = FALSE;
      }

      cBaseMeshAttachProp::OnListenMsg(msg,obj,val); 
   }

   STANDARD_DESCRIBE_TYPE(sMeshAttach); 
};

//
// sdesc info, horrible macro for basic array element
//
#define an_sMeshAttach_field_set(n,nstr) \
   { "attached obj"nstr,  kFieldTypeInt,    FieldLocation(sMeshAttach, m_Attachment[n].m_Object) }, \
   { "joint of skeleton", kFieldTypeInt,    FieldLocation(sMeshAttach, m_Attachment[n].m_iJoint) }, \
   { "x angle (0-65535)", kFieldTypeShort,  FieldLocation(sMeshAttach, m_Attachment[n].m_Angles.tx) }, \
   { "y angle (0-65535)", kFieldTypeShort,  FieldLocation(sMeshAttach, m_Attachment[n].m_Angles.ty) }, \
   { "z angle (0-65535)", kFieldTypeShort,  FieldLocation(sMeshAttach, m_Attachment[n].m_Angles.tz) }, \
   { "offset",            kFieldTypeVector, FieldLocation(sMeshAttach, m_Attachment[n].m_Offset) }

// here is the actual sdesc data
static sFieldDesc g_Fields[] =
{
   an_sMeshAttach_field_set(0," 1"),
   an_sMeshAttach_field_set(1," 2"),
   an_sMeshAttach_field_set(2," 3"),
   an_sMeshAttach_field_set(3," 4"),
   
   { "custom data", kFieldTypeVoidPtr, FieldLocation(sMeshAttach, m_pCustom),
     kFieldFlagInvisible | kFieldFlagNotEdit },

   { "accessory", kFieldTypeVoidPtr, FieldLocation(sMeshAttach, m_pAccessory),
     kFieldFlagInvisible | kFieldFlagNotEdit },
};


/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\

   We do not actually store the custom data for our models with our
   levels, since it's full of pointers and indices which could change
   if the mesh models were altered.  Instead we derive it on-the-fly,
   using data structures which follow the object's creation and
   deletion.

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */

// @TODO: pCustom->num_attachments should be kept up to date, ie. shouldnt
//   be just called with 4 every time, even if there are basically 0

// copy orientation of attachments from property to mms_custom_data
static void UpdateAttachments(ObjID obj, sMeshAttach *pAttach,
                              mms_model *pModel)
{
   sMeshAttachInstance *pInstance = pAttach->m_Attachment;
   mms_attachment *pAttachment = pAttach->m_pCustom->attachment_list;

   for (int i = 0; i < kMeshAttachMax; ++i) {
      pAttachment->user_data = pInstance->m_Object;

      if (pInstance->m_Object != OBJ_NULL) {
         pAttachment->segment_index
            = mm_rigid_segment_from_joint(pModel, pInstance->m_iJoint);

#ifndef SHIP
         if (pAttachment->segment_index == -1)
            mprintf("Mesh attach: no rigid segment for joint %d\n",
                    pInstance->m_iJoint);
#endif // ~SHIP

         mx_ang2mat(&pAttachment->relative_trans.mat, 
                    &pAttach->m_Attachment[i].m_Angles);
         pAttachment->relative_trans.vec = pInstance->m_Offset;
      }

      ++pInstance;
      ++pAttachment;
   }
   pAttach->m_pAccessory->m_bAttachmentCurrent = TRUE;
}


static sStructDesc g_Struct
   = StructDescBuild(sMeshAttach, kStructFlagNone, g_Fields);

// the property descriptor
static sPropertyDesc g_PropertyDesc =
{
   PROP_MESH_ATTACH_DATA_NAME,
   kPropertyNoInherit | kPropertyInstantiate, NULL, 0, 0,
   { "Renderer", "Mesh Attach" },
};

static IMeshAttachProperty *g_pMeshAttachProperty = 0;

// In the Beginning...
extern "C" BOOL MeshAttachPropInit(void)
{
   StructDescRegister(&g_Struct);
   g_pMeshAttachProperty = new cMeshAttachProp(&g_PropertyDesc);
   return TRUE;
}


extern "C" BOOL ObjMeshAttachGet(ObjID obj, sMeshAttach **ppAttach)
{
   Assert_(g_pMeshAttachProperty);
   return g_pMeshAttachProperty->Get(obj, ppAttach);
}


extern "C" BOOL ObjMeshAttachSet(ObjID obj, sMeshAttach *pAttach)
{
   Assert_(g_pMeshAttachProperty);
   g_pMeshAttachProperty->Set(obj, pAttach);
   // really.. if pAttach is NULL, or if all its m_Attach objs are 0
   //  we should remove the property, not just set it to useless data!!!

   // why is this here! this makes nooooo sense
   return (g_pMeshAttachProperty != NULL);
}

EXTERN BOOL AttachObjToMeshObj(ObjID mesh_obj, sMeshAttachInstance *mData)
{
   sMeshAttach *pMeshAttachInfo, NewAttachData;
   int use_idx=0;
   
   if (g_pMeshAttachProperty->Get(mesh_obj, &pMeshAttachInfo))
   {
      for (; use_idx<kMeshAttachMax; use_idx++)
         if (pMeshAttachInfo->m_Attachment[use_idx].m_Object==OBJ_NULL)
            break;  // found our candidate
   }
   else
   {  // create the structure and set it
      pMeshAttachInfo=&NewAttachData;
      memset(pMeshAttachInfo,0,sizeof(sMeshAttach));
   }
   if (use_idx<kMeshAttachMax)
   {
      pMeshAttachInfo->m_Attachment[use_idx]=*mData;
      ObjMeshAttachSet(mesh_obj,pMeshAttachInfo);
   }
   else
   {
      Warning(("Out of mesh attach prop slots for %d (attachee %d)\n",mesh_obj,mData->m_Object));
      Warning(("Someday, when we use links direct, it will all be fine\n"));
      return FALSE;
   }
   return TRUE;
}

EXTERN BOOL RemoveObjFromMeshObj(ObjID mesh_obj, ObjID attached_obj)
{
   sMeshAttach *pMeshAttachInfo;
   if (g_pMeshAttachProperty->Get(mesh_obj, &pMeshAttachInfo))
   {
      int i;
      for (i=0; i<kMeshAttachMax; i++)
         if (pMeshAttachInfo->m_Attachment[i].m_Object==attached_obj)
            break;
      if (i==kMeshAttachMax)
         Warning(("Couldn't find %d attached to %d, punting\n",attached_obj,mesh_obj));
      else
      {  // here is the real do the job code
         memset(&pMeshAttachInfo->m_Attachment[i],0,sizeof(sMeshAttachInstance));
         ObjMeshAttachSet(mesh_obj,pMeshAttachInfo);
         return TRUE;
      }
   }
   else
      Warning(("Trying to remove %d from %d, but it has no attachments\n",attached_obj,mesh_obj));
   return FALSE;
}

// This is set by the mesh setup code in in rendobj.c.
static uchar *g_pMeshAttachClut = 0;

// This is the callback handed to the mesh library for rendering
// attachments.
extern "C" void MeshAttachRenderCallback(mxs_trans *trans,
                                         mms_attachment *attachment)
{
   if (attachment->user_data == OBJ_NULL)
      return;

   mxs_angvec new_angles;

   mx_mat2ang(&new_angles, &trans->mat);
   ObjPosUpdate(attachment->user_data, &trans->vec, &new_angles);

   // want to add "if software, try and decide before or after or something"
   // uses portal_render_object to work w/pick system
   portal_render_object(attachment->user_data, g_pMeshAttachClut, OBJ_NO_SPLIT);   
}

extern "C" mms_custom_data *MeshAttachRenderSetup(ObjID obj, mms_model *pModel,
                                                  uchar *pClut)
{
   sMeshAttach *pAttach;

   if (!g_pMeshAttachProperty->Get(obj, &pAttach)) {
      mm_set_attachment_cback(NULL);
      return NULL;
   }      

   if (!(pAttach->m_pAccessory->m_bAttachmentCurrent))
      UpdateAttachments(obj, pAttach, pModel);

   g_pMeshAttachClut = pClut;
   mm_set_attachment_cback(MeshAttachRenderCallback);
   return pAttach->m_pCustom;
}
