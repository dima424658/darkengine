/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef _DPCHAZPR_H
#define _DPCHAZPR_H

#ifndef __PROPFAC__H
#include <propfac_.h>
#endif // !__PROPFAC__H

#ifndef PROPBASE_H
#include <propbase.h>
#endif // ! PROPBASE_H

// current ambient rad level
#define PROP_RADAMBIENT_NAME "RadAmb"
EXTERN IFloatProperty *gPropRadAmbient;

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

#endif  // ! _DPCHAZPR_H
