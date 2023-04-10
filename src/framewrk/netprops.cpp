/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/netprops.cpp,v 1.11 1999/06/22 19:31:29 Justin Exp $

#include <lg.h>
#include <mprintf.h>
#include <comtools.h>
#include <appagg.h>

#include <netprops.h>
#include <propbase.h>
#include <dataops_.h> // for cClassDataOps
#include <propsprs.h> // for cSparseHashPropertyStore
#include <propert_.h>

#include <sdesbase.h>
#include <sdestool.h>


// Must be last header 
#include <dbmem.h>


////////////////////////////////////////////////////////////
// NETWORK CATEGORY PROPERTY
//

static sPropertyDesc netcat_prop_desc =
{
   PROP_NETWORK_CATEGORY_NAME,
   0,  // flags
   NULL, // constraints
   0, 0, // Version
   { "Networking", "Network Category", }, // ui strings
   kPropertyChangeLocally,
};

//
// Network Category sdesc 
//
static char* network_category_strings[] = 
{ 
   "Local Only",
   "Hosted",
   "Obsolete",
};

#define NUM_NETCAT_STRINGS (sizeof(network_category_strings)/sizeof(network_category_strings[0]))

#define NETCAT_TYPENAME "eNetworkCategory"

static sFieldDesc netcat_field[] = 
{
   { "", kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, NUM_NETCAT_STRINGS, NUM_NETCAT_STRINGS, network_category_strings },
}; 

static sStructDesc netcat_sdesc = 
{
   NETCAT_TYPENAME, 
   sizeof(int),
   kStructFlagNone,
   sizeof(netcat_field)/sizeof(netcat_field[0]),
   netcat_field,
}; 

static sPropertyTypeDesc netcat_tdesc = 
{
   NETCAT_TYPENAME,
   sizeof(int),
};

IIntProperty* gNetworkCategoryProp = NULL;

eNetworkCategory NetworkCategory(ObjID obj)
{
   Assert_(gNetworkCategoryProp);
   eNetworkCategory result = kLocalOnly;
   gNetworkCategoryProp->Get(obj,&result);
   return result;
}

void SetNetworkCategory(ObjID obj,eNetworkCategory cat)
{
   Assert_(gNetworkCategoryProp);
   gNetworkCategoryProp->Set(obj,cat);
}

static void NetworkCategoryPropInit()
{
   AutoAppIPtr_(StructDescTools,pTools);
   pTools->Register(&netcat_sdesc); 
   gNetworkCategoryProp = CreateIntegralProperty(&netcat_prop_desc,&netcat_tdesc,kPropertyImplSparseHash);
}

////////////////////////////////////////////////////////////
//
// HEARTBEAT RATE PROPERTY
//

// the IIntProperty interface
// F_DECLARE_INTERFACE(IIntProperty); 

//
// Property Descriptor for Heartbeat rate
//
// This specifies the name and special flags for the Heartbeat property.
//

static sPropertyDesc HeartbeatDesc =
{
   PROP_HEARTBEAT_NAME, // Name 
   0,  // flags
   NULL, 0, 0,
   { "Networking", "Heartbeat", "Approximate number of millisecs between position updates" },
   kPropertyChangeLocally, // net_flags
}; 

//
// Property Store for Heartbeat
//
// We have chosen the generic "dense" store
//

#define HEARTBEAT_IMPL kPropertyImplDense

//
// Create the heartbeat property
//

IIntProperty* gPropHeartbeat = NULL;

void HeartbeatPropInit(void) 
{
   gPropHeartbeat = CreateIntProperty(&HeartbeatDesc,HEARTBEAT_IMPL);
   // Don't release, we're saving that pointer! 
}

void HeartbeatPropTerm(void)
{
   SafeRelease(gPropHeartbeat);
}

//
// ACCESSORS
//

int ObjGetHeartbeat(ObjID obj)
{
   Assert_(gPropHeartbeat);
   int rate = 0; // default value
   gPropHeartbeat->Get(obj,&rate);
   return rate;
}

void ObjSetHeartbeat(ObjID obj, int rate)
{
   Assert_(gPropHeartbeat);
   gPropHeartbeat->Set(obj, rate);
}

/////////////////////////////////////////////////
// AvatarHeightOffset Property
//

static sPropertyDesc AvatarHeightDesc =
{
   PROP_AVATAR_HEIGHT_OFFSET_NAME, // Name 
   0,  // flags
   NULL, 0, 0,
   { "Networking", "Avatar Height Offset", 
     "Difference in height between player & this obj as an avatar" },
}; 

IFloatProperty *gAvatarHeightOffsetProp = NULL;

/////////////////////////////////////////////////
// LocalCopy Property
//

static sPropertyDesc LocalCopyDesc =
{
   PROP_LOCAL_COPY_NAME, // Name 
   kPropertyNoEdit,  // flags
   NULL, 0, 0,
   { "Networking", "Local Copy", 
     "TRUE iff this object is a local pseudo-proxy for a remote obj" },
   kPropertyChangeLocally,
}; 

IBoolProperty *gLocalCopyProp = NULL;

/////////////////////////////////////////////////
// DistinctAvatar Property
//

static sPropertyDesc DistinctAvatarDesc =
{
   PROP_DISTINCT_AVATAR_NAME,
   0, // flags
   NULL, 0, 0,
   { "Networking", "Distinct Avatar",
     "TRUE iff this object is used to proxy a different kind of object" },
   0, // net flags
};

IBoolProperty *gDistinctAvatarProp = NULL;

/////////////////////////////////////////////////
// HandoffNumber Property
//

static sPropertyDesc HandoffNumberDesc =
{
   PROP_HANDOFF_NUMBER_NAME,
   kPropertyNoEdit, // flags
   NULL, 0, 0,
   { "Networking", "Handoff Number",
     "The number of times this object has been handed off" },
   kPropertyChangeLocally, // net flags
};

IIntProperty *gHandoffNumberProp = NULL;

////////////////////////////////////////////////
// INITIALIZATION

void NetPropsInit(void)
{
   NetworkCategoryPropInit();
   HeartbeatPropInit();
   // HostObjIDInit();
   gAvatarHeightOffsetProp = CreateFloatProperty(&AvatarHeightDesc, kPropertyImplSparseHash);
   gLocalCopyProp = CreateBoolProperty(&LocalCopyDesc, kPropertyImplSparseHash);
   gDistinctAvatarProp = CreateBoolProperty(&DistinctAvatarDesc, kPropertyImplSparseHash);
   gHandoffNumberProp = CreateIntProperty(&HandoffNumberDesc, kPropertyImplSparseHash);
}

void NetPropsShutdown(void)
{
   SafeRelease(gNetworkCategoryProp);
   SafeRelease(gLocalCopyProp);
   SafeRelease(gDistinctAvatarProp);
   SafeRelease(gHandoffNumberProp);
   HeartbeatPropTerm();
}
