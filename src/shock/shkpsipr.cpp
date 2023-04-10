/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkpsipr.cpp,v 1.13 1999/11/19 14:58:39 adurant Exp $

#include <shkpsipr.h>

#include <propert_.h>
#include <dataops_.h>
#include <prophash.h>

#include <sdesc.h>
#include <sdesbase.h>

#include <relation.h>
#include <linkbase.h>

#include <shkplcst.h>
#include <nzprutil.h>
#include <shkpsibs.h>
#include <shkpsipw.h>

// for esnd cut 'n paste
#include <property.h>
#include <propbase.h>
#include <propface.h>
#include <propfac_.h>
#include <proplist.h>
#include <objnotif.h>

// must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////
// Teleport link

IRelation *g_pPsiTeleportLinks;

static sRelationDesc psiTeleportDesc = { "Teleport", 0, 0, 0 };
static sRelationDataDesc noDataDesc = { "None", 0 };

//////////////////////////////////////////////////
// Psi Power desc
//

IPsiPowerProperty *g_pPsiPowerProperty;

typedef cSpecificProperty<IPsiPowerProperty, &IID_IPsiPowerProperty, sPsiPower*, cHashPropertyStore<cClassDataOps<sPsiPower> > > cPsiPowerPropertyBase;

// property implementation class
class cPsiPowerProperty: public cPsiPowerPropertyBase
{
public:
   cPsiPowerProperty(const sPropertyDesc* desc)
      : cPsiPowerPropertyBase(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(sPsiPower); 

};

char* psiTypeNames[] = 
{
   "Shot",
   "Shield",
   "One-Shot",
   "Sustained", 
   "Cursor Target"
};

static sFieldDesc psiPowerFields[] = 
{
   { "Power", kFieldTypeEnum, FieldLocation(sPsiPower, m_power), kFieldFlagUnsigned, 0, kPsiMax, kPsiMax, psiPowerNames},
   { "Type", kFieldTypeEnum, FieldLocation(sPsiPower, m_type), kFieldFlagUnsigned, 0, kPsiTypeMax, kPsiTypeMax, psiTypeNames},
   { "Start Cost", kFieldTypeInt, FieldLocation(sPsiPower, m_startCost), },
   { "Data 1", kFieldTypeFloat, FieldLocation(sPsiPower, m_data[0]), },
   { "Data 2", kFieldTypeFloat, FieldLocation(sPsiPower, m_data[1]), },
   { "Data 3", kFieldTypeFloat, FieldLocation(sPsiPower, m_data[2]), },
   { "Data 4", kFieldTypeFloat, FieldLocation(sPsiPower, m_data[3]), },
};

static sStructDesc psiPowerStructDesc = 
   StructDescBuild(sPsiPower, kStructFlagNone, psiPowerFields);

IPsiPowerProperty *CreatePsiPowerProperty(sPropertyDesc *desc, 
                                                          ePropertyImpl impl)
{
   StructDescRegister(&psiPowerStructDesc);
   return new cPsiPowerProperty(desc);
}

static sPropertyDesc psiPowerDesc = 
{
   PROP_PSI_POWER_DESC, 
   kPropertyNoInherit,
   NULL, 
   1,    // version
   0,
   {"Psi", "Power Description"},
};

void PsiPowerPropertyInit()
{
   g_pPsiPowerProperty = CreatePsiPowerProperty(&psiPowerDesc, kPropertyImplDense);
}

//////////////////////////////////////////////////
// Psi Shield desc
//

IPsiShieldProperty *g_pPsiShieldProperty;

typedef cSpecificProperty<IPsiShieldProperty, &IID_IPsiShieldProperty, sPsiShield*, cHashPropertyStore<cClassDataOps<sPsiShield> > > cPsiShieldPropertyBase;

// property implementation class
class cPsiShieldProperty: public cPsiShieldPropertyBase
{
public:
   cPsiShieldProperty(const sPropertyDesc* desc)
      : cPsiShieldPropertyBase(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(sPsiShield); 

};

static sFieldDesc psiShieldFields[] = 
{
   {"Base Time", kFieldTypeInt, FieldLocation(sPsiShield, m_baseTime), },
   {"Add Time", kFieldTypeInt, FieldLocation(sPsiShield, m_addTime), },
   {"Base Int", kFieldTypeInt, FieldLocation(sPsiShield, m_baseInt), },
};

static sStructDesc psiShieldStructDesc = 
   StructDescBuild(sPsiShield, kStructFlagNone, psiShieldFields);

IPsiShieldProperty *CreatePsiShieldProperty(sPropertyDesc *desc, 
                                                          ePropertyImpl impl)
{
   StructDescRegister(&psiShieldStructDesc);
   return new cPsiShieldProperty(desc);
}

static sPropertyDesc psiShieldDesc = 
{
   PROP_PSI_SHIELD_DESC, 
   kPropertyNoInherit,
   NULL, 
   1,    // version
   0,
   {"Psi", "Shield Description"},
};

sPsiShield defaultPsiShield =
{
   0, 0, 0,
};

void PsiShieldPropertyInit()
{
   g_pPsiShieldProperty = CreatePsiShieldProperty(&psiShieldDesc, kPropertyImplDense);
}

//////////////////////////////////////////////////
// Psi State desc
//

IPsiStateProperty *g_pPsiStateProperty;

typedef cSpecificProperty<IPsiStateProperty, &IID_IPsiStateProperty, cPsiState*, cHashPropertyStore<cNoZeroDataOps<cPsiState> > > cPsiStatePropertyBase;

// property implementation class
class cPsiStateProperty: public cPsiStatePropertyBase
{
public:
   cPsiStateProperty(const sPropertyDesc* desc)
      : cPsiStatePropertyBase(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(cPsiState); 

};

static sFieldDesc psiStateFields[] = 
{
   { "", kFieldTypeEnum, FieldLocation(cPsiState, m_currentPower), kFieldFlagUnsigned|kFieldFlagNotEdit, 0, kPsiMax+1, kPsiMax+1, psiPowerNames},
   {"Current Points", kFieldTypeInt, FieldLocation(cPsiState, m_points), },
   {"Max Points", kFieldTypeInt, FieldLocation(cPsiState, m_maxPoints), },
};

static sStructDesc PsiStateStructDesc = 
   StructDescBuild(cPsiState, kStructFlagNone, psiStateFields);

IPsiStateProperty *CreatePsiStateProperty(sPropertyDesc *desc, 
                                                          ePropertyImpl impl)
{
   StructDescRegister(&PsiStateStructDesc);
   return new cPsiStateProperty(desc);
}

static sPropertyDesc psiStateDesc = 
{
   PROP_PSI_STATE_DESC, 
   kPropertyInstantiate,
   NULL, 
   1,    // version
   0,
   {"Psi", "State"},
};

sPsiState defaultPsiState =
{
   kPsiNone, 
   0,
   0,
};

cPsiState::cPsiState()
{
   *this = *(cPsiState*)(&defaultPsiState);
}

void PsiStatePropertyInit()
{
   g_pPsiStateProperty = CreatePsiStateProperty(&psiStateDesc, kPropertyImplDense);
}
 
sPsiState* PsiStateGet(ObjID objID)
{
   cPsiState *pPsiState;

   if (g_pPsiStateProperty->Get(objID, &pPsiState))
      return pPsiState;
   else
      return &defaultPsiState;
}

void PsiStateSetCurrentPower(ObjID objID, ePsiPowers power)
{
   cPsiState *pPsiState;

   if (g_pPsiStateProperty->Get(objID, &pPsiState))
   {
      pPsiState->m_currentPower = power;
      g_pPsiStateProperty->Set(objID, pPsiState);
   }
}

void PsiStateSetPoints(ObjID objID, float points)
{
   cPsiState *pPsiState;
   if (g_pPsiStateProperty->Get(objID, &pPsiState))
   {
      pPsiState->m_points = points;
      g_pPsiStateProperty->Set(objID, pPsiState);
   }
}

void PsiStateSetMaxPoints(ObjID objID, float maxPoints)
{
   cPsiState *pPsiState;
   if (g_pPsiStateProperty->Get(objID, &pPsiState))
   {
      pPsiState->m_maxPoints = maxPoints;
      g_pPsiStateProperty->Set(objID, pPsiState);
   }
}

/////////////////////////////////////////////////
IIntProperty *g_PsiOverloadProperty;
IIntProperty *g_PsiOverload2Property;

BOOL PsiOverloaded(ObjID obj, ePsiPowers which)
{
   int powers;
   // this is kind of grody...
   if (which < 32)
   {
      g_PsiOverloadProperty->Get(obj, &powers);
      return(powers & (1 << which));
   }
   else
   {
      g_PsiOverload2Property->Get(obj, &powers);
      return(powers & (1 << (which - 32)));
   }
}

void PsiSetOverload(ObjID obj, ePsiPowers which, BOOL val)
{
   int powers;
   if (which < 32)
   {
      g_PsiOverloadProperty->Get(obj, &powers);
      if (val)
         powers = powers | (1 << which);
      else
         powers = powers & (~(1 << which));
      g_PsiOverloadProperty->Set(obj, powers);
   }
   else
   {
      g_PsiOverload2Property->Get(obj, &powers);
      if (val)
         powers = powers | (1 << (which - 32));
      else
         powers = powers & (~(1 << which));
      g_PsiOverload2Property->Set(obj, powers);
   }
}

#define PROP_PSIOVERLOAD_DESC "PsiOverDesc"
static sPropertyDesc PsiOverloadDesc = { PROP_PSIOVERLOAD_DESC, 0, NULL, 1, 0, {"Player", "Overload State"}, };
#define PROP_PSIOVERLOAD2_DESC "PsiOver2Desc"
static sPropertyDesc PsiOverload2Desc = { PROP_PSIOVERLOAD2_DESC, 0, NULL, 1, 0, {"Player", "Overload State 2"}, };

/////////////////////////////////////////////////
IIntProperty *g_PsiRadarProperty;
#define PROP_PSIRADAR_DESC "PsiRadar"
static sPropertyDesc PsiRadarDesc = { PROP_PSIRADAR_DESC, 0, NULL, 1, 0, {"Psi", "Radar Type"}, };

static char* RadarNames[] =
{
   "None","Enemy", "Powerup",
};

static sFieldDesc RadarFields[] = 
{
   { "", kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, 3, 3, RadarNames},
};

static sPropertyTypeDesc RadarTypeDesc = {PROP_PSIRADAR_DESC, sizeof(int)}; 

static sStructDesc RadarStructDesc = 
{
   PROP_PSIRADAR_DESC, 
   sizeof(int),
   kStructFlagNone,
   sizeof(RadarFields)/sizeof(RadarFields[0]),
   RadarFields,
}; 

/////////////////////////////////////////////////

IFloatProperty *g_PsiAlchemyProperty;
#define PROP_PSIALCHEMY_DESC "Alchemy"
static sPropertyDesc PsiAlchemyDesc = { PROP_PSIALCHEMY_DESC, 0, NULL, 1, 0, {"Psi", "Alchemy Val"}, };

/////////////////////////////////////////////////

IIntProperty *g_PsiFabricateProperty;
#define PROP_PSIFabricate_DESC "Fabricate"
static sPropertyDesc PsiFabricateDesc = { PROP_PSIFabricate_DESC, 0, NULL, 1, 0, {"Psi", "Fabricate Quantity"}, };

/////////////////////////////////////////////////

IIntProperty *g_PsiFabCostProperty;
static sPropertyDesc PsiFabCostDesc = { "FabCost", 0, NULL, 1, 0, {"Psi", "Fabricate Cost"}, };

/////////////////////////////////////////////////

IBoolProperty *g_pPsiNotPullableProperty;
#define PROP_PSINOTPULL_DESC "NotPullable"
static sPropertyDesc PsiNotPullableDesc = { PROP_PSINOTPULL_DESC, 0, NULL, 1, 0, {"Psi", "NotPullable"}, };

/////////////////////////////////////////////////

static sFieldDesc g_Fields [] =
{
   { "1: Tags", kFieldTypeString, 
     FieldLocation(sESndTagList, m_TagStrings) },
};

static sStructDesc g_ListStruct
  = StructDescBuild(sESndTagList, kStructFlagNone, g_Fields);

static IESndTagProperty* g_pESndPsiProp = NULL;

struct sOpsList : public sESndTagList 
{
   sOpsList()
   {
      m_pTagSet = new cTagSet; 
      m_TagStrings[0] = '\0'; 
   } 

