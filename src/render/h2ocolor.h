/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/h2ocolor.h,v 1.1 1998/09/16 00:17:07 mahk Exp $
#pragma once  
#ifndef __H2OCOLOR_H
#define __H2OCOLOR_H
#include <property.h>

////////////////////////////////////////////////////////////
// FLOW COLOR PROPERTY
//

struct sRGBA 
{
   uchar rgb[4]; // the fourth is pad 
   float alpha; 
}; 

struct sTLucTable
{
   uchar table[256][256]; 
};

typedef struct sRGBA sRGBA; 

//
// Property interface
//
#undef INTERFACE
#define INTERFACE IWaterColorProperty
DECLARE_PROPERTY_INTERFACE(IWaterColorProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(const sRGBA*); 

   //
   // Map a texture index into a water color 
   //
   STDMETHOD_(const sRGBA*,ForTexture)(THIS_ int idx) PURE; 
}; 

#undef INTERFACE

#define PROP_WATER_COLOR_NAME "WaterColor" 
#define PROP_FLOW_COLOR_NAME "FlowColor"

EXTERN void WaterColorPropInit(); 
EXTERN void WaterColorPropTerm(); 

// How many different flow colors do we support

enum 
{
   kNumWaterBanks = 4, 
};

// 
// Get RGBA for water, given a bank
//
EXTERN const sRGBA* GetWaterColorForBank(int i); 
EXTERN void WaterColorSetupTextures(void); 

//
// Get water bank idx from flow group idx
//
EXTERN int GetWaterBankForFlow(int idx); 


#endif // __H2OCOLOR_H




