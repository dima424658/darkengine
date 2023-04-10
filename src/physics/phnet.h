/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/physics/phnet.h,v 1.12 2000/01/31 09:52:01 adurant Exp $
#pragma once

#ifndef _PHYSNET_H
#define _PHYSNET_H

#include <lg.h>
#include <phystyp2.h>
#include <phscrt.h>
#include <matrix.h>
#include <phmod.h>

EXTERN void PhysNetInit();
EXTERN void PhysNetTerm();

// This should get called once per frame, to check heartbeats and stuff:
EXTERN void PhysNetFrame();
// This should get called after we load a file:
EXTERN void PhysNetPostLoad();

// Dispatch messages to clients that an object has been registered or
// deregistered with physics:
EXTERN void PhysBroadcastRegisterSphereHat(ObjID objID, 
                                           unsigned flags, 
                                           mxs_real radius);
EXTERN void PhysBroadcastRegisterSphere(ObjID objID, 
                                        tPhysSubModId numSubModels, 
                                        unsigned flags, 
                                        mxs_real radius);
EXTERN void PhysBroadcastRegisterOBB(ObjID objID, unsigned flags);
EXTERN void PhysBroadcastDeregister(ObjID objID);

// Dispatch message to clients about changes in a door's state:
EXTERN void PhysBroadcastDoor(ObjID door, 
                              int what_happened, 
                              int what_was_happening);

// Control velocity broadcasting
EXTERN void PhysBroadcastControlVelocity(ObjID door, mxs_vector *velocity);
EXTERN void PhysBroadcastAxisControlVelocity(ObjID door, 
                                             int axis, 
                                             mxs_real speed);
EXTERN void PhysBroadcastControlRotationalVelocity(ObjID door, 
                                                   mxs_vector *velocity);

// Tell networking that an object is going to sleep or waking up
EXTERN void PhysNetSetSleep(ObjID obj, 
                            BOOL sleep, 
                            BOOL startNow DEFAULT_TO(FALSE));

// Tell networking that the player has come into contact with an object
EXTERN void PhysNetPlayerContactCreate(cPhysModel *pModel);
EXTERN void PhysNetPlayerContactDestroy(cPhysModel *pModel);

//
// Something a tad different. PhysNet maintains a concept of "borrowing"
// an object while we are in contact with it. If another method somewhere
// breaks that physical contact, but wants to keep ownership of the object,
// it should call this; otherwise, when the contact is broken, the PhysNet
// listener will hand the object back to the world host. Without this, if
// you pick up an object that you are in contact with, you'll wind up not
// owning it, which is evil.
//
// @TBD: Figure out a way to do this internally, with fancier listeners,
// so that other systems don't have to know about PhysNet borrows.
//
EXTERN void PhysNetDiscardBorrow(ObjID obj);

// An object has just entered/left another. This decides whether that
// fact should be transmitted remotely. Returns TRUE iff it has been
// so transmitted; if FALSE, we should deal with it locally.
EXTERN BOOL PhysNetBroadcastEnterExit(ePhysEnterExit inout,
                                      ObjID enteree,
                                      ObjID enterer,
                                      int submod);

// An object's position has just been hard-changed by its owner. Tell
// the clients. Assumes that pos is valid. Not intended for frame-by-frame
// use, but mainly for teleports. Should generally only be called by the
// object's host; will noop otherwise.
EXTERN void PhysNetBroadcastObjPos(ObjID obj,
                                   mxs_vector *pos,
                                   mxs_angvec *fac);
// Same thing; will just get the object's postion first. Object should
// have physics.
EXTERN void PhysNetBroadcastObjPosByObj(ObjID obj);

// An object's gravity has just changed.
EXTERN void PhysNetBroadcastGravity(ObjID obj, mxs_real gravity);

// @HACK: this forces the system to allow network broadcasts of phys/dephys
// messages, even if the object is in a container. Appropriate mainly for
// situations when we are adding/removing something from a container, and
// need to send the phys msg while the object is in the container. Be sure
// to reset this once you're done.
EXTERN void PhysNetForceContainedMsgs(BOOL force);

#endif // !_PHYSNET_H
