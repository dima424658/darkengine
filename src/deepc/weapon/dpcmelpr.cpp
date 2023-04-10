/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/


#include <dpcmelpr.h>

#include <propert_.h>
#include <sdesc.h>
#include <sdesbase.h>

// must be last header
#include <dbmem.h>

/////////////////////////////////////////////////////////////
// Melee Type Property

IIntProperty* g_pMeleeTypeProperty = NULL;

static sPropertyDesc meleeTypePropDesc =
{
   PROP_DPC_MELEE_TYPE_NAME,
   0, NULL, 0, 0,
   { "Gun", "Melee Type" }
};

static char *meleeTypeNames[] = 
{ 
   "Knife", 
   "Knuckles", 
};
 
static sFieldDesc meleeTypeFieldDesc[] = 
{
   { "", kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, 2, 2, meleeTypeNames},
}; 

static sStructDesc meleeTypeStructDesc = 
{
   PROP_DPC_MELEE_TYPE_NAME, 
   sizeof(int),
   kStructFlagNone,
   sizeof(meleeTypeFieldDesc)/sizeof(meleeTypeFieldDesc[0]),
   meleeTypeFieldDesc,
}; 

static sPropertyTypeDesc meleeTypeTypeDesc = {PROP_DPC_MELEE_TYPE_NAME, sizeof(int)}; 

// Init the property
void MeleeTypePropertyInit(void)
{
   StructDescRegister(&meleeTypeStructDesc);
   g_pMeleeTypeProperty = CreateIntegralProperty(&meleeTypePropDesc, &meleeTypeTypeDesc, kPropertyImplDense);
}

void MeleeTypePropertyShutdown(void)
{
   SafeRelease(g_pMeleeTypeProperty);
}
