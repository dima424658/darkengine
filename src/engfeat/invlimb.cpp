/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/invlimb.cpp,v 1.2 1998/10/05 17:26:19 mahk Exp $

#include <invlimb.h>
// for silly inv properties for now
#include <propface.h>
#include <property.h>
#include <propbase.h>

// must be last header
#include <dbmem.h>

static ILabelProperty *pInvLimbModel = NULL;

Label *invGetLimbModelName(ObjID o)
{
   Label *pName = NULL;

   pInvLimbModel->Get(o,&pName);
   return pName; 
}

static sPropertyDesc inv_limb_desc =  { "InvLimbModel", 0, NULL, 0, 0, { "Inventory", "Limb Model" } };

void invLimbInit(void)
{
   pInvLimbModel=CreateLabelProperty(&inv_limb_desc,kPropertyImplVerySparse);
}

void invLimbTerm(void)
{
   SafeRelease(pInvLimbModel); 
}


