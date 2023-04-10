/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <string.h>
#include <mprintf.h>

#include <property.h>
#include <propface.h>
#include <propbase.h>
#include <propert_.h>
#include <prophash.h>
#include <dataops_.h>

#include <sdesc.h>
#include <sdesbase.h>

#include <susprop.h>


//must be last header
#include <dbmem.h>

F_DECLARE_INTERFACE(ISuspiciousProperty);

#define SUSPTYPELEN 32

struct sSuspiciousInfo
{
   bool   is_suspicious;
   char   suspicious_type[SUSPTYPELEN];
   float  min_light_level;
};

/////////////////////////////
#undef INTERFACE
#define INTERFACE ISuspiciousProperty
DECLARE_PROPERTY_INTERFACE(ISuspiciousProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(sSuspiciousInfo*); 
}; 
#undef INTERFACE

static ISuspiciousProperty *g_pSuspiciousProp;

/////////////////////////////

static sPropertyDesc _g_pSuspiciousPropDesc =
{
  PROP_SUSPICIOUS_NAME,
  0,
  NULL,
  0,
  0,
  {"Engine Features","Suspicious"}
};

//////////////////////////

typedef cSpecificProperty<ISuspiciousProperty, &IID_ISuspiciousProperty, sSuspiciousInfo *, cHashPropertyStore< cClassDataOps<sSuspiciousInfo> > > cSuspiciousPropertyBase;

class cSuspiciousProp : public cSuspiciousPropertyBase
{
public:
  cSuspiciousProp()
    : cSuspiciousPropertyBase(&_g_pSuspiciousPropDesc)
      {
      };

  STANDARD_DESCRIBE_TYPE(sSuspiciousInfo);
};

static sFieldDesc _g_SuspiciousFieldDesc [] =
{
  { "Is Suspicious", kFieldTypeBool, FieldLocation(sSuspiciousInfo,is_suspicious) },
  { "Suspicious Type", kFieldTypeString, FieldLocation(sSuspiciousInfo, suspicious_type)},
  { "Minimum Light Level",kFieldTypeFloat, FieldLocation(sSuspiciousInfo, min_light_level)}
};

static sStructDesc _g_pSuspiciousStructDesc = StructDescBuild(sSuspiciousInfo,kStructFlagNone,_g_SuspiciousFieldDesc);

BOOL InitSuspiciousProp(void)
{
  StructDescRegister(&_g_pSuspiciousStructDesc);
  g_pSuspiciousProp = new cSuspiciousProp;
  return TRUE;
}

////////////////////////////////////////////
BOOL TermSuspiciousProp(void)
{
  SafeRelease(g_pSuspiciousProp);
  return TRUE;
}

////////////////////////////////////////////
BOOL IsSuspicious(ObjID object)
{
  sSuspiciousInfo *pSusInfo;
  if (!g_pSuspiciousProp->Get(object,&pSusInfo))
    return FALSE;
  if (pSusInfo->is_suspicious)
    return TRUE;
  else
    return FALSE;
}

////////////////////////////////////////////
BOOL GetSuspiciousType(ObjID object, char *sustype, int typelen)
{
  sSuspiciousInfo *pSusInfo;
  if (!g_pSuspiciousProp->Get(object,&pSusInfo))
    return FALSE;
  if (typelen < SUSPTYPELEN)
    return FALSE;
  if (pSusInfo->suspicious_type == 0)
    return FALSE;
  if (strcmp(pSusInfo->suspicious_type,"")==0) //empty string
    return FALSE;
  strcpy(sustype,pSusInfo->suspicious_type);
  return TRUE;
}

////////////////////////////////////////////
BOOL GetSuspiciousLightLevel(ObjID object, float *lightlevel)
{
   sSuspiciousInfo *pSusInfo;
   if (!g_pSuspiciousProp->Get(object,&pSusInfo))
      return FALSE;
   if (pSusInfo->min_light_level < 0)
      return FALSE; //don't allow light level 0.  
   *lightlevel = pSusInfo->min_light_level;
   return TRUE;
}

////////////////////////////////////////////
void StartSuspiciousIter(sPropertyObjIter *iter)
{
  g_pSuspiciousProp->IterStart(iter);
}

////////////////////////////////////////////
BOOL NextSuspiciousIter(sPropertyObjIter *iter, ObjID *obj, BOOL *setting)
{
  sSuspiciousInfo *pSusInfo;

  if (!g_pSuspiciousProp->IterNextValue(iter,obj,&pSusInfo)) 
    return FALSE;

  *setting=pSusInfo->is_suspicious;
  
  return TRUE;

}

///////////////////////////////////////////
void StopSuspiciousIter(sPropertyObjIter *iter)
{
  g_pSuspiciousProp->IterStop(iter);
}
