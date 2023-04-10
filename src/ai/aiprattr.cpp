/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiprattr.cpp,v 1.5 1998/11/21 14:19:25 MROWLEY Exp $
//
//

#include <lg.h>

#include <dataops_.h>
#include <propbase.h>
#include <property.h>
#include <propert_.h>
#include <prophash.h>
#include <sdesbase.h>
#include <sdesc.h>

#include <aiprops.h>
#include <aiprattr.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

IAIRatingProperty * g_pAIVisionStatProp;
IAIRatingProperty * g_pAIHearingStatProp;

// combat uses these 6
IAIRatingProperty * g_pAIAggressionStatProp;
IAIRatingProperty * g_pAIDodginessStatProp;
IAIRatingProperty * g_pAISlothStatProp;
IAIRatingProperty * g_pAIVerbosityStatProp;
IAIRatingProperty * g_pAIDefensiveStatProp;
IAIRatingProperty * g_pAIAptitudeStatProp;

///////////////////////////////////////////////////////////////////////////////
//
// AI Rating type sdesc
//

static const char * _g_ppszAIRatings[kAIRT_Num] = 
{
   "Null",                                       // kAIRT_Null
   "Well Below Average",                         // kAIRT_WellBelowAvg
   "Below Average",                              // kAIRT_BelowAvg
   "Average",                                    // kAIRT_Avg
   "Above Average",                              // kAIRT_AboveAvg
   "Well Above Average",                         // kAIRT_WellAboveAvg
};

const char * AIGetRatingName(eAIRating rating)
{
   return _g_ppszAIRatings[rating];
}

///////////////////////////////////////

static sFieldDesc _g_AIRatingFieldDesc[] = 
{
   { "", kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, kAIRT_Num, kAIRT_Num, _g_ppszAIRatings },
}; 

static sStructDesc _g_AIRatingStructDesc = 
{
   "eAIRating",
   sizeof(int),
   kStructFlagNone,
   sizeof(_g_AIRatingFieldDesc)/sizeof(_g_AIRatingFieldDesc[0]),
   _g_AIRatingFieldDesc,
}; 

///////////////////////////////////////

class cAIRatingDataOps : public cSimpleDataOps
{
public:
   STDMETHOD_(sDatum,New)() 
   { 
      return (void *)(kAIRT_Avg); 
   }
};

///////////////////////////////////////

typedef cSpecificProperty<IAIRatingProperty, &IID_IAIRatingProperty, eAIRating, cHashPropertyStore< cAIRatingDataOps > > cAIRatingPropertyBase;

class cAIRatingProperty : public cAIRatingPropertyBase
{
public:

   cAIRatingProperty(const sPropertyDesc * pDesc)
      : cAIRatingPropertyBase(pDesc)
   {
   }

   STANDARD_DESCRIBE_TYPE(eAIRating); 
};

///////////////////////////////////////
//
// Property descs for attributes
//

#define DeclareAttributePropertDesc(Attrib) \
   static sPropertyDesc g_AI##Attrib##PropertyDesc = \
   { \
      "AI_" #Attrib , \
      0, \
      NULL, 0, 0, \
      { AI_ATTRIB_CAT, #Attrib }, \
      kPropertyChangeLocally, \
   }

DeclareAttributePropertDesc(Vision);
DeclareAttributePropertDesc(Hearing);

DeclareAttributePropertDesc(Aggression);
DeclareAttributePropertDesc(Dodginess);
DeclareAttributePropertDesc(Sloth);
DeclareAttributePropertDesc(Verbosity);
DeclareAttributePropertDesc(Defensive);
DeclareAttributePropertDesc(Aptitude);

#define CreateAttributeProperty(Attrib) \
   new cAIRatingProperty(&g_AI##Attrib##PropertyDesc)

///////////////////////////////////////////////////////////////////////////////
//
// Initialization and cleanup
//

void AIInitAttrProps()
{
   StructDescRegister(&_g_AIRatingStructDesc);
   
   g_pAIVisionStatProp     = CreateAttributeProperty(Vision);
   g_pAIHearingStatProp    = CreateAttributeProperty(Hearing);

   g_pAIAggressionStatProp = CreateAttributeProperty(Aggression);
   g_pAIDodginessStatProp  = CreateAttributeProperty(Dodginess);
   g_pAISlothStatProp      = CreateAttributeProperty(Sloth);
   g_pAIVerbosityStatProp  = CreateAttributeProperty(Verbosity);
   g_pAIDefensiveStatProp  = CreateAttributeProperty(Defensive);
   g_pAIAptitudeStatProp   = CreateAttributeProperty(Aptitude);
}

///////////////////////////////////////

void AITermAttrProps()
{
   SafeRelease(g_pAIVisionStatProp);
   SafeRelease(g_pAIHearingStatProp);

   SafeRelease(g_pAIAggressionStatProp);
   SafeRelease(g_pAIDodginessStatProp);
   SafeRelease(g_pAISlothStatProp);
   SafeRelease(g_pAIVerbosityStatProp);   
   SafeRelease(g_pAIDefensiveStatProp);
   SafeRelease(g_pAIAptitudeStatProp);   
}

///////////////////////////////////////////////////////////////////////////////

