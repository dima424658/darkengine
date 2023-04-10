/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propbase.h,v 1.24 2000/01/29 13:24:15 adurant Exp $
#pragma once

#ifndef PROPBASE_H
#define PROPBASE_H

#include <property.h>
#include <nettypes.h> // for tNetMsgHandlerID

/////////////////////////////////////////////////////////////
// PROPERTY SYSTEM BASE TYPES
//
F_DECLARE_INTERFACE(ITagFile); 

//------------------------------------------------------------
// Property Flags 
// 
enum ePropertyFlags_
{
   kPropertyNoInherit = 0x0001,   // don't ever inherit this from metaproperties
   kPropertyDerived   = 0x0002,   // This property's value is derived ("set" method is meaningless)
   kPropertyConcrete  = 0x0004,   // This property should only exist for "concrete" objects 
   kPropertyTransient = 0x0008,   // Property is transient, don't save.  
   kPropertyInstantiate=0x0010,   // Instantiate instead of inheriting 
   kPropertyNoCache   = 0x0020,   // don't use inheritance cache
   kPropertyNoClone   = 0x0040,   // Don't copy this property when "cloning" an object   
   kPropertyNoEdit    = 0x0080,   // Don't show this property in the editor
   kPropertySendBeginCreate    = 0x0100,  // This property needs to be notified when objects are created 
   kPropertySendEndCreate      = 0x0200,  // This property needs to be notified when objects are created 
   kPropertySendLoadObj        = 0x0400,  // This property needs to be notified when objects are loaded 



};

typedef ulong ePropertyFlags;

//------------------------------------------------------------
// Networking Flags 
// 
enum ePropertyNetworkingFlags_
{
   kPropertyChangeLocally     = 0x0001, // Hosts & proxies make changes locally (no net msgs).
   kPropertyNonGuaranteedMsgs = 0x0002, // Change broadcasts don't need guaranteed messaging.
   kPropertyProxyChangable    = 0x0004, // Proxy machines can initiate changes to this property.
};

typedef ulong ePropertyNetworkingFlags;

//------------------------------------------------------------
// Constraints
//

enum ePropertyConstrantKind_ 
{
   kPropertyNullConstraint, // Terminator
   kPropertyRequires, // I must have the required property 
   kPropertyAutoCreate,  // Create the implied property when this one is created
};

typedef ulong ePropertyConstraintKind;


struct sPropertyConstraint 
{
   ePropertyConstraintKind kind;    // kind of constraint (above)
   const char* against;             // against which property (by name)
};

typedef struct sPropertyConstraint sPropertyConstraint;


//------------------------------------------------------------
// Property Descriptor
// 

typedef ulong tPropertyVersion;

struct sPropertyDesc
{
   char              name[16];   // What is the name of this property (e.g., "position")
   ePropertyFlags    flags;      // special flags (as above)
   const sPropertyConstraint* constraints;  // null-terminated array of constraints
   tPropertyVersion version;     // current property version   
   tPropertyVersion oldest;      // oldest acceptable version (zero means current version)
   struct sPropertyUIData
   {
      const char* category;       // Property category
      const char* friendly_name;  // Human-readable name
      const char* help_text;   // A text description of the property 
   } ui;
   ePropertyNetworkingFlags net_flags;  // flags r.e. synchronization in multi-player games.
};


//------------------------------------------------------------
// Property Listener Messages
//

enum ePropertyListenMsg_
{
   kListenPropModify          = 1 << 0,   // Property has been modified for obj
   kListenPropSet             = 1 << 1,   // Property just became relevant for obj
   kListenPropUnset           = 1 << 2,   // Property just became irrelevant for obj
   kListenPropLoad            = 1 << 3,   // During load or post-load
   kListenPropRebuildConcrete = 1 << 4,   // I'm a concrete object, and my inherited 
                                          // value may have changed.
   kListenPropRebuildConcreteRelevant = 1 << 5, // Set on RebuildConcrete messages 
                                          // in which the property is still relevant
   kListenPropRequestFromHost  = 1 << 6,  // Network game proxy requesting change on host.
                                          // Must be paired with another flag.
};

typedef ulong ePropertyListenMsg; 

//------------------------------------------------------------
// Actual listener message structure
//
union uPropListenerValue
{
   void* ptrval; 
   int   intval; 
}; 

typedef union uPropListenerValue uPropListenerValue; 

struct sPropertyListenMsg 
{
   ePropertyListenMsg type;  // type of message
   PropertyID property;      // property sending message
   ObjID    obj;             // Object message is about
   uPropListenerValue    value;   // new value of property for object
   ObjID donor;              // My donor, for rebuildconcrete only
};


//------------------------------------------------------------
// Property object iteration state
//

struct sPropertyObjIter 
{
   ObjID next;
   void* state[4]; // who knows what state you need?
};


//------------------------------------------------------------
// Stock Property implementations
// 
 enum ePropertyImpl_
{
   kPropertyImplBoolean, // special impl for booleans
   kPropertyImplLlist,
   kPropertyImplHash, 
   kPropertyImplSparseHash,
   kPropertyImplBigArray, 

   kPropertyImplSparse = kPropertyImplLlist,
   kPropertyImplVerySparse = kPropertyImplSparseHash,
   kPropertyImplDense = kPropertyImplHash,
   kPropertyImplVeryDense = kPropertyImplBigArray,
};



//------------------------------------------------------------
// TYPE DESCRIPTION
//

#define PROP_TYPE_NAME_LEN 32
struct sPropertyTypeDesc 
{
   char type[PROP_TYPE_NAME_LEN];
   ulong size;
};

//------------------------------------------------------------
// Property store description
//

struct sPropertyStoreDesc
{
   const char* friendly_name;  
}; 

#endif // PROPBASE_H


