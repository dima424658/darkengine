/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkrsprp.h,v 1.3 2000/01/31 09:59:06 adurant Exp $
#pragma once

#ifndef _SHKRSPRP_H
#define _SHKRSPRP_H

// Setting text properties
#define PROP_RESEARCHTIME_NAME "RsrchTime"
EXTERN IIntProperty* gPropResearchTime;

#define PROP_RESEARCHTEXT_NAME "RsrchTxt"
EXTERN IStringProperty* gPropResearchText;

#define MAX_CHEM_ITEMS  7
typedef struct sChemInfo
{
   char m_chem[MAX_CHEM_ITEMS][64];
   int m_time[MAX_CHEM_ITEMS];
} sChemInfo;

// ChemInfo description property
F_DECLARE_INTERFACE(IChemInfoProperty);

#undef INTERFACE
#define INTERFACE IChemInfoProperty

DECLARE_PROPERTY_INTERFACE(IChemInfoProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sChemInfo*);  // Type-specific accessors, by reference
};
#define PROP_CHEMNEED_NAME  "ChemNeeded"
EXTERN IChemInfoProperty *gPropChemNeeded;

#endif