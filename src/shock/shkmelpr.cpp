/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkmelpr.cpp,v 1.1 1998/08/17 16:50:25 JON Exp $

#include <shkmelpr.h>

#include <propert_.h>
#include <sdesc.h>
#include <sdesbase.h>

// must be last header
#include <dbmem.h>

/////////////////////////////////////////////////////////////
// Melee Type Property

IIntProperty* g_pMeleeTypeProperty;

static sPropertyDesc meleeTypePropDesc =
{
   PROP_SHOCK_MELEE_TYPE_NAME,
   0, NULL, 0, 0,
   { "Gun", "Melee Type" }
};

static char *meleeTypeNames[] = 
{ 
   "Pipe", 
   "Shock", 
   "Shard", 
};
 
static sFieldDesc meleeTypeFieldDesc[] = 
{
   { "", kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, 1, 1, meleeTypeNames},
}; 

static sStructDesc meleeTypeStructDesc = 
{
   PROP_SHOCK_MELEE_TYPE_NAME, 
   sizeof(int),
   kStructFlagNone,
   sizeof(meleeTypeFieldDesc)/sizeof(meleeTypeFieldDesc[0]),
   meleeTypeFieldDesc,
}; 

static sPropertyTypeDesc meleeTypeTypeDesc = {PROP_SHOCK_MELEE_TYPE_NAME, sizeof(int)}; 

// Init the property
void MeleeTypePropertyInit(void)
{
   StructDescRegister(&meleeTypeStructDesc);
   g_pMeleeTypeProperty = CreateIntegralProperty(&meleeTypePropDesc, &meleeTypeTypeDesc, kPropertyImplDense);
}
