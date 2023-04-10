/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/physics/phlistyp.h,v 1.2 2000/01/29 13:32:49 adurant Exp $
#pragma once

#ifndef __PHLISTYP_H
#define __PHLISTYP_H

#include <types.h>

#include <objtype.h>

// Handle for listener
typedef int PhysListenerHandle;

// Data For listener function
typedef void *PhysListenerData;

// Our actual message
typedef struct sPhysListenMsg sPhysListenMsg;  

// Message sets
typedef int PhysListenMsgSet;

// Listener function: 
typedef void (LGAPI * PhysListenFunc)(ObjID, const sPhysListenMsg*, PhysListenerData);

#endif