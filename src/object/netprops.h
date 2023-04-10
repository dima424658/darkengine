/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/netprops.h,v 1.9 2000/01/29 13:23:51 adurant Exp $
#pragma once

#ifndef __NETPROPS_H
#define __NETPROPS_H

#include <objtype.h>

#include <property.h>
#include <propface.h>
#include <objtype.h>  // for NetObjID
#include <iobjnet.h>  // for eNetworkCategory

////////////////////////////////////////////////////////////
//
// NETWORK CATEGORY PROPERTY
//

// The possible values for the network category; these are the values
// that can be returned by ObjNetworkCategory.
// When the property is not set, it is assumed to be kLocalOnly
enum eNetworkCategoryEnum {
   // Object is created on all machines, not synchronized or hosted:
   kLocalOnly,
   // Objects loaded from missions are hosted by the DefaultHost, run-time
   // objects are hosted by whatever object is in the proxy table.
   kHosted,
};
typedef int eNetworkCategory;

#define PROP_NETWORK_CATEGORY_NAME "NetworkCategory"

EXTERN eNetworkCategory NetworkCategory(ObjID obj);
EXTERN void             SetNetworkCategory(ObjID obj, eNetworkCategory cat);

////////////////////////////////////////////////////////////
//
// HEARTBEAT RATE PROPERTY 
//
// The value of this property determines how often this object will send
// out "heartbeat" updates to the other players. It should be smaller for
// objects whose movement is less predictable and more relevant (ie, AIs
// that are close to players), and larger for objects whose movement is
// more predictable (ie, projectiles). If set to zero, the object will
// never send a heartbeat. The unit is simulation time, and is more or
// less milliseconds.
//

// property name
#define PROP_HEARTBEAT_NAME "Heartbeat"

// Accessors

EXTERN int ObjGetHeartbeat(ObjID obj);
EXTERN void ObjSetHeartbeat(ObjID obj, int rate);

///////////////////////////////////////
//
// HostObjID PROPERTY
//
// A "host object ID", which describes who owns the real object
// that this proxy belongs to, and its objID on that host.
//
// @NOTE: The property itself is now gone; only the struct itself is left.
// The property has been replaced by a relation, which is hidden entirely
// within objnet.
//

typedef struct sHostObjID {
   ObjID host;
   NetObjID netObjID; // ObjID on m_host, not necessarily a valid ObjID here.
} sHostObjID;

/////////////////////////////////////////////////
// AvatarHeightOffset Property
//

#define PROP_AVATAR_HEIGHT_OFFSET_NAME "AvatarZOffset"
EXTERN IFloatProperty *gAvatarHeightOffsetProp;

/////////////////////////////////////////////////
//
// Network Local Copy Property
//
// This property is mainly a convenience, to allow us to record that
// certain objects are "local-only copies" of others. There are times
// that we prefer not to have proper proxies, but rather separate
// copies of an object -- this sometimes makes sense for short-lived
// objects like bullets or special effects.
//

#define PROP_LOCAL_COPY_NAME "LocalCopy"
EXTERN IBoolProperty *gLocalCopyProp;

/////////////////////////////////////////////////
//
// Distinct Avatar Property
//
// This should be set on any object whose "avatar" -- that is, its
// proxy representation -- has a different archetype than the hosted
// object.

#define PROP_DISTINCT_AVATAR_NAME "DistinctAvatar"
EXTERN IBoolProperty *gDistinctAvatarProp;

/////////////////////////////////////////////////
//
// Handoff Number Property
//
// This is a serial number of the times that this object has been handed
// off from machine to machine. We use it so we can reject obsolete handoff
// messages that arrive out of order.

#define PROP_HANDOFF_NUMBER_NAME "HandoffNumber"
EXTERN IIntProperty *gHandoffNumberProp;

/////////////////////////////
//
// Initialization
//

EXTERN void NetPropsInit(void);
EXTERN void NetPropsShutdown(void);

#endif // __NETPROPS_H


