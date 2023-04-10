/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//
// Death model name property
//

#include <dpcdmprp.h>

#include <property.h>
#include <propface.h>
#include <propbase.h>

#include <osysbase.h>

#include <appagg.h>

#include <traitman.h>
#include <traitbas.h>
#include <objquery.h>

#include <objmodel.h>
#include <mprintf.h>

#include <dmgmodel.h>
#include <dmgbase.h>
#include <mnamprop.h>

#include <dbmem.h>

static ILabelProperty* pDeathModelNameProp = NULL;

////////////////////////////////////////////////////////////
// DEATH MODEL NAME PROPERTY CREATION 
//

#define DEATHMODELNAMEPROP_IMPL kPropertyImplSparse

static sPropertyConstraint deathmodelnameprop_const[] =
{
   { kPropertyNullConstraint, NULL }
};

static sPropertyDesc deathmodelnameprop_desc =
{
   PROP_DEATHMODELNAME_NAME,
   0,
   deathmodelnameprop_const,
   0,0, // verison
   { "Shape", "Death Model Name" }, 
};

/*
// called when the name is modified
static void LGAPI ModelNameListener(sPropertyListenMsg* msg, PropListenerData data)
{
   if (msg->property != modelnameprop->GetID())
      return;
   
   // don't do this work here, it is done in NumProp::Notify
   if (msg->type & kListenPropLoad) 
      return ;

   if (msg->type & (kListenPropModify|kListenPropSet))
   {
      int dummy;
      if (OBJ_IS_CONCRETE(msg->obj) || ObjGetModelNumber(msg->obj,&dummy))
         ObjLoadModel(msg->obj);
      else // load all descendents
      {
         AutoAppIPtr_(TraitManager,TraitMan);
         IObjectQuery* query = TraitMan->Query(msg->obj,kTraitQueryAllDescendents);
         for (; !query->Done(); query->Next())
         {
            ObjID obj = query->Object();
            if (OBJ_IS_CONCRETE(obj))
               ObjLoadModel(msg->obj);
         }
         SafeRelease(query);
      }
   }
}
*/

eDamageResult LGAPI DeathModelDamageListener(const sDamageMsg* pMsg, tDamageCallbackData data)
{
   Label name;

   if (ObjGetDeathModelName(pMsg->victim, (char*)&name))
      ObjSetModelName(pMsg->victim, (char*)&name);
   return kDamageNoOpinion;
}

// Init the property
void DeathModelNamePropInit(void)
{
   pDeathModelNameProp = CreateLabelProperty(&deathmodelnameprop_desc, DEATHMODELNAMEPROP_IMPL);
   AutoAppIPtr_(DamageModel, pDamageModel);
   pDamageModel->Listen(kDamageMsgSlay, &DeathModelDamageListener, NULL);
}

// get and set functions
BOOL ObjGetDeathModelName(ObjID obj, char *name)
{
   Assert_(pDeathModelNameProp);
   char *temp;
   BOOL retval = pDeathModelNameProp->Get(obj, (Label**)&temp);
   if (retval) // hope name long enough!
      strcpy(name,temp);
   return retval;
}

void ObjSetDeathModelName(ObjID obj, char *name)
{
   Assert_(pDeathModelNameProp);
   pDeathModelNameProp->Set(obj, (Label*)name);
}
