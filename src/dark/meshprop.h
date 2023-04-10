/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/meshprop.h,v 1.1 1999/11/02 16:15:39 adurant Exp $
// lockpick service, properties, the whole thing...

#pragma once
#ifndef __MESHPROP_H
#define __MESHPROP_H

#include <matrixs.h>
#include <meshatt.h>
#include <meshatts.h>

#define PROP_BELT_LINK_NAME "BeltLink"

EXTERN void BeltLinkInit(void);
EXTERN void BeltLinkTerm(void);
EXTERN sMeshAttachInstance GetBeltLinkAttachment(ObjID o);

typedef struct
{
  int meshjoint;
  int xrotation;
  int yrotation;
  int zrotation;
  mxs_vector vecoffset;
} sBeltLink;

#undef INTERFACE
#define INTERFACE IBeltLinkProperty
DECLARE_PROPERTY_INTERFACE(IBeltLinkProperty)
{
  DECLARE_UNKNOWN_PURE();
  DECLARE_PROPERTY_PURE();
  DECLARE_PROPERTY_ACCESSORS(sBeltLink*);
};



#define PROP_ALT_LINK_NAME "AltLink"

EXTERN void AltLinkInit(void);
EXTERN void AltLinkTerm(void);
EXTERN sMeshAttachInstance GetAltLinkAttachment(ObjID o);

typedef struct
{
  int meshjoint;
  int xrotation;
  int yrotation;
  int zrotation;
  mxs_vector vecoffset;
} sAltLink;

#undef INTERFACE
#define INTERFACE IAltLinkProperty
DECLARE_PROPERTY_INTERFACE(IAltLinkProperty)
{
  DECLARE_UNKNOWN_PURE();
  DECLARE_PROPERTY_PURE();
  DECLARE_PROPERTY_ACCESSORS(sAltLink*);
};

#endif  // !__MESHPROP_H
