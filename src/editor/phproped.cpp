/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/editor/phproped.cpp,v 1.2 1998/10/09 20:17:20 CCAROLLO Exp $
//

#include <proped.h>
#include <iobjed.h>

#include <sdesc.h>
#include <isdesced.h>
#include <isdescst.h>
#include <sdesbase.h>
#include <sdestool.h>

#include <phcore.h>
#include <phmods.h>
#include <phmod.h>

#include <phprop.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////////////////////////

class cPhysAuxEditTrait: public cSdescPropEditTrait
{
public:
   cPhysAuxEditTrait(IProperty* prop, int type)
      : cSdescPropEditTrait(prop)
   {
      Caps.flags |= kTraitCanParse|kTraitCanUnparse|kTraitCanEdit;

      switch (type)
      {
         case PHYS_ATTR:
         {
            mSdescMap[0] = GetPhysAttrDesc(kPMT_OBB);
            mSdescMap[1] = GetPhysAttrDesc(kPMT_Sphere);
            mSdescMap[2] = GetPhysAttrDesc(kPMT_SphereHat);
            mSdescMap[3] = GetPhysAttrDesc(kPMT_Invalid);
            break;
         }
         case PHYS_STATE:
         {
            mSdescMap[0] = GetPhysStateDesc();
            mSdescMap[1] = GetPhysStateDesc();
            mSdescMap[2] = GetPhysStateDesc();
            mSdescMap[3] = GetPhysStateDesc();
            break;
         }
         case PHYS_CONTROL:
         {
            mSdescMap[0] = GetPhysControlDesc();
            mSdescMap[1] = GetPhysControlDesc();
            mSdescMap[2] = GetPhysControlDesc();
            mSdescMap[3] = GetPhysControlDesc();
            break;
         }
         case PHYS_DIMS:
         {
            mSdescMap[0] = GetPhysDimsDesc(kPMT_OBB);
            mSdescMap[1] = GetPhysDimsDesc(kPMT_Sphere);
            mSdescMap[2] = GetPhysDimsDesc(kPMT_SphereHat);
            mSdescMap[3] = GetPhysDimsDesc(kPMT_Invalid);
            break;
         }
         default:
         {
            Warning(("cPhysAuxEditTrait: unknown type: %d\n", type));
            return;
         }
      }
   }

   STDMETHOD(Edit)(ObjID obj)
   {
      cPhysTypeProp *pTypeProp;

      if (g_pPhysTypeProp->Get(obj, &pTypeProp))
      {
         Sdesc = mSdescMap[pTypeProp->type];
         return cSdescPropEditTrait::Edit(obj);
      }
      else
      {
         Warning(("Edit: obj %d has no phys type property!\n", obj));
         return S_FALSE;
      }
   }

   STDMETHOD(Unparse)(ObjID obj, char* buf, int buflen) 
   {
      cPhysTypeProp *pTypeProp;

      if (g_pPhysTypeProp->Get(obj, &pTypeProp))
      {
         Sdesc = mSdescMap[pTypeProp->type];
         return cSdescPropEditTrait::Unparse(obj, buf, buflen);
      }
      else
      {
         Warning(("Edit: obj %d has no phys type property!\n", obj));
         return S_FALSE;
      }
   }

private:

   sStructDesc *mSdescMap[4];
};

////////////////////////////////////////////////////////////////////////////////

void CreateAuxPhysPropEditor(IProperty *prop, int type)
{
   IObjEditors* pEditors = AppGetObj(IObjEditors);
   if (pEditors)
   {
      IEditTrait *trait = new cPhysAuxEditTrait(prop, type);
      pEditors->AddTrait(trait);
      SafeRelease(trait);
      SafeRelease(pEditors);
   }
}

////////////////////////////////////////////////////////////////////////////////






