/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkhazpr.h,v 1.2 2000/01/31 09:56:35 adurant Exp $
#pragma once

#include <propfac_.h>
#include <propbase.h>

// current ambient rad level
#define PROP_RADAMBIENT_NAME "RadAmb"
EXTERN IFloatProperty *gPropRadAmbient;

// personal radiation level
#define PROP_RADLEVEL_NAME "RadLevel"
EXTERN IFloatProperty *gPropRadLevel;

// ambient radiation dropoff rate
#define PROP_RADDRAIN_NAME "RadDrain"
EXTERN IFloatProperty *gPropRadDrain;

// base radiation absorption rate
#define PROP_RADABSORB_NAME "RadAbsorb"
EXTERN IFloatProperty *gPropRadAbsorb;

// base radiation absorption rate
#define PROP_RADRECOVER_NAME "RadRecover"
EXTERN IFloatProperty *gPropRadRecover;

// Toxin level
#define PROP_TOXIN_NAME "Toxin"
EXTERN IFloatProperty *gPropToxin;
