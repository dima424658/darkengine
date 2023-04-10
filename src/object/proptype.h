/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/proptype.h,v 1.8 2000/01/29 13:24:46 adurant Exp $
#pragma once

#ifndef PROPTYPE_H
#define PROPTYPE_H
#include <osystype.h>

#ifdef __cplusplus
# define CONSTFUNC const
#else
# define CONSTFUNC
#endif

//------------------------------------------------------------
// FORWARD-DECLARED PROPERTY TYPES
//

//
//    Unique integer property ID
//
typedef int PropertyID;

#define PROPID_NULL (-1)

//
//    Property descriptor structure
//
typedef struct sPropertyDesc sPropertyDesc;

//
//    Enum of popular property implementations 
//
typedef ulong ePropertyImpl;

//------------------------------------------------------------
// TYPES FOR NOTIFICATION
//

//
//    Type enum of Notify messages:
//
typedef eObjNotifyMsg ePropertyNotifyMsg;
typedef ObjNotifyData PropNotifyData;


//------------------------------------------------------------
// TYPES FOR LISTENERS
//

//
//    Struct for listen messages
//
typedef struct sPropertyListenMsg sPropertyListenMsg;  

//
// Handle for listener
//
DECLARE_HANDLE(PropListenerHandle);

//
//    Satellite data For listener function
//
DECLARE_HANDLE(PropListenerData);

//
//    Listener function: 
//
typedef void (LGAPI * PropertyListenFunc)(sPropertyListenMsg* msg, PropListenerData data);

//
//    A set of ePropertyListenMsg values, or'ed together: 
//
typedef ulong PropertyListenMsgSet;

//------------------------------------------------------------
// TYPES FOR ITERATION
//

//
// Iterator state structure
//
typedef struct sPropertyObjIter sPropertyObjIter;

//------------------------------------------------------------
// TYPE DESCRIPTION TYPES
//

typedef struct sPropertyTypeDesc sPropertyTypeDesc;

//------------------------------------------------------------
// PROPERTY STORE TYPES
//

typedef struct sPropertyStoreDesc sPropertyStoreDesc; 

//------------------------------------------------------------
// PROPERTY NETWORKING TYPES
//

typedef struct sPropertyNetMsg sPropertyNetMsg;  

#endif // PROPTYPE_H