   sOpsList(const sESndTagList& dat)
   {
      strncpy(m_TagStrings,dat.m_TagStrings,sizeof(m_TagStrings)); 
      m_pTagSet = new cTagSet(m_TagStrings); 
   }

   sOpsList(const sOpsList& dat)
   {
      strncpy(m_TagStrings,dat.m_TagStrings,sizeof(m_TagStrings)); 
      m_pTagSet = new cTagSet(m_TagStrings); 
   }

   sOpsList& operator=(const sOpsList& dat)
   {
      strncpy(m_TagStrings,dat.m_TagStrings,sizeof(m_TagStrings)); 
      m_pTagSet->FromString(m_TagStrings); 
      return *this; 
   }

   ~sOpsList()
   {
      delete m_pTagSet; 
   }

} ; 

class cESndTagOps : public cClassDataOps<sOpsList>
{
   typedef cClassDataOps<sOpsList> cParent; 
public: 

   cESndTagOps() : cParent(kNoFlags) {} ; 
   STDMETHOD(Read)(THIS_ sDatum* dat, IDataOpsFile* file, int version)
   {
      if (!dat->value)
         *dat = New(); 
      sOpsList* list = (sOpsList*)dat->value; 
      sESndTagList tmp;
      memset(&tmp,0,sizeof(tmp)); 
      // we derive from cClassDataOps, thus version == size; 
      int sz = (version < sizeof(tmp)) ? version : sizeof(tmp); 
      Verify(file->Read(&tmp,sz) == sz); 
      *list = tmp; 
      return S_OK; 
   }

}; 

class cESndTagStore : public cListPropertyStore<cESndTagOps>
{
}; 

class cESndTagProperty : public cSpecificProperty<IESndTagProperty,
   &IID_IESndTagProperty, sESndTagList*, cESndTagStore>

{
   typedef cSpecificProperty<IESndTagProperty,
                             &IID_IESndTagProperty,
                             sESndTagList*,
                             cESndTagStore> cParent;

public: 
   cESndTagProperty(const sPropertyDesc* desc)
      : cParent(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(sESndTagList); 
}; 


static sPropertyDesc g_PsiPropDesc =
{
   PROP_ESND_PSI_NAME,
   0,
   NULL,
   0,0, // verison
   { "Schema", "Psi Tags" }, 
};

EXTERN BOOL ObjGetESndPsi(ObjID obj, sESndTagList **pData)
{
   Assert_(g_pESndPsiProp);
   return g_pESndPsiProp->Get(obj, pData);
}


EXTERN BOOL ObjSetESndPsi(ObjID obj, sESndTagList *pData)
{
   Assert_(g_pESndPsiProp);
   return g_pESndPsiProp->Set(obj, pData);
}

/////////////////////////////////////////////////

void PsiPropertiesInit(void)
{
   PsiPowerPropertyInit();
   PsiStatePropertyInit();
   PsiShieldPropertyInit();
   g_PsiOverloadProperty = CreateIntProperty(&PsiOverloadDesc, kPropertyImplVerySparse);
   g_PsiOverload2Property = CreateIntProperty(&PsiOverload2Desc, kPropertyImplVerySparse);

   StructDescRegister(&RadarStructDesc);
   g_PsiRadarProperty = CreateIntegralProperty(&PsiRadarDesc, &RadarTypeDesc, kPropertyImplLlist); // kPropertyImplDense);

   g_PsiAlchemyProperty = CreateFloatProperty(&PsiAlchemyDesc, kPropertyImplDense);
   g_PsiFabricateProperty = CreateIntProperty(&PsiFabricateDesc, kPropertyImplDense);
   g_PsiFabCostProperty = CreateIntProperty(&PsiFabCostDesc, kPropertyImplDense);
   g_pPsiNotPullableProperty = CreateBoolProperty(&PsiNotPullableDesc, kPropertyImplDense);

   g_pESndPsiProp = new cESndTagProperty(&g_PsiPropDesc);

   // links
   g_pPsiTeleportLinks = CreateStandardRelation(&psiTeleportDesc, &noDataDesc, (kQCaseSetSourceKnown));
}

/////////////////////////////////////////////////

void PsiPropertiesTerm(void)
{
   SafeRelease(g_pPsiPowerProperty);
   SafeRelease(g_pPsiStateProperty);
   SafeRelease(g_pPsiShieldProperty);
   SafeRelease(g_PsiOverloadProperty);
   SafeRelease(g_PsiOverload2Property);
   SafeRelease(g_PsiRadarProperty);
   SafeRelease(g_PsiAlchemyProperty);
   SafeRelease(g_PsiFabricateProperty);
   SafeRelease(g_pPsiTeleportLinks);
   SafeRelease(g_pPsiNotPullableProperty);
   SafeRelease(g_pESndPsiProp);
}
