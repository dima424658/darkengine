/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/camovl.h,v 1.4 2000/01/29 13:38:35 adurant Exp $
#pragma once

// Spiffy Remote Camera 2d rendering stuff.

#ifndef _CAMOVL_H_
#define _CAMOVL_H_

EXTERN void CameraOverlayInit();
EXTERN void CameraOverlayTerm();
EXTERN void CameraOverlayOnFrame();
EXTERN void CameraOverlayRender(BOOL bIsScreenLocked);

#endif