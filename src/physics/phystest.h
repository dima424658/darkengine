/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/physics/phystest.h,v 1.9 2000/01/31 09:52:16 adurant Exp $
#pragma once

#ifndef _PHYSTEST_H
#define _PHYSTEST_H

#ifndef SHIP

EXTERN void PhysicsLaunchSphere(mxs_real speed);
EXTERN void PhysicsLaunchOBB(mxs_real speed);
EXTERN void PhysicsSetControl(mxs_real speed);
EXTERN void PhysicsStopControl();

EXTERN void PhysicsJoyride();
EXTERN void PhysicsTestRaycast(ObjID objID);


#ifdef DBG_ON
EXTERN void PhysicsDebugObj(ObjID objID);
EXTERN void PhysicsDebug(int what);
#endif

#endif // SHIP

#endif
