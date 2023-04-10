/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/stimprop.h,v 1.3 2000/01/29 12:44:47 adurant Exp $
#pragma once

#ifndef __STIMPROP_H
#define __STIMPROP_H


////////////////////////////////////////////////////////////
// Act/React properties
//

//
// "Source Scale" Property, an IFloatProperty that scales all sources on 
// this object.  
//

#define PROP_SOURCE_SCALE_NAME "arSrcScale"

EXTERN IFloatProperty* g_pSourceScaleProperty;

#endif // __STIMPROP_H
