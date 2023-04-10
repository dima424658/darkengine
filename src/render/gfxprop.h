/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/gfxprop.h,v 1.2 2000/01/29 13:38:55 adurant Exp $
#pragma once

/*=========================================================

  Created:  3/24/99 3:05:00 PM

  File:  gfxprop.h

  Description:  


=========================================================*/

#ifndef _GFXPROP_H
#define _GFXPROP_H


//_____INCLUDES_AND_DEFINITIONS___________________________

#include <property.h>

// Z-Bias property

// Z-bias property name
#define Z_BIAS_PROPERTY_NAME    "Z-Bias"

#define BUMP_PROPERTY_NAME    "Bump Map"


//______EXPORTED_DATA_____________________________________


// Z-Bias property

EXTERN void ZBiasPropertyInit( void );
 
//legal values are integers in the interval [ 0, 16 ]

EXTERN BOOL ObjGetZBiasProp( ObjID obj, int* psl );
EXTERN BOOL ObjSetZBiasProp( ObjID obj, int sl );



// Bump Mapping property

EXTERN void BumpPropertyInit(void);

EXTERN BOOL ObjGetBumpProp( ObjID obj );

EXTERN BOOL ObjSetBumpProp( ObjID obj, BOOL bBump );

#endif //_GFXPROP_H